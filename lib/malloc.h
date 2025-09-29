/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: francisco <francisco@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/20 16:15:10 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 02:39:39 by francisco        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H

#define FT_MALLOC_H

#include <stddef.h>
#include "heap/heap.h"
#include <errno.h>

// exposes these functions as shared_lib
#define FT_API __attribute__((visibility("default")))

FT_API void free(void* ptr);
FT_API void* malloc(size_t size);
FT_API void* realloc(void* ptr, size_t size);
FT_API void show_alloc_mem();

#endif