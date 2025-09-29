/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 17:16:46 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 17:29:35 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "helpers.h"

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

size_t ft_align_up(size_t x, size_t align)
{
	if (align == 0)
		return x;
	size_t r = x % align;
	if (r == 0)
		return x;
	size_t add = align - r;
	// overflow guard
	if (x > SIZE_MAX - add)
		return SIZE_MAX;
	return x + add;
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
