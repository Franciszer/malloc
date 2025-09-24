/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 22:32:52 by francisco         #+#    #+#             */
/*   Updated: 2025/09/24 14:21:31 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_HEAP_H
#define FT_HEAP_H

#include <stddef.h>
#include "zone/zone.h"
#include "zone/zone_list.h"
#include "data_structures/linked_list.h"
#include "helpers/helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TINY_MAX 128
#define SMALL_MAX 4096

#define N_ZONE_CATEGORIES 3

/* Minimal front-end manager:
 * - tiny  : slab zones (various bin sizes)
 * - small : slab zones (various bin sizes)
 * - large : capacity-1 zones
 *
 * v1 policy:
 * - Do NOT auto-destroy empty slab zones (keep them in the lists).
 * - LARGE zones are always unmapped on free.
 */
typedef struct s_heap {
	ft_ll_node* zls[N_ZONE_CATEGORIES]; // zone lists
	size_t tiny_min_blocks;				/* >=100 in prod; tests may use smaller */
	size_t small_min_blocks;
} t_heap;

/* Global heap state (define in heap.c) */
extern t_heap g_heap;

/* Init all lists empty + set min_blocks knobs. */
void ft_heap_init(size_t tiny_min_blocks, size_t small_min_blocks);
/* Unmap every zone in tiny/small/large and reset to init state. */
void ft_heap_destroy(void);

/* Allocator entry points (used by tests and wired by your malloc.c) */
void* ft_heap_malloc(size_t n);

void ft_heap_free(void* p);
/* Optional for later: */
void* ft_heap_realloc(void* p, size_t n);

/* ---- helpers (tested) ---- */

/* Classify request into TINY/SMALL/LARGE.
 * Pick sane defaults in heap.c (e.g., TINY_MAX=128, SMALL_MAX=4096). */
t_zone_class ft_heap_classify(size_t n);

/* Find which zone owns 'p' by scanning tiny/small/large lists.
 * Return NULL if not found. */
t_zone* ft_heap_find_owner(const void* p);

/* Count zones in a class list. */
size_t ft_heap_zone_count(t_zone_class klass);

/* Sum of free blocks across all slab zones in a class (LARGE excluded). */
size_t ft_heap_total_free_in_class(t_zone_class klass);

static inline ft_ll_node** list_for(t_zone_class k);

static inline void ll_unlink(ft_ll_node** head, ft_ll_node* node);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* FT_HEAP_H */
