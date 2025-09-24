/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_list.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:16:35 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 15:06:26 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zone/zone_list.h"

void ft_zone_ll_destroy(ft_ll_node** head)
{
	if (!head)
		return;

	for (ft_ll_node* n; (n = ft_ll_pop_front(head));) {
		t_zone* z = FT_CONTAINER_OF(n, t_zone, link);
		ft_zone_destroy(z);
	}
	/* *head already NULL thanks to pop_front loop */
}

t_zone* ft_zone_ll_find_container_of(ft_ll_node* head, const void* p)
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

t_zone* ft_zone_ll_first_with_space(ft_ll_node* head)
{
	FT_LL_FOR_EACH(it, head)
	{
		t_zone* z = FT_CONTAINER_OF(it, t_zone, link);
		if (ft_zone_has_space(z))
			return z;
	}
	return NULL;
}