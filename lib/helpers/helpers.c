/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 17:16:46 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/25 16:41:53 by frthierr         ###   ########.fr       */
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

void ft_putc(char c)
{
	(void)!write(1, &c, 1);
}

void ft_putstr(const char* s)
{
	if (!s)
		return;
	const char* p = s;
	while (*p)
		p++;
	(void)!write(1, s, (size_t)(p - s));
}

void ft_putusize(size_t v)
{
	char buf[32];
	size_t i = 0;
	if (v == 0) {
		ft_putc('0');
		return;
	}
	while (v) {
		buf[i++] = (char)('0' + (v % 10));
		v /= 10;
	}
	while (i--)
		ft_putc(buf[i]);
}

void ft_puthex_ptr(const void* p)
{
	static const char hexd[] = "0123456789abcdef";
	uintptr_t x = (uintptr_t)p;
	ft_putstr("0x");
	/* print without leading zeros but at least one nibble */
	int started = 0;
	for (int sh = (int)(sizeof(uintptr_t) * 8 - 4); sh >= 0; sh -= 4) {
		unsigned nib = (unsigned)((x >> sh) & 0xF);
		if (!started) {
			if (nib == 0 && sh > 0)
				continue;
			started = 1;
		}
		ft_putc(hexd[nib]);
	}
	if (!started)
		ft_putc('0');
}
