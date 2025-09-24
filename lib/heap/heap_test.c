/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_integration_test.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tests <>                                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24                                    by tests           */
/*   Updated: 2025/09/24                                    by tests           */
/*                                                                            */
/* ************************************************************************** */

#include <stddef.h>
#include <stdint.h>
#include <string.h> /* only for memcmp in tests (your lib uses ft_memcpy) */
#include "munit.h"

#include "heap/heap.h"
#include "zone/zone.h"
#include "zone/zone_list.h"
#include "helpers/helpers.h"

/* Per-test setup/teardown: keep zones small to finish quickly */
static void* setup(const MunitParameter params[], void* user_data)
{
	(void)params;
	(void)user_data;
	/* tiny_min_blocks, small_min_blocks */
	ft_heap_init(/*tiny*/ 8, /*small*/ 4);
	return NULL;
}

static void teardown(void* fixture)
{
	(void)fixture;
	ft_heap_destroy();
}

/* ---------- helpers for tests ---------- */

static inline int is_aligned(const void* p)
{
	return (((uintptr_t)p) % FT_ALIGN) == 0;
}

static t_zone* first_zone_of(t_zone_class k)
{
	t_ll_node* head = g_heap.zls[k];
	if (!head)
		return NULL;
	return FT_CONTAINER_OF(head, t_zone, link);
}

/* ---------- tests ---------- */

static MunitResult tiny_alloc_free_basic(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	void* a = ft_heap_malloc(1);
	void* b = ft_heap_malloc(16);
	void* c = ft_heap_malloc(128); /* still TINY_MAX */

	munit_assert_ptr_not_null(a);
	munit_assert_ptr_not_null(b);
	munit_assert_ptr_not_null(c);

	munit_assert_true(is_aligned(a));
	munit_assert_true(is_aligned(b));
	munit_assert_true(is_aligned(c));

	/* exactly one TINY zone should exist */
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 1);

	t_zone* z = first_zone_of(FT_Z_TINY);
	munit_assert_not_null(z);
	/* We allocated 3 blocks */
	munit_assert_size(z->free_count, ==, z->capacity - 3);

	/* Free middle, then alloc again → should reuse same slot (first-free) */
	ft_heap_free(b);
	munit_assert_size(z->free_count, ==, z->capacity - 2);
	void* b2 = ft_heap_malloc(8);
	munit_assert_ptr_not_null(b2);
	munit_assert_ptr_equal(b2, b);

	/* Clean up remaining */
	ft_heap_free(a);
	ft_heap_free(b2);
	ft_heap_free(c);

	/* Zone should trim when fully free (your code does this) */
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);

	return MUNIT_OK;
}

static MunitResult small_alloc_free_basic(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	/* A SMALL request (e.g., 2000) */
	void* p = ft_heap_malloc(2000);
	munit_assert_ptr_not_null(p);
	munit_assert_true(is_aligned(p));
	munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 1);

	t_zone* z = first_zone_of(FT_Z_SMALL);
	munit_assert_not_null(z);
	munit_assert_size(z->bin_size, ==, SMALL_MAX);
	munit_assert_size(z->free_count, ==, z->capacity - 1);

	ft_heap_free(p);
	/* SMALL zone trims on fully free */
	munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 0);

	return MUNIT_OK;
}

static MunitResult large_alloc_free_basic(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	size_t big = SMALL_MAX + 1; /* force LARGE */
	void* p = ft_heap_malloc(big);
	munit_assert_ptr_not_null(p);
	munit_assert_true(is_aligned(p));
	munit_assert_size(ft_heap_zone_count(FT_Z_LARGE), ==, 1);

	ft_heap_free(p);
	munit_assert_size(ft_heap_zone_count(FT_Z_LARGE), ==, 0);

	return MUNIT_OK;
}

static MunitResult malloc_zero_behaves_like_one(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	void* p = ft_heap_malloc(0);
	munit_assert_ptr_not_null(p);
	munit_assert_true(is_aligned(p));
	/* Should be TINY by policy (0→1) */
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 1);

	ft_heap_free(p);
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);

	return MUNIT_OK;
}

