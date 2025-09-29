/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 17:25:08 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 17:05:37 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HELPERS_H
#define HELPERS_H

#include <stddef.h> // size_t
#include <unistd.h>
#include <inttypes.h>
/* Alignment for returned payloads and internal pointers (heap & zone agree) */
#define FT_ALIGN 16

/* Byte-wise memcpy replacement.
   Copies n bytes from src to dst; returns dst. */
void* ft_memcpy(void* dst, const void* src, size_t n);

size_t ft_page_size(void);
size_t ft_align_up(size_t n, size_t a);

/* printing helpers (stdout, no stdio) */
void ft_putc(char c);
void ft_putstr(const char* s);
void ft_putusize(size_t v);		   /* decimal */
void ft_puthex_ptr(const void* p); /* prints like 0x1a2b3c */

#endif /* HELPERS_H */
