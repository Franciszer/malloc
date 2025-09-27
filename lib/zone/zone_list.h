/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_list.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:16:21 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/27 19:21:56 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_ZONE_LIST_H
#define FT_ZONE_LIST_H

#include "zone/zone.h"
#include "data_structures/linked_list.h"

/* Destroy all zones in an intrusive list and null the head.
 * Signature mirrors the previous ft_zone_list_destroy: it takes the
 * intrusive list head (t_ll_node**).
 */
void ft_zone_ll_destroy(t_ll_node** head);

/* Find the zone in the list whose payload contains p; NULL if none. */
t_zone* ft_zone_ll_find_container_of(t_ll_node* head, const void* p);

/* Return the first zone in list that currently has space (calls ft_zone_has_space). */
t_zone* ft_zone_ll_first_with_space(t_ll_node* head);

/* Print a class header and all zones' blocks in ascending zone-base order.
 * Header format: "<LABEL> : 0x<min-zone-base>\n"
 * Returns total bytes printed for this class.
 */
size_t ft_zone_ll_show_class(const char* label, t_ll_node* head);

/* Print all zones in ascending order of z->mem_begin.
 * For each zone:
 *   "<label> : <zone_mem_begin>\n"
 *   ... then ft_zone_print_blocks(z)
 * Returns the total summed payload bytes printed (sum of block sizes).
 */
size_t ft_zone_ll_print_sorted(const char* label, t_ll_node* head);

#endif /* FT_ZONE_LIST_H */
