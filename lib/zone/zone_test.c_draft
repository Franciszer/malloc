/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_test.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 19:17:57 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/21 19:18:41 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "munit.h"
#include <stdint.h>
#include <string.h>
#include "zone.h"

/* Small helpers */
static int is_aligned(const void* p, size_t a)
{
	return ((uintptr_t)p % a) == 0;
}
static int is_block_aligned_in_zone(const ft_zone* z, const void* p)
{
	if (!ft_zone_contains(z, p))
		return 0;
	return (((uintptr_t)p - (uintptr_t)z->mem_begin) % z->bin_size) == 0;
}

/* --- Test: ft_page_size and ft_align_up basics --- */
static MunitResult test_page_and_align(const MunitParameter params[], void* ud)
{
	(void)params;
	(void)ud;

	size_t ps = ft_page_size();
	munit_assert(ps > 0);

	/* ft_align_up: 16-aligned, 32-aligned sanity checks */
	munit_assert_uint((unsigned)(ft_align_up(0, 16)), ==, 0u);
	munit_assert_uint((unsigned)(ft_align_up(1, 16)), ==, 16u);
	munit_assert_uint((unsigned)(ft_align_up(16, 16)), ==, 16u);
	munit_assert_uint((unsigned)(ft_align_up(17, 16)), ==, 32u);
	munit_assert_uint((unsigned)(ft_align_up(31, 16)), ==, 32u);
	munit_assert_uint((unsigned)(ft_align_up(32, 32)), ==, 32u);
	munit_assert_uint((unsigned)(ft_align_up(33, 32)), ==, 64u);

	return MUNIT_OK;
}

/* --- Test: create a slab zone; capacity/alignment/push-pop behavior --- */
static MunitResult test_new_slab_and_blocks(const MunitParameter params[], void* ud)
{
	(void)params;
	(void)ud;

	const size_t req_bin = 24;	   /* will be aligned to FT_ALIGN (16) -> 32 */
	const size_t min_blocks = 128; /* subject: you'll use >=100 in production */
	ft_zone* z = ft_zone_new_slab(FT_Z_TINY, req_bin, min_blocks);
	munit_assert_ptr_not_null(z);

	/* Basic header fields */
	munit_assert_int(z->klass, ==, FT_Z_TINY);
	munit_assert_uint((unsigned)z->bin_size, >=, (unsigned)req_bin);
	munit_assert_uint((unsigned)(z->bin_size % FT_ALIGN), ==, 0u);

	/* Alignment and mapping */
	munit_assert_true(is_aligned(z->mem_begin, FT_ALIGN));
	munit_assert_ptr(z->mem_end, >, z->mem_begin);
	munit_assert_uint((unsigned)(ft_zone_mapped_bytes(z) % ft_page_size()), ==, 0u);

	/* Capacity & free count */
	munit_assert_size(z->capacity, >=, min_blocks);
	munit_assert_size(z->free_count, ==, z->capacity);

	/* Pop all blocks; verify alignment and uniqueness; then zone is empty */
	size_t cap = z->capacity;
	void** seen = (void**)munit_malloc(sizeof(void*) * cap);
	for (size_t i = 0; i < cap; ++i) {
		void* p = ft_zone_pop_block(z);
		munit_assert_ptr_not_null(p);
		munit_assert_true(ft_zone_contains(z, p));
		munit_assert_true(is_aligned(p, FT_ALIGN));
		munit_assert_true(is_block_aligned_in_zone(z, p));
		seen[i] = p;
	}
	/* Next pop must fail; free_count must be zero */
	munit_assert_ptr_null(ft_zone_pop_block(z));
	munit_assert_size(z->free_count, ==, 0);

	/* Push them all back; free_count returns to capacity */
	for (size_t i = 0; i < cap; ++i) {
		ft_zone_push_block(z, seen[i]);
	}
	munit_assert_size(z->free_count, ==, z->capacity);

	ft_zone_destroy(z);
	return MUNIT_OK;
}

