/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: francisco <francisco@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 11:50:20 by francisco         #+#    #+#             */
/*   Updated: 2025/09/23 00:28:49 by francisco        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heap.h"

ft_heap g_ft_heap = {0};

static const size_t TINY_BINS[]  = { 16, 32, 64, TINY_MAX };
static const size_t SMALL_BINS[] = { 256, 512, 1024, 2048, SMALL_MAX };


// all bins must be multiples of FT_ALIGN
_Static_assert((TINY_MAX  % FT_ALIGN) == 0,  "TINY_MAX must be aligned");
_Static_assert((SMALL_MAX % FT_ALIGN) == 0,  "SMALL_MAX must be aligned");

// class boundaries must match bin tops
_Static_assert(TINY_BINS[ARRAY_LEN(TINY_BINS)-1]  == TINY_MAX,  "max TINY bin = TINY_MAX");
_Static_assert(SMALL_BINS[ARRAY_LEN(SMALL_BINS)-1]== SMALL_MAX, "max SMALL bin = SMALL_MAX");

// classes should not overlap
_Static_assert(TINY_MAX < SMALL_BINS[0], "SMALL must start above TINY");

/* Init all lists empty + set min_blocks knobs. */
void ft_heap_init(size_t tiny_min_blocks, size_t small_min_blocks)
{
	g_ft_heap.tiny = NULL;
	g_ft_heap.small = NULL;
	g_ft_heap.large = NULL;
	g_ft_heap.tiny_min_blocks = tiny_min_blocks;
	g_ft_heap.small_min_blocks = small_min_blocks;
}

/* Unmap every zone in tiny/small/large and reset to init state. */
void ft_heap_destroy(void)
{
	ft_zone_list_destroy(&g_ft_heap.tiny);
	ft_zone_list_destroy(&g_ft_heap.small);
	ft_zone_list_destroy(&g_ft_heap.large);

	g_ft_heap.tiny = NULL;
	g_ft_heap.small = NULL;
	g_ft_heap.large = NULL;

	g_ft_heap.tiny_min_blocks = 0;
	g_ft_heap.small_min_blocks = 0;
}

/* Allocator entry points (used by tests and wired by your malloc.c) */
void* ft_heap_malloc(size_t n);void* ft_heap_malloc(size_t n)
{
    // 1) normalize & align the requested size
    size_t need = ft_align_up(n ? n : 1, FT_ALIGN);

    // 2) classify AFTER alignment
    ft_zone_class k = ft_heap_classify(need);

    // 3) LARGE → one mapping per allocation
    if (k == FT_Z_LARGE) {
        ft_zone *z = ft_zone_new_large(need);
        if (!z) return NULL;
        ft_ll_push_front(list_for(FT_Z_LARGE), &z->link);
        return z->mem_begin;
    }

    // 4) TINY/SMALL → pick exact-size slab bin (first >= need)
    size_t bin = ft_heap_pick_bin_size(k, need);
    if (!bin) {
        // defensive fallback if bins don't fully cover thresholds (shouldn't happen)
        ft_zone *z = ft_zone_new_large(need);
        if (!z) return NULL;
        ft_ll_push_front(list_for(FT_Z_LARGE), &z->link);
        return z->mem_begin;
    }

    // 5) find an existing slab zone with the same bin and spare blocks
    ft_ll_node **head = list_for(k);
    ft_zone *z_with_space = NULL;

    FT_LL_FOR_EACH(it, *head) {
        ft_zone *z = FT_CONTAINER_OF(it, ft_zone, link);
        if (z->bin_size == bin && z->free_count > 0) {
            z_with_space = z;
            break;
        }
    }

    // 6) none found → create one
    if (!z_with_space) {
        size_t min_blocks = (k == FT_Z_TINY) ? g_ft_heap.tiny_min_blocks
                                             : g_ft_heap.small_min_blocks;
        if (min_blocks < 1) min_blocks = 1;

        z_with_space = ft_zone_new_slab(k, bin, min_blocks);
        if (!z_with_space) return NULL;

        ft_ll_push_front(head, &z_with_space->link);
    }

    // 7) pop a block from the free list (already FT_ALIGN aligned)
    void *p = ft_zone_pop_block(z_with_space);
    return p; // may be NULL only if unexpected race/logic bug
}


void ft_heap_free(void* p)
{
    if (!p) return;

    ft_zone *z = ft_heap_find_owner(p);
    if (!z) {
        // invalid free: ignore or add a debug assert
        return;
    }

    if (z->klass == FT_Z_LARGE) {
        // unlink from LARGE list and unmap
        ft_ll_node **head = list_for(FT_Z_LARGE);
        ll_unlink(head, &z->link);
        ft_zone_destroy(z);
        return;
    }

    // slab: return block to free list
    // caller is expected to pass a block start; zone code pushes blindly
    ft_zone_push_block(z, p);

    // Optional trimming (disabled by default to reduce munmap):
    // if (z->free_count == z->capacity) {
    //     ft_ll_node **head = list_for(z->klass);
    //     ll_unlink(head, &z->link);
    //     ft_zone_destroy(z);
    // }
}

