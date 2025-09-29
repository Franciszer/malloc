/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_integration_test.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tests <>                                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24                                    by tests           */
/*   Updated: 2025/09/29                                    by tests           */
/*                                                                            */
/* ************************************************************************** */

#include <stdint.h>
#include <string.h>

#include "munit.h"

#include "heap/heap.h"
#include "zone/zone.h"
#include "helpers/helpers.h"

/* ---------- test setup/teardown ---------- */

static void* setup(const MunitParameter params[], void* user_data) {
    (void)params; (void)user_data;
    /* New API: arguments interpreted as BIN SIZES in your current code. */
    ft_heap_init(TINY_BIN_SIZE, SMALL_BIN_SIZE);
    return NULL;
}

static void teardown(void* fixture) {
    (void)fixture;
    ft_heap_destroy();
}

/* ---------- small local helpers ---------- */

static inline int is_aligned(const void* p) {
    return ((uintptr_t)p % FT_ALIGN) == 0;
}

static t_zone* first_zone_of(t_zone_class k) {
    t_ll_node* head = g_heap.zls[k];
    return head ? FT_CONTAINER_OF(head, t_zone, link) : NULL;
}

/* ---------- tests ---------- */

/* Three tiny allocs reduce free_count by 3; middle free is reused. */
static MunitResult tiny_alloc_reuse_and_trim(const MunitParameter[], void*) {
    void* a = ft_heap_malloc(1);
    void* b = ft_heap_malloc(16);
    void* c = ft_heap_malloc(TINY_BIN_SIZE); /* still tiny (== bin) */

    munit_assert_not_null(a);
    munit_assert_not_null(b);
    munit_assert_not_null(c);
    munit_assert_true(is_aligned(a));
    munit_assert_true(is_aligned(b));
    munit_assert_true(is_aligned(c));

    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 1);

    t_zone* z = first_zone_of(FT_Z_TINY);
    munit_assert_not_null(z);
    size_t free_before = z->free_count;
    (void)free_before; /* silence if not used by some compilers */

    /* We consumed 3 blocks total in this zone. */
    munit_assert_size(z->free_count + 3, ==, z->capacity);

    /* Free middle, then allocate again -> should reuse same slot */
    ft_heap_free(b);
    munit_assert_size(z->free_count + 2, ==, z->capacity);

    void* b2 = ft_heap_malloc(8);
    munit_assert_ptr_equal(b2, b);
    munit_assert_size(z->free_count + 3, ==, z->capacity);

    /* Free all; empty slab should be trimmed */
    ft_heap_free(a);
    ft_heap_free(b2);
    ft_heap_free(c);
    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);

    return MUNIT_OK;
}

/* A size just above tiny bin goes to SMALL, stays aligned, one SMALL zone. */
static MunitResult small_alloc_zone_lifetime(const MunitParameter[], void*) {
    size_t small_req = (size_t)TINY_BIN_SIZE + 1; /* force SMALL */
    void* p = ft_heap_malloc(small_req);
    munit_assert_not_null(p);
    munit_assert_true(is_aligned(p));
    munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 1);

    t_zone* z = first_zone_of(FT_Z_SMALL);
    munit_assert_not_null(z);
    munit_assert_size(z->bin_size, ==, SMALL_BIN_SIZE);
    munit_assert_size(z->free_count + 1, ==, z->capacity);

    ft_heap_free(p);
    /* slab fully free -> trimmed */
    munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 0);
    return MUNIT_OK;
}

/* Large allocations: one zone, shrink keeps ptr, grow moves & preserves data. */
static MunitResult large_alloc_shrink_grow(const MunitParameter[], void*) {
    size_t big = (size_t)SMALL_BIN_SIZE + 256; /* force LARGE */
    uint8_t* p = (uint8_t*)ft_heap_malloc(big);
    munit_assert_not_null(p);
    munit_assert_true(is_aligned(p));
    munit_assert_size(ft_heap_zone_count(FT_Z_LARGE), ==, 1);

    for (size_t i = 0; i < 256; ++i) p[i] = (uint8_t)(0xA0 | (i & 0x0F));

    /* Shrink: still LARGE -> same pointer */
    uint8_t* q = (uint8_t*)ft_heap_realloc(p, big / 2);
    munit_assert_ptr_equal(q, p);

    /* Grow: new mapping */
    uint8_t* r = (uint8_t*)ft_heap_realloc(q, big * 2);
    munit_assert_ptr_not_equal(r, q);

    for (size_t i = 0; i < 256; ++i)
        munit_assert_uint8(r[i], ==, (uint8_t)(0xA0 | (i & 0x0F)));

    ft_heap_free(r);
    munit_assert_size(ft_heap_zone_count(FT_Z_LARGE), ==, 0);
    return MUNIT_OK;
}

/* realloc within a slab keeps the same pointer; shrink/grow under bin limit. */
static MunitResult realloc_within_slab_keeps_ptr(const MunitParameter[], void*) {
    char* p = (char*)ft_heap_malloc(TINY_BIN_SIZE - 28); /* definitely tiny */
    munit_assert_not_null(p);

    char* q = (char*)ft_heap_realloc(p, TINY_BIN_SIZE - 8);  /* still ≤ bin */
    munit_assert_ptr_equal(q, p);

    char* r = (char*)ft_heap_realloc(q, TINY_BIN_SIZE);      /* exactly bin */
    munit_assert_ptr_equal(r, p);

    char* s = (char*)ft_heap_realloc(r, TINY_BIN_SIZE - 64); /* shrink */
    munit_assert_ptr_equal(s, p);

    ft_heap_free(s);
    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);
    return MUNIT_OK;
}

