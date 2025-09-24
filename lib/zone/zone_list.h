/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_list.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:16:21 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 15:03:51 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_ZONE_LIST_H
#define FT_ZONE_LIST_H

#include "zone/zone.h"
#include "data_structures/linked_list.h"

/* Destroy all zones in an intrusive list and null the head.
 * Signature mirrors the previous ft_zone_list_destroy: it takes the
 * intrusive list head (ft_ll_node**).
 */
void ft_zone_ll_destroy(ft_ll_node** head);

/* Find the zone in the list whose payload contains p; NULL if none. */
t_zone* ft_zone_ll_find_container_of(ft_ll_node* head, const void* p);

/* Return the first zone in list that currently has space (calls ft_zone_has_space). */
t_zone* ft_zone_ll_first_with_space(ft_ll_node* head);

#endif /* FT_ZONE_LIST_H */
