/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   linked_list.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 16:37:16 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 19:01:57 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_LINKED_LIST_H
#define FT_LINKED_LIST_H

#include <stddef.h>

/*
** Intrusive doubly-linked list node.
** Embed this as a field in any struct you want to link.
*/
typedef struct s_ll_node {
	struct s_ll_node* prev;
	struct s_ll_node* next;
} t_ll_node;

/* Initialize a node (detached state). */
void ft_ll_init(t_ll_node* n);

/* Returns non-zero if the node is currently linked into a list. */
int ft_ll_is_linked(const t_ll_node* n);

/* Push node to the front of the list headed by *head. */
void ft_ll_push_front(t_ll_node** head, t_ll_node* n);

/* Push node to the back of the list headed by *head. */
void ft_ll_push_back(t_ll_node** head, t_ll_node* n);

/* Pop and return the front node (NULL if empty). */
t_ll_node* ft_ll_pop_front(t_ll_node** head);

/* Remove a specific node from the list (no-op if already detached). */
void ft_ll_remove(t_ll_node** head, t_ll_node* n);

/* Returns the number of nodes in a list */
size_t ft_ll_len(t_ll_node** head);

/* Simple iteration helper macro (safe against removal of 'it' during loop). */
#define FT_LL_FOR_EACH(it, head) for (t_ll_node* it = (head); it != NULL; it = it->next)

/* Variant that allows removing 'it' safely during iteration. */
#define FT_LL_FOR_EACH_SAFE(it, tmp, head)                                                         \
	for (t_ll_node* it = (head), *tmp = (it ? it->next : NULL); it != NULL;                        \
		 it = tmp, tmp = (it ? it->next : NULL))

/* Upcast a pointer to an embedded member back to its container type.
 * Example:
 *   typedef struct { t_ll_node link; int x; } ft_zone;
 *   ft_zone *z = FT_CONTAINER_OF(n, ft_zone, link);
 *
 * Notes:
 * - This is purely pointer arithmetic; no allocation involved.
 * - Use the CONST variant if your input pointer is const-qualified.
 */
#ifndef FT_CONTAINER_OF
#define FT_CONTAINER_OF(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))
#endif

#ifndef FT_CONTAINER_OF_CONST
#define FT_CONTAINER_OF_CONST(ptr, type, member)                                                   \
	((const type*)((const char*)(ptr)-offsetof(type, member)))
#endif

#endif /* FT_LINKED_LIST_H */