/* Crossing bins moves and copies; also creates a SMALL slab. */
static MunitResult realloc_tiny_to_small_moves_and_copies(const MunitParameter[], void*) {
    uint8_t* p = (uint8_t*)ft_heap_malloc(TINY_BIN_SIZE - 32);
    munit_assert_not_null(p);
    for (size_t i = 0; i < 128 && i < (size_t)(TINY_BIN_SIZE - 32); ++i) p[i] = (uint8_t)i;

    /* Force SMALL: request just over tiny bin. */
    uint8_t* q = (uint8_t*)ft_heap_realloc(p, (size_t)TINY_BIN_SIZE + 1);
    munit_assert_ptr_not_equal(q, p);
    munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 1);

    for (size_t i = 0; i < 128 && i < (size_t)(TINY_BIN_SIZE - 32); ++i)
        munit_assert_uint8(q[i], ==, (uint8_t)i);

    ft_heap_free(q);
    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);
    munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 0);
    return MUNIT_OK;
}

/* Two tiny allocations should have the same owner zone. */
static MunitResult find_owner_same_zone_for_tiny(const MunitParameter[], void*) {
    void* a = ft_heap_malloc(64);
    void* b = ft_heap_malloc(120); /* both ≤ TINY_BIN_SIZE */

    munit_assert_not_null(a);
    munit_assert_not_null(b);

    t_zone* za = ft_heap_find_owner(a);
    t_zone* zb = ft_heap_find_owner(b);
    munit_assert_not_null(za);
    munit_assert_not_null(zb);
    munit_assert_ptr_equal(za, zb);

    ft_heap_free(a);
    ft_heap_free(b);
    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);
    return MUNIT_OK;
}

/* realloc(p,0) frees and returns NULL (your current semantics). */
static MunitResult realloc_zero_frees(const MunitParameter[], void*) {
    void* p = ft_heap_malloc(32);
    munit_assert_not_null(p);
    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 1);

    void* np = ft_heap_realloc(p, 0);
    munit_assert_null(np);
    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);
    return MUNIT_OK;
}

/* Invalid free must be ignored (no crash, no state change). */
static MunitResult free_invalid_is_ignored(const MunitParameter[], void*) {
    void* p = ft_heap_malloc(64);
    munit_assert_not_null(p);
    size_t nz_before = ft_heap_zone_count(FT_Z_TINY);

    ft_heap_free((void*)0x12345678);

    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, nz_before);

    ft_heap_free(p);
    munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 0);
    return MUNIT_OK;
}

/* ft_heap_show_alloc_mem() returns total of: tiny/ small blocks as bin sizes, plus exact large sizes. */
static MunitResult show_alloc_mem_total_is_correct(const MunitParameter[], void*) {
    void* t1 = ft_heap_malloc(1);                   /* tiny => TINY_BIN_SIZE */
    void* t2 = ft_heap_malloc(TINY_BIN_SIZE - 8);   /* tiny => TINY_BIN_SIZE */
    void* s1 = ft_heap_malloc((size_t)TINY_BIN_SIZE + 1); /* small => SMALL_BIN_SIZE */
    size_t L  = (size_t)SMALL_BIN_SIZE + 512;       /* large exact */
    void* lg = ft_heap_malloc(L);

    munit_assert_not_null(t1);
    munit_assert_not_null(t2);
    munit_assert_not_null(s1);
    munit_assert_not_null(lg);

    size_t total = ft_heap_show_alloc_mem();
    size_t expected = (2 * (size_t)TINY_BIN_SIZE) + (1 * (size_t)SMALL_BIN_SIZE) + L;
    munit_assert_size(total, ==, expected);

    ft_heap_free(t1);
    ft_heap_free(t2);
    ft_heap_free(s1);
    ft_heap_free(lg);
    return MUNIT_OK;
}

/* ---------- suite ---------- */

static MunitTest tests[] = {
    {"/tiny_alloc_reuse_and_trim",           tiny_alloc_reuse_and_trim,           setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/small_alloc_zone_lifetime",           small_alloc_zone_lifetime,           setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/large_alloc_shrink_grow",             large_alloc_shrink_grow,             setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/realloc_within_slab_keeps_ptr",       realloc_within_slab_keeps_ptr,       setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/realloc_tiny_to_small_moves_and_copies", realloc_tiny_to_small_moves_and_copies, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/find_owner_same_zone_for_tiny",       find_owner_same_zone_for_tiny,       setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/realloc_zero_frees",                  realloc_zero_frees,                  setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/free_invalid_is_ignored",             free_invalid_is_ignored,             setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/show_alloc_mem_total_is_correct",     show_alloc_mem_total_is_correct,     setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

static const MunitSuite suite = { "/heap_integration", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };

int main(int argc, char* argv[]) {
    return munit_suite_main(&suite, NULL, argc, argv);
}
