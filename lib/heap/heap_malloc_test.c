#include "munit.h"
#include "heap/heap.h"
#include "zone/zone.h"
#include <stdint.h>
#include <string.h>

/* Helpers */
static void assert_aligned(void* p) {
    munit_assert_not_null(p);
    munit_assert_uint64((uintptr_t)p % FT_ALIGN, ==, 0);
}

/* 1) Basic alignment + ownership in TINY */
static MunitResult tiny_alignment_and_owner(const MunitParameter params[], void* data) {
    (void)params; (void)data;
    ft_heap_init(100, 100);

    void *p1 = ft_heap_malloc(1);
    void *p2 = ft_heap_malloc(15);
    void *p3 = ft_heap_malloc(16);

    assert_aligned(p1); assert_aligned(p2); assert_aligned(p3);

    ft_zone *z1 = ft_heap_find_owner(p1);
    ft_zone *z2 = ft_heap_find_owner(p2);
    ft_zone *z3 = ft_heap_find_owner(p3);

    munit_assert_not_null(z1); munit_assert_not_null(z2); munit_assert_not_null(z3);
    munit_assert_int(z1->klass, ==, FT_Z_TINY);
    munit_assert_true(ft_zone_contains(z1, p1));
    munit_assert_true(ft_zone_contains(z2, p2));
    munit_assert_true(ft_zone_contains(z3, p3));

    ft_heap_destroy();
    return MUNIT_OK;
}

/* 2) Class boundaries: SMALL max is slab; > SMALL_MAX is LARGE */
static MunitResult small_vs_large_selection(const MunitParameter params[], void* data) {
    (void)params; (void)data;
    ft_heap_init(64, 64);

    void *ps  = ft_heap_malloc(SMALL_MAX);     // slab
    void *pl  = ft_heap_malloc(SMALL_MAX + 1); // large

    assert_aligned(ps); assert_aligned(pl);

    ft_zone *zs = ft_heap_find_owner(ps);
    ft_zone *zl = ft_heap_find_owner(pl);

    munit_assert_not_null(zs); munit_assert_not_null(zl);
    munit_assert_int(zs->klass, !=, FT_Z_LARGE);
    munit_assert_int(zl->klass, ==, FT_Z_LARGE);
    munit_assert_true(pl >= zl->mem_begin && pl < zl->mem_end);

    ft_heap_destroy();
    return MUNIT_OK;
}

/* 3) Bin roundup: request rounds to first >= bin */
static MunitResult bin_roundup_behavior(const MunitParameter params[], void* data) {
    (void)params; (void)data;
    ft_heap_init(100, 100);

    void *p32  = ft_heap_malloc(17);   // expect 32-bin
    void *p128 = ft_heap_malloc(100);  // expect 128-bin

    ft_zone *z32  = ft_heap_find_owner(p32);
    ft_zone *z128 = ft_heap_find_owner(p128);

    munit_assert_not_null(z32); munit_assert_not_null(z128);
    munit_assert_size(z32->bin_size,  ==, 32);
    munit_assert_size(z128->bin_size, ==, 128);

    ft_heap_destroy();
    return MUNIT_OK;
}

/* 4) Aligned size near thresholds still stays slab (no accidental large) */
static MunitResult align_then_classify_edges(const MunitParameter params[], void* data) {
    (void)params; (void)data;
    ft_heap_init(100, 100);

    size_t s1 = SMALL_MAX - (FT_ALIGN - 1); // aligns up to SMALL_MAX
    void *p1 = ft_heap_malloc(s1);
    void *p2 = ft_heap_malloc(SMALL_MAX);    // exact

    ft_zone *z1 = ft_heap_find_owner(p1);
    ft_zone *z2 = ft_heap_find_owner(p2);

    munit_assert_not_null(z1); munit_assert_not_null(z2);
    munit_assert_int(z1->klass, !=, FT_Z_LARGE);
    munit_assert_int(z2->klass, !=, FT_Z_LARGE);

    ft_heap_destroy();
    return MUNIT_OK;
}

/* 5) Multiple allocations trigger additional zones for a big bin */
static MunitResult growth_creates_more_zones(const MunitParameter params[], void* data) {
    (void)params; (void)data;

    // Use SMALL bin near page size so capacity per zone is ~1
    ft_heap_init(1, 1);

    // request that lands in 4096 bin (adjust if your SMALL bins differ)
    size_t req = 3500;
    void *a = ft_heap_malloc(req);
    void *b = ft_heap_malloc(req);  // likely forces a second zone
    assert_aligned(a); assert_aligned(b);

    ft_zone *za = ft_heap_find_owner(a);
    ft_zone *zb = ft_heap_find_owner(b);
    munit_assert_not_null(za); munit_assert_not_null(zb);

    // They may be different zones; if capacity is 1, they MUST be different
    munit_assert_size(za->bin_size, ==, zb->bin_size);
    if (za->capacity == 1) {
        munit_assert_ptr_not_equal(za, zb);
    }

    ft_heap_destroy();
    return MUNIT_OK;
}

/* 6) Many small allocations: pointers unique, all aligned, all in-slab */
static MunitResult stress_many_tiny(const MunitParameter params[], void* data) {
    (void)params; (void)data;
    ft_heap_init(100, 100);

    enum { N = 512 };
    void *ptrs[N] = {0};

    for (int i = 0; i < N; ++i) {
        ptrs[i] = ft_heap_malloc((i % 15) + 1); // 1..15
        assert_aligned(ptrs[i]);
        ft_zone *z = ft_heap_find_owner(ptrs[i]);
        munit_assert_not_null(z);
        munit_assert_int(z->klass, ==, FT_Z_TINY);
        // uniqueness vs earlier pointers (cheap O(n^2) is fine for N=512)
        for (int j = 0; j < i; ++j) {
            munit_assert_ptr_not_equal(ptrs[i], ptrs[j]);
        }
    }

    ft_heap_destroy();
    return MUNIT_OK;
}

/* --- munit boilerplate --- */

static MunitTest heap_malloc_tests[] = {
    { (char*)"/heap/malloc/tiny_alignment_and_owner",    tiny_alignment_and_owner,    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*)"/heap/malloc/small_vs_large_selection",    small_vs_large_selection,    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*)"/heap/malloc/bin_roundup_behavior",        bin_roundup_behavior,        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*)"/heap/malloc/align_then_classify_edges",   align_then_classify_edges,   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*)"/heap/malloc/growth_creates_more_zones",   growth_creates_more_zones,   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*)"/heap/malloc/stress_many_tiny",            stress_many_tiny,            NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite suite = {
    (char*)"",
    heap_malloc_tests,
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char* argv[]) {
    return munit_suite_main(&suite, NULL, argc, argv);
}
