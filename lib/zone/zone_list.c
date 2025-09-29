/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_list.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: francisco <francisco@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:16:35 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 03:03:18 by francisco        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zone/zone_list.h"

void ft_zone_ll_destroy(t_ll_node** head)
{
	if (!head)
		return;

	for (t_ll_node* n; (n = ft_ll_pop_front(head));) {
		t_zone* z = FT_CONTAINER_OF(n, t_zone, link);
		ft_zone_destroy(z);
	}
}

t_zone* ft_zone_ll_find_container_of(t_ll_node* head, const void* p)
{
	if (!p)
		return NULL;
	FT_LL_FOR_EACH(it, head)
	{
		t_zone* z = FT_CONTAINER_OF(it, t_zone, link);
		if (ft_zone_contains(z, p))
			return z;
	}
	return NULL;
}

t_zone* ft_zone_ll_first_with_space(t_ll_node* head)
{
	FT_LL_FOR_EACH(it, head)
	{
		t_zone* z = FT_CONTAINER_OF(it, t_zone, link);
		if (ft_zone_has_space(z))
			return z;
	}
	return NULL;
}

/* find the zone with the smallest mem_begin strictly greater than 'after' */
static t_zone* min_zone_after(t_ll_node* head, const void* after)
{
	t_zone* best = NULL;
	uintptr_t cutoff = (uintptr_t)after;

	FT_LL_FOR_EACH(it, head)
	{
		t_zone* z = FT_CONTAINER_OF(it, t_zone, link);
		uintptr_t mb = (uintptr_t)z->mem_begin;
		if (mb > cutoff && (!best || mb < (uintptr_t)best->mem_begin))
			best = z;
	}
	return best;
}

size_t ft_zone_ll_print_sorted(const char* label, t_ll_node* head)
{
	if (!head)
		return 0;

	size_t total = 0;
	size_t nz = ft_ll_len(&head);
	const void* last = (const void*)0; /* -∞ for our purposes */

	for (size_t i = 0; i < nz; ++i) {
		t_zone* z = min_zone_after(head, last);
		if (!z)
			break;

		/* header line for this zone */
		if (label) {
			ft_putstr(label);
			ft_putstr(" : ");
		}
		ft_puthex_ptr(z->mem_begin);
		ft_putstr("\n");

		// in ft_zone_ll_print_sorted, right before ft_zone_print_blocks(z)
		ft_putstr("  [cap=");
		ft_putusize(z->capacity);
		ft_putstr(" free=");
		ft_putusize(z->free_count);
		ft_putstr(" bin=");
		ft_putusize(z->bin_size);
		ft_putstr("]\n");

		/* zone-level printing (your function) */
		total += ft_zone_print_blocks(z);

		last = z->mem_begin;
	}
	return total;
}