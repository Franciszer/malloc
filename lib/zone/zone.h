/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 19:02:07 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 15:02:50 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_ZONE_H
#define FT_ZONE_H

#include <stddef.h>						 /* size_t */
#include <stdint.h>						 /* uintptr_t, uint8_t */
#include "data_structures/linked_list.h" /* ft_ll_node */
#include "helpers/helpers.h"

/* Zone classes: slab for TINY/SMALL, capacity-1 for LARGE */
typedef enum t_zone_class { FT_Z_TINY, FT_Z_SMALL, FT_Z_LARGE } t_zone_class;

/* Occupancy convention for slab zones */
#define FT_OCC_FREE 0u // mmap gives zero-filled pages → free by default
#define FT_OCC_USED 1u

/* One zone = one bin size (uniform blocks). LARGE is capacity=1. */
typedef struct s_zone {
	/* ---- intrusive linkage in the heap’s per-class container ---- */
	ft_ll_node link;

	/* ---- identity / geometry ---- */
	t_zone_class klass; /* FT_Z_TINY / FT_Z_SMALL / FT_Z_LARGE */
	size_t bin_size;	/* slab block size; for LARGE: payload size */
	size_t capacity;	/* # of blocks (slab); 1 for LARGE */
	size_t free_count;	/* # of free blocks (slab); 0 for LARGE */

	/* ---- mapping & payload bounds (within the same mmap) ---- */
	void* mem_begin; /* first block/payload byte (aligned to FT_ALIGN) */
	void* mem_end;	 /* one-past-last block/payload byte */
	void* map_end;	 /* one-past-end of the whole mapping */

	/* ---- slab occupancy (array-of-bytes), unused for LARGE ----
	Stored INSIDE this mapping, right after the payload region.
	Length == capacity; values are FT_OCC_FREE or FT_OCC_USED.
	*/
	uint8_t* occ; /* NULL for LARGE */
} t_zone;

// an index that is not valid for an array of size capacity, used for error returns
#define FT_MALLOC_ERR_SLAB_INDEX(z) (z->capacity)

/* --- zone lifecycle (no list management here) --- */

/* Unified constructor:
 * - FT_Z_TINY/FT_Z_SMALL: bin_size = block size (must be multiple of FT_ALIGN).
 *   min_blocks is a lower bound; page rounding may yield capacity > min_blocks.
 * - FT_Z_LARGE          : bin_size = payload size (aligned by implementation),
 *   min_blocks ignored; capacity == 1.
 */
t_zone* ft_zone_new(t_zone_class klass, size_t bin_size, size_t min_blocks);

/* Convenience wrappers (optional, keep for clarity) */
static inline t_zone* t_zone_new_slab(t_zone_class k, size_t bin_size, size_t min_blocks)
{
	return ft_zone_new(k, bin_size, min_blocks);
}
static inline t_zone* t_zone_new_large(size_t req_bytes)
{
	return ft_zone_new(FT_Z_LARGE, req_bytes, 1);
}

/* Destroy the whole zone (munmap). */
void ft_zone_destroy(t_zone* z);

/* --- slab block ops (no heap/container logic) --- */

/* Allocate one block from a slab zone (returns NULL if none).
 * Undefined for LARGE. */
void* ft_zone_alloc_block(t_zone* z);

/* Free one block back to a slab zone.
 * Pointer must be a block start inside this zone.
 * Undefined for LARGE. */
void ft_zone_free_block(t_zone* z, void* p);

/* --- helpers --- */

/* True if p is within [mem_begin, mem_end) of this zone (payload region). */
int ft_zone_contains(const t_zone* z, const void* p);

/* Total bytes mapped for this zone (header + metadata + payload). */
size_t ft_zone_mapped_bytes(const t_zone* z);

/* True if the zone can hand out at least one block (LARGE or slab). */
int ft_zone_has_space(const t_zone* z);

/* Handy block <-> index helpers (inline) */
static inline void* ft_zone_block_at(const t_zone* z, size_t i)
{
	return (void*)((uintptr_t)z->mem_begin + i * z->bin_size);
}
static inline size_t ft_zone_index_of(const t_zone* z, const void* p)
{
	return (size_t)(((uintptr_t)p - (uintptr_t)z->mem_begin) / z->bin_size);
}

/* Recover zone pointer from intrusive list node. */
inline struct s_zone* ft_zone_from_link(struct ft_ll_node* n)
{
	return n ? FT_CONTAINER_OF(n, struct s_zone, link) : NULL;
}
inline const struct s_zone* ft_zone_from_link_const(const struct ft_ll_node* n)
{
	return n ? FT_CONTAINER_OF_CONST(n, struct s_zone, link) : NULL;
}

#endif /* FT_ZONE_H */
