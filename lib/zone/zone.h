/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 19:02:07 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/23 18:02:11 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_ZONE_H
#define FT_ZONE_H

#include <stddef.h>						 /* size_t */
#include <stdint.h>						 /* uintptr_t */
#include "data_structures/linked_list.h" /* ft_ll_node */

/* Alignment for returned payloads and internal pointers */
#define FT_ALIGN 16

/* Zone “classes” (containers): slab for TINY/SMALL, or one-off LARGE */
typedef enum ft_zone_class { FT_Z_TINY, FT_Z_SMALL, FT_Z_LARGE } ft_zone_class;

/* A zone is an mmap’d region:
 * [ ft_zone header ][ aligned block area ... up to mem_end )
 */
typedef struct ft_zone {
	ft_ll_node link;	 /* intrusive node: used in our zone lists */
	ft_zone_class klass; /* TINY / SMALL / LARGE */
	size_t bin_size;	 /* slab block size; for LARGE: payload size */
	size_t capacity;	 /* # of blocks in slab; 1 for LARGE */
	size_t free_count;	 /* how many free blocks remain (slab) */
	void* mem_begin;	 /* start of block/payload area (aligned) */
	void* mem_end;		 /* end of block/payload area (one past) */
	void* map_end;		 /* end of mapping (one past) */
	void* free_head;	 /* slab free-list head (NULL for LARGE) */
} ft_zone;

/* --- small utility funcs --- */
size_t ft_page_size(void);				/* getpagesize/sysconf */
size_t ft_align_up(size_t n, size_t a); /* round up to multiple of a */

/* --- zone lifecycle --- */
ft_zone* ft_zone_new(ft_zone_class klass, size_t bin_size, size_t min_blocks);
/* --- zone lifecycle --- */
ft_zone* ft_zone_new_slab(ft_zone_class klass,
						  size_t bin_size,
						  size_t min_blocks); /* ≥ min_blocks, page-multiple */
ft_zone* ft_zone_new_large(size_t req_bytes); /* capacity == 1 */
void ft_zone_destroy(ft_zone* z);			  /* munmap the whole zone */

/* --- slab block ops --- */
void* ft_zone_pop_block(ft_zone* z);		  /* allocate one block in slab */
void ft_zone_push_block(ft_zone* z, void* p); /* free one block in slab */

/* --- helpers --- */
int ft_zone_contains(const ft_zone* z, const void* p); /* ptr in block area? */
size_t ft_zone_mapped_bytes(const ft_zone* z);		   /* mapping size */

inline struct ft_zone* ft_zone_from_link(struct ft_ll_node* n);
inline const struct ft_zone* ft_zone_from_link_const(const struct ft_ll_node* n);

void ft_zone_list_destroy(ft_ll_node** head);

#endif /* FT_ZONE_H */
