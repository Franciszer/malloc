/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   linked_list.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 16:37:16 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/21 17:06:00 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_LINKED_LIST_H
#define FT_LINKED_LIST_H

#include <stddef.h>

/*
** Intrusive doubly-linked list node.
** Embed this as a field in any struct you want to link.
*/
typedef struct ft_ll_node {
    struct ft_ll_node *prev;
    struct ft_ll_node *next;
} ft_ll_node;

/* Initialize a node (detached state). */
void        ft_ll_init(ft_ll_node *n);

/* Returns non-zero if the node is currently linked into a list. */
int         ft_ll_is_linked(const ft_ll_node *n);

/* Push node to the front of the list headed by *head. */
void        ft_ll_push_front(ft_ll_node **head, ft_ll_node *n);

/* Push node to the back of the list headed by *head. */
void        ft_ll_push_back(ft_ll_node **head, ft_ll_node *n);

/* Pop and return the front node (NULL if empty). */
ft_ll_node *ft_ll_pop_front(ft_ll_node **head);

/* Remove a specific node from the list (no-op if already detached). */
void        ft_ll_remove(ft_ll_node **head, ft_ll_node *n);

/* Simple iteration helper macro (safe against removal of 'it' during loop). */
#define FT_LL_FOR_EACH(it, head) \
    for (ft_ll_node *it = (head); it != NULL; it = it->next)

/* Variant that allows removing 'it' safely during iteration. */
#define FT_LL_FOR_EACH_SAFE(it, tmp, head) \
    for (ft_ll_node *it = (head), *tmp = (it ? it->next : NULL); \
         it != NULL; it = tmp, tmp = (it ? it->next : NULL))

#endif /* FT_LINKED_LIST_H */
