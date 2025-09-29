/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   linked_list.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 16:37:04 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 16:18:29 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "linked_list.h"

void ft_ll_init(t_ll_node* n)
{
	if (!n)
		return;
	n->prev = NULL;
	n->next = NULL;
}

int ft_ll_is_linked(const t_ll_node* n)
{
	if (!n)
		return 0;
	return n->prev != NULL || n->next != NULL;
}

void ft_ll_push_front(t_ll_node** head, t_ll_node* n)
{
	if (!head || !n)
		return;
	n->prev = NULL;
	n->next = *head;
	if (*head)
		(*head)->prev = n;
	*head = n;
}

void ft_ll_push_back(t_ll_node** head, t_ll_node* n)
{
	if (!head || !n)
		return;
	if (!*head) {
		ft_ll_push_front(head, n);
		return;
	}
	t_ll_node* tail = *head;
	while (tail->next)
		tail = tail->next;

	n->prev = tail;
	n->next = NULL;
	tail->next = n;
}

t_ll_node* ft_ll_pop_front(t_ll_node** head)
{
	if (!head || !*head)
		return NULL;
	t_ll_node* n = *head;
	*head = n->next;
	if (*head)
		(*head)->prev = NULL;
	n->prev = n->next = NULL;
	return n;
}

void ft_ll_remove(t_ll_node** head, t_ll_node* n)
{
	if (!head || !n)
		return;
	if (n->prev)
		n->prev->next = n->next;
	else if (*head == n)
		*head = n->next;
	if (n->next)
		n->next->prev = n->prev;
	n->prev = n->next = NULL;
}

size_t ft_ll_len(t_ll_node** head)
{
	if (!head || !*head)
		return 0;
	size_t len = 0;
	FT_LL_FOR_EACH(it, *head)
	{
		++len;
	}
	return len;
}