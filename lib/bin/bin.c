/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bin.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 20:12:22 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/21 20:14:20 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stddef.h>
#include <stdint.h>
#include "data_structures/linked_list.h"
#include "zone/zone.h"

typedef struct ft_bin {
    /* Lists of zones of this size class */
    ft_ll_node   *partial;      /* zones with free space */
    ft_ll_node   *full;         /* zones with no free space */

    /* One cached fully-empty zone (not in lists) per reclaim policy */
    ft_zone      *empty_cached; /* NULL or a slab zone with free_count == capacity */

    /* Configuration */
    ft_zone_class klass;        /* FT_Z_TINY or FT_Z_SMALL */
    size_t        bin_size;     /* aligned block size for this bin */
    size_t        min_blocks;   /* minimum blocks per new zone (>=100 in real build) */
} ft_bin;

/* ---- lifecycle ---- */

/* Initialize a bin with class (TINY/SMALL), bin_size and min_blocks.
 * - Must align bin_size up to FT_ALIGN internally if needed.
 * - Sets lists empty and clears empty_cached.
 */
void   ft_bin_init(ft_bin *bin, ft_zone_class klass, size_t bin_size, size_t min_blocks) {
    
}

/* Destroy the bin completely:
 * - Destroy (munmap) all zones in 'partial' and 'full'
 * - Destroy 'empty_cached' if set
 * - Reset fields to the same state as after ft_bin_init(...).
 */
void   ft_bin_destroy(ft_bin *bin);

/* ---- allocation / free ---- */

/* Allocate one block from this bin.
 * Behavior:
 *   - If empty_cached exists, move it back into 'partial' and allocate from it.
 *   - Else, if 'partial' non-empty, allocate from its head zone.
 *   - Else, create a new slab zone and push it to 'partial', then allocate.
 *   - If a zone becomes full after this allocation, move it to 'full'.
 * Returns: pointer to a block, or NULL on failure (e.g. mmap error).
 */
void  *ft_bin_alloc(ft_bin *bin);

/* Free a pointer that may belong to a zone of this bin.
 * - Scans partial/full (and empty_cached) to find the owner zone (ft_zone_contains).
 * - If found:
 *     * Push the block back into that zone.
 *     * If it was in 'full', move it to 'partial'.
 *     * If the zone becomes EMPTY (free_count == capacity), apply reclaim policy:
 *          - If empty_cached is NULL: remove zone from lists (if present), set empty_cached = zone.
 *          - Else: destroy the just-emptied zone.
 *   Returns 1 if freed here, 0 if the pointer does not belong to any zone in this bin.
 */
int    ft_bin_free_owned(ft_bin *bin, void *ptr);

/* ---- queries / helpers (for tests and allocator integration) ---- */

/* Find which zone in this bin owns 'ptr' (search partial/full and empty_cached).
 * Returns the zone pointer or NULL if not found.
 */
ft_zone *ft_bin_find_owner(const ft_bin *bin, const void *ptr);

/* Counts (do NOT include empty_cached in these counts) */
size_t  ft_bin_partial_count(const ft_bin *bin);
size_t  ft_bin_full_count(const ft_bin *bin);

/* Total free blocks in all zones in 'partial' (excluding empty_cached). */
size_t  ft_bin_partial_free_blocks(const ft_bin *bin);

/* Debug: sanity-check internal invariants (optional; may be no-op in release):
 * - partial list zones all have free_count > 0
 * - full list zones all have free_count == 0
 * - empty_cached is either NULL or (free_count == capacity) and not present in lists
 * Return 0 on success, non-zero on invariant violation.
 */
int     ft_bin_check_invariants(const ft_bin *bin);
