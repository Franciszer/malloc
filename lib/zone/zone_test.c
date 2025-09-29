/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_test.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 21:50:00 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 17:54:10 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zone/zone.h"
#include "helpers/helpers.h"
#include "munit.h"

#include <stdint.h>

#define ALIGN_OK(p, a) (((uintptr_t)(p)) % (a) == 0)

static MunitResult test_page_and_align(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	/* page size sanity */
	size_t ps = ft_page_size();
	munit_assert_size(ps, >=, 1024); /* very lax sanity check */

	/* header alignment value used by zone implementation */
	size_t hdr = ft_align_up(sizeof(t_zone), FT_ALIGN);
	munit_assert_size(hdr % FT_ALIGN, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_new_slab_and_layout(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	const size_t bsz = ft_align_up(64, FT_ALIGN); /* pick a small aligned bin */
	const size_t min_blocks = 5;

	t_zone* z = ft_zone_new(FT_Z_TINY, bsz, min_blocks);
	munit_assert_ptr_not_null(z);

	/* basic identity */
	munit_assert_int(z->klass, ==, FT_Z_TINY);
	munit_assert_size(z->bin_size, ==, bsz);
	munit_assert_ptr_not_null(z->mem_begin);
	munit_assert_ptr_not_null(z->mem_end);
	munit_assert_ptr_not_null(z->map_end);
	munit_assert_ptr_not_null(z->occ);

	/* alignment & ordering */
	munit_assert_true(ALIGN_OK(z->mem_begin, FT_ALIGN));
	munit_assert_true((uintptr_t)z->mem_begin < (uintptr_t)z->mem_end);
	munit_assert_true((uintptr_t)z->mem_end < (uintptr_t)z->map_end);
	munit_assert_true((uintptr_t)z->occ == (uintptr_t)z->mem_end);

	/* capacity math: how many complete (bsz + 1) units fit after header */
	const size_t hdr = ft_align_up(sizeof(t_zone), FT_ALIGN);
	const size_t mapped = ft_zone_mapped_bytes(z);
	const size_t raw_after_hdr = mapped - hdr;
	const size_t cap_calc = raw_after_hdr / (bsz + 1);

	munit_assert_size(z->capacity, ==, cap_calc);
	munit_assert_size(z->capacity, >=, min_blocks);
	munit_assert_size(z->free_count, ==, z->capacity);

	/* payload span equals capacity * bin_size */
	const size_t pay_bytes = (size_t)((uintptr_t)z->mem_end - (uintptr_t)z->mem_begin);
	munit_assert_size(pay_bytes, ==, z->capacity * bsz);

	ft_zone_destroy(z);
	return MUNIT_OK;
}

static MunitResult test_alloc_until_full_and_free_cycle(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	const size_t bsz = ft_align_up(32, FT_ALIGN);
	const size_t min_blocks = 8;

	t_zone* z = ft_zone_new(FT_Z_TINY, bsz, min_blocks);
	munit_assert_ptr_not_null(z);

	/* allocate all blocks */
	size_t cap = z->capacity;
	void** ptrs = (void**)malloc(cap * sizeof(void*));
	munit_assert_ptr_not_null(ptrs);

	for (size_t i = 0; i < cap; ++i) {
		void* p = ft_zone_alloc_block(z);
		munit_assert_ptr_not_null(p);

		/* in-range */
		munit_assert_true(ft_zone_contains(z, p));
		/* exact block starts and monotonic ascending by bsz (because we scan from i=0) */
		if (i > 0)
			munit_assert_size((uintptr_t)p - (uintptr_t)ptrs[i - 1], ==, bsz);

		ptrs[i] = p;
	}

	/* next allocation fails */
	munit_assert_ptr_null(ft_zone_alloc_block(z));
	munit_assert_size(z->free_count, ==, 0);

	/* free in odd order and reallocate */
	for (size_t i = 0; i < cap; i += 2) {
		ft_zone_free_block(z, ptrs[i]);
	}
	munit_assert_size(z->free_count, ==, (cap + 1) / 2);

	for (size_t k = 0; k < (cap + 1) / 2; ++k) {
		void* p = ft_zone_alloc_block(z);
		munit_assert_ptr_not_null(p);
		munit_assert_true(ft_zone_contains(z, p));
	}
	munit_assert_ptr_null(ft_zone_alloc_block(z));
	munit_assert_size(z->free_count, ==, 0);

	/* free all */
	for (size_t i = 0; i < cap; ++i) {
		ft_zone_free_block(z, ptrs[i]);
	}
	munit_assert_size(z->free_count, ==, cap);

	free(ptrs);
	ft_zone_destroy(z);
	return MUNIT_OK;
}

static MunitResult test_contains_and_bounds(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	const size_t bsz = ft_align_up(48, FT_ALIGN);
	t_zone* z = ft_zone_new(FT_Z_SMALL, bsz, 4);
	munit_assert_ptr_not_null(z);

	/* begin included, end excluded */
	munit_assert_true(ft_zone_contains(z, z->mem_begin));
	munit_assert_false(ft_zone_contains(z, z->mem_end));

	/* middle inside */
	uintptr_t mid = (uintptr_t)z->mem_begin + (z->bin_size / 2);
	munit_assert_true(ft_zone_contains(z, (void*)mid));

	/* outside */
	munit_assert_false(ft_zone_contains(z, (void*)((uintptr_t)z->mem_end + 16)));

	ft_zone_destroy(z);
	return MUNIT_OK;
}

static MunitResult test_large_zone_layout(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	const size_t req = 777; /* odd on purpose to check alignment */
	t_zone* z = ft_zone_new(FT_Z_LARGE, req, 1);
	munit_assert_ptr_not_null(z);

	munit_assert_int(z->klass, ==, FT_Z_LARGE);
	munit_assert_size(z->capacity, ==, 1);
	munit_assert_size(z->free_count, ==, 0);
	munit_assert_ptr_null(z->occ);

	/* payload size equals aligned(req) */
	const size_t need = ft_align_up(req, FT_ALIGN);
	munit_assert_size((size_t)((uintptr_t)z->mem_end - (uintptr_t)z->mem_begin), ==, need);

	/* alloc API is undefined for LARGE: should return NULL, not crash */
	munit_assert_ptr_null(ft_zone_alloc_block(z));

	/* contains begin .. end-1 */
	munit_assert_true(ft_zone_contains(z, z->mem_begin));
	munit_assert_false(ft_zone_contains(z, z->mem_end));

	ft_zone_destroy(z);
	return MUNIT_OK;
}

static MunitResult test_min_blocks_zero_is_clamped(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	const size_t bsz = ft_align_up(16, FT_ALIGN);
	t_zone* z = ft_zone_new(FT_Z_TINY, bsz, 0); /* 0 → treated as 1 */
	munit_assert_ptr_not_null(z);

	munit_assert_size(z->capacity, >=, 1);
	munit_assert_size(z->free_count, ==, z->capacity);

	ft_zone_destroy(z);
	return MUNIT_OK;
}

static MunitResult test_free_ignores_outside_ptrs(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	const size_t bsz = ft_align_up(64, FT_ALIGN);
	t_zone* z = ft_zone_new(FT_Z_SMALL, bsz, 4);
	munit_assert_ptr_not_null(z);

	size_t before = z->free_count;
	/* outside pointer should be ignored (no crash, no counter change) */
	int dummy;
	ft_zone_free_block(z, &dummy);
	munit_assert_size(z->free_count, ==, before);

	/* inside but not a block start is "undefined" by API; we don't rely on behavior.
	   We only verify no crash by NOT calling it here. */

	ft_zone_destroy(z);
	return MUNIT_OK;
}

/* capture stdout into heap buffer; returns malloc'd string the test must free */
static char* cap_stdout(void (*fn)(void*), void* arg)
{
	int pipefd[2];
	pipe(pipefd);
	int saved = dup(1);
	dup2(pipefd[1], 1);
	close(pipefd[1]);

	fn(arg);

	/* close write end (fd=1) to signal EOF to reader */
	close(1);
	dup2(saved, 1);
	close(saved);

	/* slurp */
	char* out = NULL;
	size_t cap = 0, len = 0;
	char buf[512];
	ssize_t n;
	while ((n = read(pipefd[0], buf, sizeof buf)) > 0) {
		if (len + (size_t)n + 1 > cap) {
			cap = (cap ? cap * 2 : 1024);
			out = (char*)realloc(out, cap);
		}
		memcpy(out + len, buf, (size_t)n);
		len += (size_t)n;
	}
	if (!out) {
		out = (char*)malloc(1);
		len = 0;
	}
	out[len] = '\0';
	close(pipefd[0]);
	return out;
}

/* wrappers so we can match cap_stdout signature */
static void print_zone_blocks(void* arg)
{
	ft_zone_print_blocks((const t_zone*)arg);
}

static MunitResult test_zone_print_slab(const MunitParameter params[], void* user_data)
{
	/* tiny slab of bin=32, min_blocks=4 */
	t_zone* z = ft_zone_new(FT_Z_TINY, 32, 4);
	munit_assert_not_null(z);

	/* allocate two blocks */
	void* a = ft_zone_alloc_block(z);
	void* b = ft_zone_alloc_block(z);
	munit_assert_not_null(a);
	munit_assert_not_null(b);

	char* out = cap_stdout(print_zone_blocks, z);
	/* expect two lines and "32 bytes" twice */
	munit_assert_ptr_not_null(strstr(out, "32 bytes"));
	/* very loose check: two occurrences */
	const char* p = out;
	int cnt = 0;
	while ((p = strstr(p, "32 bytes"))) {
		cnt++;
		p += 2;
	}
	munit_assert_int(cnt, >=, 2);

	free(out);
	ft_zone_destroy(z);
	return MUNIT_OK;
}

static MunitResult test_zone_print_large(const MunitParameter params[], void* user_data)
{
	t_zone* z = ft_zone_new(FT_Z_LARGE, 5008, 1);
	munit_assert_not_null(z);
	char* out = cap_stdout(print_zone_blocks, z);
	munit_assert_ptr_not_null(strstr(out, "5008 bytes"));
	free(out);
	ft_zone_destroy(z);
	return MUNIT_OK;
}

/* ---------- test registry ---------- */

static MunitTest tests[] = {
	{"/zone/page_and_align", test_page_and_align, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/zone/new_slab_and_layout",
	 test_new_slab_and_layout,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone/alloc_until_full_and_free",
	 test_alloc_until_full_and_free_cycle,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone/contains_and_bounds",
	 test_contains_and_bounds,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone/large_zone_layout", test_large_zone_layout, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/zone/min_blocks_zero",
	 test_min_blocks_zero_is_clamped,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone/free_ignores_outside_ptrs",
	 test_free_ignores_outside_ptrs,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone/show/slab", test_zone_print_slab, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/zone/show/large", test_zone_print_large, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite suite = {"/zone",
								 tests,
								 NULL,
								 1, /* iterations */
								 MUNIT_SUITE_OPTION_NONE};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
	return munit_suite_main(&suite, NULL, argc, argv);
}
