/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   linked_list.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 16:37:04 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/21 17:06:51 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "linked_list.h"

void ft_ll_init(ft_ll_node* n)
{
	if (!n)
		return;
	n->prev = NULL;
	n->next = NULL;
}

int ft_ll_is_linked(const ft_ll_node* n)
{
	if (!n)
		return 0;
	return n->prev != NULL || n->next != NULL;
}

static void ft__ll_insert_before(ft_ll_node** head, ft_ll_node* pos, ft_ll_node* n)
{
	/* Insert n before pos; if pos is NULL, insert at end (handled by caller). */
	n->next = pos;
	if (pos) {
		n->prev = pos->prev;
		pos->prev = n;
	} else {
		n->prev = NULL;
	}
	if (n->prev) {
		n->prev->next = n;
	} else {
		/* n is new head */
		*head = n;
	}
}

void ft_ll_push_front(ft_ll_node** head, ft_ll_node* n)
{
	if (!head || !n)
		return;
	n->prev = NULL;
	n->next = *head;
	if (*head)
		(*head)->prev = n;
	*head = n;
}

void ft_ll_push_back(ft_ll_node** head, ft_ll_node* n)
{
	if (!head || !n)
		return;
	if (!*head) {
		ft_ll_push_front(head, n);
		return;
	}
	ft_ll_node* tail = *head;
	while (tail->next)
		tail = tail->next;
	/* Insert after tail */
	n->prev = tail;
	n->next = NULL;
	tail->next = n;
}

ft_ll_node* ft_ll_pop_front(ft_ll_node** head)
{
	if (!head || !*head)
		return NULL;
	ft_ll_node* n = *head;
	*head = n->next;
	if (*head)
		(*head)->prev = NULL;
	n->prev = n->next = NULL;
	return n;
}

void ft_ll_remove(ft_ll_node** head, ft_ll_node* n)
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
