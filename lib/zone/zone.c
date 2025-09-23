	/* ************************************************************************** */
	/*                                                                            */
	/*                                                        :::      ::::::::   */
	/*   zone.c                                             :+:      :+:    :+:   */
	/*                                                    +:+ +:+         +:+     */
	/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
	/*                                                +#+#+#+#+#+   +#+           */
	/*   Created: 2025/09/21 19:02:36 by frthierr          #+#    #+#             */
	/*   Updated: 2025/09/23 21:36:19 by frthierr         ###   ########.fr       */
	/*                                                                            */
	/* ************************************************************************** */

	#include "zone.h"
	#include <sys/mman.h>
	#include <unistd.h>

	/* ---------------- Portability ---------------- */

	#ifndef MAP_ANONYMOUS
	#ifdef MAP_ANON
	#define MAP_ANONYMOUS MAP_ANON
	#endif
	#endif

	#ifndef MAP_ANON
	#ifdef MAP_ANONYMOUS
	#define MAP_ANON MAP_ANONYMOUS
	#endif
	#endif

	#if !defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
	#error                                                                                             \
		"Anonymous mmap flags unavailable. On Linux define _DEFAULT_SOURCE (or _GNU_SOURCE); on macOS define _DARWIN_C_SOURCE."
	#endif

	// static declarations

	static size_t ft_zone_find_first_free_block(const t_zone* z);
	static t_zone* ft_zone_make_large(size_t hdr, size_t ps, size_t need);
	static t_zone*ft_zone_make_slab(t_zone_class klass, size_t hdr, size_t ps, size_t bsz, size_t min_blocks);

	/* ---------------- internal mmap helpers ---------------- */

	static void* ft_map(size_t bytes)
	{
		void* p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		return (p == MAP_FAILED) ? NULL : p;
	}

	static void ft_unmap(void* p, size_t bytes)
	{
		if (p && bytes)
			(void)munmap(p, bytes);
	}

	/* ---------------- zone creation/destruction ---------------- */

	// t_zone_new: create either a slab (FT_Z_TINY/FT_Z_SMALL) or a large zone (FT_Z_LARGE).
	// - Slab: payload = capacity * bsz, then occ[capacity] right after payload.
	// - Large: capacity = 1, no occ[].
	// Requires: ft_page_size(), ft_align_up(), ft_ll_init(), and your mmap wrappers (ft_map/ft_unmap)
	// in zone.c.

	t_zone* ft_zone_new(t_zone_class klass, size_t bin_size, size_t min_blocks)
	{
		const size_t ps = ft_page_size();						  // from helpers
		const size_t hdr = ft_align_up(sizeof(t_zone), FT_ALIGN); // keep payload aligned

		if (klass == FT_Z_LARGE) {
			const size_t need = ft_align_up(bin_size ? bin_size : 1, FT_ALIGN);
			return ft_zone_make_large(hdr, ps, need);
		}

		const size_t bsz = ft_align_up(bin_size ? bin_size : 1, FT_ALIGN);
		const size_t mb = min_blocks ? min_blocks : 1;
		return ft_zone_make_slab(klass, hdr, ps, bsz, mb);
	}

	static t_zone* ft_zone_make_large(size_t hdr, size_t ps, size_t need)
	{
		const size_t total = ft_align_up(hdr + need, ps);
		t_zone* z = (t_zone*)ft_map(total);
		if (!z)
			return NULL;

		ft_ll_init(&z->link);
		z->klass = FT_Z_LARGE;
		z->bin_size = need;
		z->capacity = 1;
		z->free_count = 0;
		z->mem_begin = (void*)((uintptr_t)z + hdr);
		z->mem_end = (void*)((uintptr_t)z->mem_begin + need);
		z->occ = NULL;
		z->map_end = (void*)((uintptr_t)z + total);
		return z;
	}

	static t_zone*
	ft_zone_make_slab(t_zone_class klass, size_t hdr, size_t ps, size_t bsz, size_t min_blocks)
	{
		// each block costs: payload (bsz) + 1 byte in occ[]
		const size_t total_min = hdr + min_blocks * (bsz + 1);
		const size_t total = ft_align_up(total_min, ps);
		const size_t raw = total - hdr;
		const size_t cap = raw / (bsz + 1);
		if (cap == 0)
			return NULL;

		const size_t pay_bytes = cap * bsz;

		t_zone* z = (t_zone*)ft_map(total);
		if (!z)
			return NULL;

		ft_ll_init(&z->link);
		z->klass = klass; // FT_Z_TINY / FT_Z_SMALL
		z->bin_size = bsz;
		z->capacity = cap;
		z->free_count = cap;

		z->mem_begin = (void*)((uintptr_t)z + hdr);
		z->mem_end = (void*)((uintptr_t)z->mem_begin + pay_bytes);

		// occ[] sits right after payload; length == capacity
		z->occ = (uint8_t*)z->mem_end;

		z->map_end = (void*)((uintptr_t)z + total);

		// If FT_OCC_FREE == 0 (recommended), mmap zero-fill means occ[] is already “all free”.
		// If you use FT_OCC_FREE == 1, initialize here:
		// for (size_t i = 0; i < cap; ++i) z->occ[i] = FT_OCC_FREE;

		return z;
	}

	void ft_zone_destroy(t_zone* z)
	{
		if (!z)
			return;
		ft_unmap(z, ft_zone_mapped_bytes(z));
	}

	/* ---------------- slab block ops ---------------- */

	void* ft_zone_alloc_block(t_zone* z)
	{
		if (!z || z->klass == FT_Z_LARGE || !(z->free_count))
			return NULL;

		size_t free_blk_idx = ft_zone_find_first_free_block(z);

		if (free_blk_idx == FT_MALLOC_ERR_SLAB_INDEX(z))
			return NULL;

		z->free_count--;
		
		z->occ[free_blk_idx] = FT_OCC_USED;
		return (void*)((char*)z->mem_begin + free_blk_idx * z->bin_size); 
	}

	void ft_zone_free_block(t_zone* z, void* p)
	{
		if (!z || z->klass == FT_Z_LARGE || !p) return;

		// Optional: fast bail if outside
		if (!ft_zone_contains(z, p)) return;

		size_t idx = ft_zone_index_of(z, p);
		if (idx >= z->capacity) return;         // defensive

		if (z->occ[idx] == FT_OCC_USED) {
			z->occ[idx] = FT_OCC_FREE;
			z->free_count++;
		}
	}
	/* ---------------- helpers ---------------- */

	int ft_zone_contains(const t_zone* z, const void* p)
	{
		if (!z || !p)
			return 0;
		uintptr_t a = (uintptr_t)p;
		/* Only the payload/block area counts as "inside" for allocation purposes. */
		return (a >= (uintptr_t)z->mem_begin) && (a < (uintptr_t)z->mem_end);
	}

	size_t ft_zone_mapped_bytes(const t_zone* z)
	{
		if (!z)
			return 0;
		return (size_t)((uintptr_t)z->map_end - (uintptr_t)z);
	}

	// returns index of free block
	// FT_MALLOC_ERR_SLAB_INDEX if none
	static size_t ft_zone_find_first_free_block(const t_zone* z)
	{
		for (size_t i = 0; i < z->capacity; i++) {
			if (!z->occ[i])
				return i;
		}

		return FT_MALLOC_ERR_SLAB_INDEX(z);
	}
