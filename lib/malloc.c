/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: francisco <francisco@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/20 16:17:02 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 01:00:05 by francisco        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

/* Public API just forwards to heap. These must be exported symbols. */
void free(void* ptr)
{
	ft_heap_free(ptr);
}

void* malloc(size_t size)
{
	void* p = ft_heap_malloc(size);
	if (!p && size)
		errno = ENOMEM;
	return p;
}

void* realloc(void* ptr, size_t size)
{
	void* np = ft_heap_realloc(ptr, size);
	if (!np && size)
		errno = ENOMEM;
	return np;
}

void show_alloc_mem()
{
	// TODO!
}
