/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: francisco <francisco@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/20 16:15:10 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/23 00:15:42 by francisco        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stddef.h>
#include "data_structures/linked_list.h"
#include <stdlib.h>
#include <errno.h>
#include "heap/heap.h"

void free(void* ptr);
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);

void show_alloc_mem();