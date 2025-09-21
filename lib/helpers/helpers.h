/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 17:25:08 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/22 19:21:47 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HELPERS_H
#define HELPERS_H

#include <stddef.h> // size_t

/* Length of a fixed-size array (only works when the expression is an array,
   not a pointer). */
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

/* Return the first element in an ascending-sorted size_t array that is >= key.
   Returns 0 if no such element exists or len == 0.
   (Assumes 0 is not a valid positive bin size; if it is, adapt the sentinel.)
*/
size_t lb_ge_size_t(const size_t* arr, size_t len, size_t key);

/* Convenience macro: captures the array length at the call site so you can write:
	 size_t v = LB_GE(BINS, n);
   NOTE: pass a real array, not a pointer (array-to-pointer decay would break ARRAY_LEN).
*/
#define LB_GE(arr, key) lb_ge_size_t((arr), ARRAY_LEN(arr), (key))

/* Byte-wise memcpy replacement.
   Copies n bytes from src to dst; returns dst.
   Overlap is **undefined** (just like memcpy). */
void* ft_memcpy(void* dst, const void* src, size_t n);

#endif /* HELPERS_H */
