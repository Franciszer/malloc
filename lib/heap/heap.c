/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 11:50:20 by francisco         #+#    #+#             */
/*   Updated: 2025/09/28 02:53:05 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heap.h"

#include "heap.h"
#include "zone/zone_list.h" // for ft_zone_ll_destroy, _first_with_space, _find_container_of

static inline t_ll_node** list_for(t_zone_class k);
static inline size_t ft_bin_size_for_k(t_zone_class k);

t_heap g_heap = {0};

#ifndef FT_HEAP_NO_CTOR
__attribute__((constructor)) static void ft_malloc_ctor(void)
{
	ft_heap_init(TINY_MAX, SMALL_MAX); /* or your chosen min_blocks */
}
#endif

#ifndef FT_HEAP_NO_DTOR
__attribute__((destructor)) static void ft_malloc_dtor(void)
{
	ft_heap_destroy();
}
#endif

/* Init all lists empty + set min_blocks knobs. */
void ft_heap_init(size_t tiny_min_blocks, size_t small_min_blocks)
{
	g_heap = (t_heap){0}; // initializes every field to 0
	g_heap.tiny_min_blocks = tiny_min_blocks;
	g_heap.small_min_blocks = small_min_blocks;
}

/* Unmap every zone in tiny/small/large and reset to init state. */
void ft_heap_destroy(void)
{
	for (int i = 0; i < N_ZONE_CATEGORIES; ++i) {
		ft_zone_ll_destroy(&g_heap.zls[i]);
	}

	g_heap.tiny_min_blocks = 0;
	g_heap.small_min_blocks = 0;
}
#include "helpers/helpers.h"
/* Allocator entry points (used by tests and wired by your malloc.c) */
void* ft_heap_malloc(size_t n)
{
	// 1) normalize & align
	size_t need = ft_align_up(n ? n : 1, FT_ALIGN);

	// 2) classify
	t_zone_class k = ft_heap_classify(need);

	t_ll_node** head = list_for(k);
 
	// 3) LARGE → one mapping per allocation
	if (k == FT_Z_LARGE) {
		t_zone* z = t_zone_new_large(need); // inline wrapper to ft_zone_new
		if (!z)
			return NULL;
		ft_ll_push_front(head, &z->link);
		return z->mem_begin;
	}

	// 4) TINY/SMALL → find an existing zone with space
	t_zone* z = ft_zone_ll_first_with_space(*head);

	// 5) none → create one
	if (!z) {
		size_t min_blocks = (k == FT_Z_TINY) ? g_heap.tiny_min_blocks : g_heap.small_min_blocks;
		if (min_blocks < 1)
			min_blocks = 1;

		size_t bin_size = ft_bin_size_for_k(k);
		z = ft_zone_new(k, bin_size, min_blocks);
		if (!z)
			return NULL;

		ft_ll_push_front(head, &z->link); // <-- push the NEW zone
	}

	// 6) allocate a block from that zone
	return ft_zone_alloc_block(z); // returns NULL only if race/logic bug
}

void ft_heap_free(void* p)
{
	if (!p)
		return;

	t_zone* z = ft_heap_find_owner(p);
	if (!z) {
		// invalid free
		return;
	}

	if (z->klass == FT_Z_LARGE) {
		// unlink & unmap whole LARGE zone
		t_ll_node** head = list_for(FT_Z_LARGE);
		ft_ll_remove(head, &z->link);
		ft_zone_destroy(z);
		return;
	}

	// slab: return block
	ft_zone_free_block(z, p);

	// trim of fully-empty zone:
	if (z->free_count == z->capacity) {
		t_ll_node** head = list_for(z->klass);
		ft_ll_remove(head, &z->link);
		ft_zone_destroy(z);
	}
}

/* Optional for later: */
void* ft_heap_realloc(void* p, size_t n)
{
	if (!p)
		return ft_heap_malloc(n);
	if (n == 0) {
		ft_heap_free(p);
		return NULL;
	}

	t_zone* z = ft_heap_find_owner(p);
	if (!z) {
		// undefined by C; we choose to fail safe
		return NULL;
	}

	size_t need = ft_align_up(n, FT_ALIGN);

	if (z->klass != FT_Z_LARGE) {
		// If it still fits in this slab block, keep it
		if (need <= z->bin_size)
			return p;

		// Grow: allocate new, copy min(old,new), free old
		void* np = ft_heap_malloc(need);
		if (!np)
			return NULL;

		size_t to_copy = (z->bin_size < need) ? z->bin_size : need;
		ft_memcpy(np, p, to_copy);
		ft_heap_free(p);
		return np;
	}

	// LARGE: keep if shrinking, otherwise move
	if (need <= z->bin_size)
		return p;

	void* np = ft_heap_malloc(need);
	if (!np)
		return NULL;

	size_t to_copy = (z->bin_size < need) ? z->bin_size : need;
	ft_memcpy(np, p, to_copy);
	ft_heap_free(p);
	return np;
}

/* ---- helpers (tested) ---- */

/* Classify request into TINY/SMALL/LARGE.
 */
t_zone_class ft_heap_classify(size_t n) {
    if (n == 0) n = 1;
    if (n <= TINY_MAX)  return FT_Z_TINY;
    if (n <= SMALL_MAX) return FT_Z_SMALL;
    return FT_Z_LARGE;
}


t_zone* ft_heap_find_owner(const void* p)
{
	if (!p)
		return NULL;
	for (size_t i = 0; i < N_ZONE_CATEGORIES; ++i) {
		t_ll_node* head = g_heap.zls[i];
		t_zone* z = ft_zone_ll_find_container_of(head, p);
		if (z)
			return z;
	}
	return NULL;
}

size_t ft_heap_zone_count(t_zone_class klass)
{
	return ft_ll_len(&g_heap.zls[klass]);
}

size_t ft_heap_total_free_in_class(t_zone_class klass)
{
	if (klass == FT_Z_LARGE)
		return 0;

	size_t total = 0;
	t_ll_node* head = g_heap.zls[klass];

	FT_LL_FOR_EACH(it, head)
	{
		t_zone* z = FT_CONTAINER_OF(it, t_zone, link);
		if (z->klass == klass)
			total += z->free_count;
	}
	return total;
}

// pick the right list by class
static inline t_ll_node** list_for(t_zone_class k)
{
	return &g_heap.zls[(int)k];
}

static inline size_t ft_bin_size_for_k(t_zone_class k)
{
	return (k == FT_Z_TINY) ? TINY_MAX : SMALL_MAX; // one bin per class
}

/* Print TINY/SMALL/LARGE zone contents and a final "Total : N bytes" line. */
size_t ft_heap_show_alloc_mem(void)
{
    size_t total = 0;

    total += ft_zone_ll_print_sorted("TINY",  g_heap.zls[FT_Z_TINY]);
    total += ft_zone_ll_print_sorted("SMALL", g_heap.zls[FT_Z_SMALL]);
    total += ft_zone_ll_print_sorted("LARGE", g_heap.zls[FT_Z_LARGE]);

    ft_putstr("Total : ");
    ft_putusize(total);
    ft_putstr(" bytes\n");

    return total;
}