static MunitResult find_owner_works(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	void* a = ft_heap_malloc(64);
	void* b = ft_heap_malloc(120);

	munit_assert_not_null(a);
	munit_assert_not_null(b);

	t_zone* za = ft_heap_find_owner(a);
	t_zone* zb = ft_heap_find_owner(b);
	munit_assert_not_null(za);
	munit_assert_not_null(zb);
	munit_assert_ptr_equal(za, zb); /* both in same TINY zone (single bin) */

	/* Non-owned pointer should return NULL */
	t_zone* zfake = ft_heap_find_owner((const void*)0x12345);
	munit_assert_null(zfake);

	ft_heap_free(a);
	ft_heap_free(b);
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);

	return MUNIT_OK;
}

static MunitResult realloc_within_slab_keeps_ptr(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	/* TINY block */
	char* p = (char*)ft_heap_malloc(100);
	munit_assert_not_null(p);
	memset(p, 0xAA, 100);

	/* Grow but remain ≤ bin_size (128) */
	char* q = (char*)ft_heap_realloc(p, 120);
	munit_assert_ptr_equal(q, p);

	/* Shrink also stays same */
	char* r = (char*)ft_heap_realloc(q, 64);
	munit_assert_ptr_equal(r, p);

	ft_heap_free(r);
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);

	return MUNIT_OK;
}

static MunitResult realloc_tiny_to_small_moves_and_copies(const MunitParameter params[],
														  void* fixture)
{
	(void)params;
	(void)fixture;

	/* TINY block */
	uint8_t* p = (uint8_t*)ft_heap_malloc(100);
	munit_assert_not_null(p);
	for (size_t i = 0; i < 100; ++i)
		p[i] = (uint8_t)i;

	/* Grow beyond 128 → SMALL */
	uint8_t* q = (uint8_t*)ft_heap_realloc(p, 512);
	munit_assert_ptr_not_equal(q, p);
	munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 1);

	/* First 100 bytes preserved */
	munit_assert_int(memcmp(q, (uint8_t[]){0}, 0), ==, 0); /* dummy to ensure include */
	for (size_t i = 0; i < 100; ++i) {
		munit_assert_uint8(q[i], ==, (uint8_t)i);
	}

	ft_heap_free(q);
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);
	munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 0);

	return MUNIT_OK;
}

static MunitResult realloc_large_shrink_and_grow(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	size_t big = SMALL_MAX + 256; /* LARGE */
	uint8_t* p = (uint8_t*)ft_heap_malloc(big);
	munit_assert_not_null(p);
	for (size_t i = 0; i < 256; ++i)
		p[i] = (uint8_t)(0xF0 | (i & 0x0F));

	/* Shrink (stay LARGE) → same pointer */
	uint8_t* q = (uint8_t*)ft_heap_realloc(p, big / 2);
	munit_assert_ptr_equal(q, p);

	/* Grow → new mapping */
	uint8_t* r = (uint8_t*)ft_heap_realloc(q, big * 2);
	munit_assert_ptr_not_equal(r, q);

	/* Data preserved up to old size */
	for (size_t i = 0; i < 256; ++i) {
		munit_assert_uint8(r[i], ==, (uint8_t)(0xF0 | (i & 0x0F)));
	}

	ft_heap_free(r);
	munit_assert_size(ft_heap_zone_count(FT_Z_LARGE), ==, 0);

	return MUNIT_OK;
}

static MunitResult free_invalid_is_ignored(const MunitParameter params[], void* fixture)
{
	(void)params;
	(void)fixture;

	void* p = ft_heap_malloc(64);
	munit_assert_not_null(p);
	size_t tiny_before = ft_heap_zone_count(FT_Z_TINY);

	/* Invalid free: should not crash or modify the heap */
	ft_heap_free((void*)0x12345678);

	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, tiny_before);

	ft_heap_free(p);
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);

	return MUNIT_OK;
}

/* ---------- suite ---------- */

static MunitTest tests[] = {
	{"/tiny_alloc_free_basic",
	 tiny_alloc_free_basic,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/small_alloc_free_basic",
	 small_alloc_free_basic,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/large_alloc_free_basic",
	 large_alloc_free_basic,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/malloc_zero_behaves_like_one",
	 malloc_zero_behaves_like_one,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/find_owner_works", find_owner_works, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/realloc_within_slab_keeps_ptr",
	 realloc_within_slab_keeps_ptr,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/realloc_tiny_to_small_moves_and_copies",
	 realloc_tiny_to_small_moves_and_copies,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/realloc_large_shrink_and_grow",
	 realloc_large_shrink_and_grow,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/free_invalid_is_ignored",
	 free_invalid_is_ignored,
	 setup,
	 teardown,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite suite = {"/heap_integration", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char* argv[])
{
	return munit_suite_main(&suite, NULL, argc, argv);
}
