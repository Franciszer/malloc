/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 17:16:46 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/23 19:45:23 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "helpers.h"

/* Binary search lower_bound for size_t, works for any ascending-sorted array.
   Finds the *first* index i in [0, len] with arr[i] >= key (if any).
   Returns arr[i] (the value), or 0 if not found.
*/
size_t lb_ge_size_t(const size_t* arr, size_t len, size_t key)
{
	if (!arr || len == 0)
		return 0;

	size_t lo = 0;
	size_t hi = len; /* search in the half-open interval [lo, hi) */

	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2; /* no bit shifts; generic and safe */
		if (arr[mid] < key) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	}

	return (lo < len) ? arr[lo] : 0;
}

void* ft_memcpy(void* dst, const void* src, size_t n)
{
	/* memcpy semantics: if n==0, OK even if pointers are NULL.
	   If regions overlap and n>0, behavior is undefined (we just copy forward). */
	if (dst == src || n == 0)
		return dst;

	unsigned char* d = (unsigned char*)dst;
	const unsigned char* s = (const unsigned char*)src;

	for (size_t i = 0; i < n; ++i)
		d[i] = s[i];

	return dst;
}

/* ---------------- alignment & pagesize ---------------- */

size_t ft_align_up(size_t n, size_t a)
{
	return (n + (a - 1)) & ~(a - 1);
}

size_t ft_page_size(void)
{
#ifdef __APPLE__
	/* Subject allows getpagesize() on macOS */
	return (size_t)getpagesize();
#else
	/* Subject allows sysconf(_SC_PAGESIZE) on Linux */
	long ps = sysconf(_SC_PAGESIZE);
	return (ps > 0) ? (size_t)ps : 4096u;
#endif
}