/* Optional for later: */
void* ft_heap_realloc(void* p, size_t n)
{
    if (!p) return ft_heap_malloc(n);
    if (n == 0) { ft_heap_free(p); return NULL; }

    ft_zone *z = ft_heap_find_owner(p);
    if (!z) {
        // invalid pointer for realloc -> undefined by C; here we fail safe
        return NULL;
    }

    size_t need = ft_align_up(n, FT_ALIGN);

    if (z->klass != FT_Z_LARGE) {
        // slab: if it still fits, keep it
        if (need <= z->bin_size) {
            return p;
        }
        // else grow: allocate new, copy, free old
        void *np = ft_heap_malloc(need);
        if (!np) return NULL;

        size_t to_copy = (z->bin_size < need) ? z->bin_size : need;
        ft_memcpy(np, p, to_copy);
        ft_heap_free(p);
        return np;
    }

    // LARGE: we could shrink-in-place if need <= old, else move
    if (need <= z->bin_size) {
        return p; // keep same mapping when shrinking
    }

    void *np = ft_heap_malloc(need);
    if (!np) return NULL;

    size_t to_copy = (z->bin_size < need) ? z->bin_size : need;
    ft_memcpy(np, p, to_copy);
    ft_heap_free(p);
    return np;
}


/* ---- helpers (tested) ---- */

/* Classify request into TINY/SMALL/LARGE.
 */
ft_zone_class ft_heap_classify(size_t n)
{
	if (n == 0) n = 1;
	return (n <= TINY_MAX) ? FT_Z_TINY : (n <= SMALL_MAX) ? FT_Z_SMALL : FT_Z_LARGE;
}

/* Decide the slab block size for a class.
 * Must return a multiple of FT_ALIGN and >= n.
 * Example: TINY bins {16,32,64,128}, SMALL {256,512,1024,2048,4096}. */
size_t ft_heap_pick_bin_size(ft_zone_class klass, size_t n)
{
	n = ft_align_up(n, FT_ALIGN);

	if (klass == FT_Z_TINY)
		return LB_GE(TINY_BINS, n);
	if (klass == FT_Z_SMALL)
		return LB_GE(SMALL_BINS, n);
	return 0;
}

/* Find which zone owns 'p' by scanning tiny/small/large lists.
 * Return NULL if not found. */
ft_zone* ft_heap_find_owner(const void* p)
{
	// zone lists
	ft_ll_node* zls[3] = {g_ft_heap.tiny, g_ft_heap.small, g_ft_heap.large};

	for (size_t i = 0; i < 3; i++) {
		// zone list
		ft_ll_node* zl = zls[i];

		FT_LL_FOR_EACH(it, zl)
		{
			ft_zone* z = FT_CONTAINER_OF(it, ft_zone, link);

			if (ft_zone_contains(z, p))
				return z;
		}
	}
	return NULL;
}

/* Count zones in a class list. */
size_t ft_heap_zone_count(ft_zone_class klass)
{
	ft_ll_node* zl = (klass == FT_Z_TINY)	 ? g_ft_heap.tiny
					 : (klass == FT_Z_SMALL) ? g_ft_heap.small
											 : g_ft_heap.large;
	return ft_ll_len(&zl);
}

/* Sum of free blocks across all slab zones in a class (LARGE excluded). */
size_t ft_heap_total_free_in_class(ft_zone_class klass)
{
    if (klass == FT_Z_LARGE) return 0;

    ft_ll_node *zl = (klass == FT_Z_TINY) ? g_ft_heap.tiny : g_ft_heap.small;
    size_t total = 0;

    FT_LL_FOR_EACH(it, zl) {
        ft_zone *z = FT_CONTAINER_OF(it, ft_zone, link);
        /* defensive: ensure we only count matching class and slab zones */
        if (z->klass == klass /* && z->kind == FT_ZONE_SLAB */)
            total += z->free_count;
    }
    return total;
}

// helper: pick the right list by class
static inline ft_ll_node **flist_for(ft_zone_class k) {
    return (k == FT_Z_TINY) ? &g_ft_heap.tiny
         : (k == FT_Z_SMALL) ? &g_ft_heap.small
                             : &g_ft_heap.large;
}

static inline void ll_unlink(ft_ll_node **head, ft_ll_node *node) {
    if (!head || !node) return;
    if (node->prev) node->prev->next = node->next; else *head = node->next;
    if (node->next) node->next->prev = node->prev;
    node->prev = node->next = NULL;
}
