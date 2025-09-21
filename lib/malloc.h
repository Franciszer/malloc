/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/20 16:15:10 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/20 16:16:41 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);

void show_alloc_mem();