/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: francisco <francisco@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/20 16:17:02 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/23 00:17:29 by francisco        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

/* Initialize on library load; pick ≥100 blocks to satisfy the subject. */
__attribute__((constructor))
static void ft_malloc_ctor(void) {
    ft_heap_init(TINY_MAX, SMALL_MAX);
}

/* Tear down on unload. */
__attribute__((destructor))
static void ft_malloc_dtor(void) {
    ft_heap_destroy();
}

/* Public API just forwards to heap. These must be exported symbols. */
void free(void* ptr) {
    ft_heap_free(ptr);
}

void* malloc(size_t size) {
    void* p = ft_heap_malloc(size);
    if (!p && size) errno = ENOMEM;
    return p;
}

void* realloc(void* ptr, size_t size) {
    void* np = ft_heap_realloc(ptr, size);
    if (!np && size) errno = ENOMEM;
    return np;
}