/* --- Test: LIFO characteristic of the free list (push/pop ordering) --- */
static MunitResult test_free_list_lifo(const MunitParameter params[], void* ud)
{
	(void)params;
	(void)ud;

	ft_zone* z = ft_zone_new_slab(FT_Z_SMALL, 64, 8); /* small capacity is fine for test */
	munit_assert_ptr_not_null(z);

	/* Take two blocks */
	void* a = ft_zone_pop_block(z);
	void* b = ft_zone_pop_block(z);
	munit_assert_ptr_not_null(a);
	munit_assert_ptr_not_null(b);
	munit_assert_ptr_not_equal(a, b);

	/* Push back in order: first A then B -> stack head should be B */
	ft_zone_push_block(z, a);
	ft_zone_push_block(z, b);

	void* p = ft_zone_pop_block(z);
	munit_assert_ptr_equal(p, b);

	p = ft_zone_pop_block(z);
	munit_assert_ptr_equal(p, a);

	ft_zone_destroy(z);
	return MUNIT_OK;
}

/* --- Test: ft_zone_contains boundaries and block alignment checks --- */
static MunitResult test_contains_and_bounds(const MunitParameter params[], void* ud)
{
	(void)params;
	(void)ud;

	ft_zone* z = ft_zone_new_slab(FT_Z_TINY, 32, 100);
	munit_assert_ptr_not_null(z);

	char* begin = (char*)z->mem_begin;
	char* end = (char*)z->mem_end;

	/* Boundaries: begin is in; end is NOT (one-past) */
	munit_assert_true(ft_zone_contains(z, begin));
	munit_assert_false(ft_zone_contains(z, end));
	if (begin > (char*)0) {
		munit_assert_false(ft_zone_contains(z, begin - 1));
	}
	if (end > begin) {
		munit_assert_true(ft_zone_contains(z, end - 1));
	}

	/* Pop a block and verify alignment relative to mem_begin/bin_size */
	void* p = ft_zone_pop_block(z);
	munit_assert_ptr_not_null(p);
	munit_assert_true(is_block_aligned_in_zone(z, p));

	ft_zone_destroy(z);
	return MUNIT_OK;
}

/* --- Test: large zone properties --- */
static MunitResult test_large_zone(const MunitParameter params[], void* ud)
{
	(void)params;
	(void)ud;

	const size_t req = 12345;
	const size_t need = ft_align_up(req, FT_ALIGN);

	ft_zone* z = ft_zone_new_large(req);
	munit_assert_ptr_not_null(z);

	munit_assert_int(z->klass, ==, FT_Z_LARGE);
	munit_assert_size(z->capacity, ==, 1);
	munit_assert_size(z->free_count, ==, 0);
	munit_assert_size(z->bin_size, ==, need);

	/* pop/push don't apply to LARGE; pop must be NULL */
	munit_assert_ptr_null(ft_zone_pop_block(z));

	/* Payload alignment and size range */
	munit_assert_true(is_aligned(z->mem_begin, FT_ALIGN));
	munit_assert_ptr(z->mem_end, >, z->mem_begin);
	munit_assert_uint((unsigned)(ft_zone_mapped_bytes(z) % ft_page_size()), ==, 0u);

	/* Contains: [mem_begin, mem_begin+need) is inside; end is outside */
	munit_assert_true(ft_zone_contains(z, z->mem_begin));
	munit_assert_true(ft_zone_contains(z, (char*)z->mem_begin + need - 1));
	munit_assert_false(ft_zone_contains(z, (char*)z->mem_begin + need));

	ft_zone_destroy(z);
	return MUNIT_OK;
}

/* --- Assemble suite --- */
static MunitTest tests[] = {
	{"/page_and_align", test_page_and_align, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/new_slab_and_blocks", test_new_slab_and_blocks, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/free_list_lifo", test_free_list_lifo, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/contains_and_bounds", test_contains_and_bounds, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/large_zone", test_large_zone, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite suite = {"/zone", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char* argv[])
{
	return munit_suite_main(&suite, NULL, argc, argv);
}
