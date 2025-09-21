/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 19:02:36 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/22 18:00:27 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zone.h"
#include <sys/mman.h>
#include <unistd.h>

/* ---------------- alignment & pagesize ---------------- */

size_t ft_align_up(size_t n, size_t a)
{
	return (n + (a - 1)) & ~(a - 1);
}

size_t ft_page_size(void)
{
#ifdef __APPLE__
	/* Subject allows getpagesize() on macOS */
	return (size_t)getpagesize();
#else
	/* Subject allows sysconf(_SC_PAGESIZE) on Linux */
	long ps = sysconf(_SC_PAGESIZE);
	return (ps > 0) ? (size_t)ps : 4096u;
#endif
}

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

/* Free-list stored inside free blocks: first word is “next” */
static inline void* ft_blk_pop(void** head)
{
	void* b = *head;
	if (b)
		*head = *(void**)b;
	return b;
}

static inline void ft_blk_push(void** head, void* b)
{
	*(void**)b = *head;
	*head = b;
}

/* ---------------- zone creation/destruction ---------------- */

ft_zone* ft_zone_new_slab(ft_zone_class klass, size_t bin_size, size_t min_blocks)
{
	const size_t ps = ft_page_size();
	const size_t hdr = ft_align_up(sizeof(ft_zone), FT_ALIGN);
	const size_t bsz = ft_align_up(bin_size, FT_ALIGN);

	/* ensure at least min_blocks worth of space, rounded to pages */
	size_t usable_min = ft_align_up(bsz * (min_blocks ? min_blocks : 1), FT_ALIGN);
	size_t total = ft_align_up(hdr + usable_min, ps);

	ft_zone* z = (ft_zone*)ft_map(total);
	if (!z)
		return NULL;

	ft_ll_init(&z->link);
	z->klass = klass;
	z->bin_size = bsz;
	z->mem_begin = (void*)((uintptr_t)z + hdr);

	/* bytes available for blocks inside the mapping */
	size_t raw_area = (size_t)((uintptr_t)z + total - (uintptr_t)z->mem_begin);
	z->capacity = raw_area / bsz;									   /* floor division */
	z->mem_end = (void*)((uintptr_t)z->mem_begin + z->capacity * bsz); /* payload end */
	z->map_end = (void*)((uintptr_t)z + total);						   /* mapping end */

	z->free_count = z->capacity;
	z->free_head = NULL;

	/* carve blocks into free list */
	char* p = (char*)z->mem_begin;
	for (size_t i = 0; i < z->capacity; ++i, p += bsz)
		ft_blk_push(&z->free_head, p);

	return z;
}

ft_zone* ft_zone_new_large(size_t req_bytes)
{
	const size_t ps = ft_page_size();
	const size_t hdr = ft_align_up(sizeof(ft_zone), FT_ALIGN);
	const size_t need = ft_align_up(req_bytes, FT_ALIGN);
	const size_t tot = ft_align_up(hdr + need, ps);

	ft_zone* z = (ft_zone*)ft_map(tot);
	if (!z)
		return NULL;

	ft_ll_init(&z->link);
	z->klass = FT_Z_LARGE;
	z->bin_size = need; /* payload size */
	z->capacity = 1;
	z->free_count = 0; /* allocated by definition */
	z->mem_begin = (void*)((uintptr_t)z + hdr);
	z->mem_end = (void*)((uintptr_t)z->mem_begin + need); /* payload end */
	z->map_end = (void*)((uintptr_t)z + tot);			  /* mapping end */
	z->free_head = NULL;

	return z; /* payload starts at z->mem_begin */
}

void ft_zone_destroy(ft_zone* z)
{
	if (!z)
		return;
	ft_unmap(z, ft_zone_mapped_bytes(z));
}

/* ---------------- slab block ops ---------------- */

void* ft_zone_pop_block(ft_zone* z)
{
	if (!z || z->klass == FT_Z_LARGE)
		return NULL;
	if (!z->free_head)
		return NULL;
	void* p = ft_blk_pop(&z->free_head);
	if (p)
		z->free_count--;
	return p;
}

void ft_zone_push_block(ft_zone* z, void* p)
{
	if (!z || z->klass == FT_Z_LARGE || !p)
		return;
	/* caller must ensure p is a block start aligned to bin_size */
	ft_blk_push(&z->free_head, p);
	z->free_count++;
}

/* ---------------- helpers ---------------- */

int ft_zone_contains(const ft_zone* z, const void* p)
{
	if (!z || !p)
		return 0;
	uintptr_t a = (uintptr_t)p;
	/* Only the payload/block area counts as "inside" for allocation purposes. */
	return (a >= (uintptr_t)z->mem_begin) && (a < (uintptr_t)z->mem_end);
}

size_t ft_zone_mapped_bytes(const ft_zone* z)
{
	if (!z)
		return 0;
	return (size_t)((uintptr_t)z->map_end - (uintptr_t)z);
}

inline struct ft_zone* ft_zone_from_link(struct ft_ll_node* n)
{
	return n ? FT_CONTAINER_OF(n, struct ft_zone, link) : NULL;
}
inline const struct ft_zone* ft_zone_from_link_const(const struct ft_ll_node* n)
{
	return n ? FT_CONTAINER_OF_CONST(n, struct ft_zone, link) : NULL;
}

void ft_zone_list_destroy(ft_ll_node** head)
{
    if (!head || !*head) return;

    for (ft_ll_node* n; (n = ft_ll_pop_front(head)); ) {
        ft_zone* z = FT_CONTAINER_OF(n, ft_zone, link);
        ft_zone_destroy(z);
    }
}
