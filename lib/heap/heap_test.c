/* lib/heap/heap_test.c */
#include <stdint.h>
#include <string.h>

#include "munit.h"

#include "heap/heap.h"
#include "zone/zone.h"
#include "helpers/helpers.h"

/* ---------- test setup/teardown ---------- */

static void* setup(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;
	/* Your API: args are BIN sizes. */
	ft_heap_init(TINY_BIN_SIZE, SMALL_BIN_SIZE);
	return NULL;
}

static void teardown(void* fixture)
{
	(void)fixture;
	ft_heap_destroy();
}

/* ---------- helpers ---------- */

static inline int is_aligned(const void* p) {
	return ((uintptr_t)p % FT_ALIGN) == 0;
}

static t_zone* first_zone_of(t_zone_class k) {
	t_ll_node* head = g_heap.zls[k];
	return head ? FT_CONTAINER_OF(head, t_zone, link) : NULL;
}

/* Accept either: (a) no zone (trimmed), or (b) one fully free slab. */
static void assert_class_empty_or_fully_free(t_zone_class k)
{
	size_t n = ft_heap_zone_count(k);
	if (n == 0) return;
	munit_assert_size(n, ==, 1);
	t_zone* z = first_zone_of(k);
	munit_assert_not_null(z);
	munit_assert_size(z->free_count, ==, z->capacity);
}

/* ---------- tests ---------- */

static MunitResult tiny_alloc_reuse_and_trim(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	void* a = ft_heap_malloc(1);
	void* b = ft_heap_malloc(16);
	void* c = ft_heap_malloc(TINY_BIN_SIZE); /* still tiny */

	munit_assert_not_null(a);
	munit_assert_not_null(b);
	munit_assert_not_null(c);
	munit_assert_true(is_aligned(a));
	munit_assert_true(is_aligned(b));
	munit_assert_true(is_aligned(c));

	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 1);
	t_zone* z = first_zone_of(FT_Z_TINY);
	munit_assert_not_null(z);

	/* Consumed 3 blocks in this tiny slab */
	munit_assert_size(z->free_count + 3, ==, z->capacity);

	/* Free middle; reuse the same slot */
	ft_heap_free(b);
	munit_assert_size(z->free_count + 2, ==, z->capacity);
	void* b2 = ft_heap_malloc(8);
	munit_assert_ptr_equal(b2, b);
	munit_assert_size(z->free_count + 3, ==, z->capacity);

	/* Free all: either trimmed or fully free remains */
	ft_heap_free(a);
	ft_heap_free(b2);
	ft_heap_free(c);
	assert_class_empty_or_fully_free(FT_Z_TINY);

	return MUNIT_OK;
}

static MunitResult small_alloc_zone_lifetime(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

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
	assert_class_empty_or_fully_free(FT_Z_SMALL);
	return MUNIT_OK;
}

static MunitResult large_alloc_shrink_grow(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	size_t big = (size_t)SMALL_BIN_SIZE + 256; /* LARGE */
	uint8_t* p = (uint8_t*)ft_heap_malloc(big);
	munit_assert_not_null(p);
	munit_assert_true(is_aligned(p));
	munit_assert_size(ft_heap_zone_count(FT_Z_LARGE), ==, 1);

	for (size_t i = 0; i < 256; ++i)
		p[i] = (uint8_t)(0xA0 | (i & 0x0F));

	/* Shrink stays LARGE → same pointer */
	uint8_t* q = (uint8_t*)ft_heap_realloc(p, big / 2);
	munit_assert_ptr_equal(q, p);

	/* Grow → new mapping */
	uint8_t* r = (uint8_t*)ft_heap_realloc(q, big * 2);
	munit_assert_ptr_not_equal(r, q);

	for (size_t i = 0; i < 256; ++i)
		munit_assert_uint8(r[i], ==, (uint8_t)(0xA0 | (i & 0x0F)));

	ft_heap_free(r);
	munit_assert_size(ft_heap_zone_count(FT_Z_LARGE), ==, 0);
	return MUNIT_OK;
}

static MunitResult realloc_within_slab_keeps_ptr(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	char* p = (char*)ft_heap_malloc(TINY_BIN_SIZE - 28); /* tiny */
	munit_assert_not_null(p);

	char* q = (char*)ft_heap_realloc(p, TINY_BIN_SIZE - 8); /* still ≤ bin */
	munit_assert_ptr_equal(q, p);

	char* r = (char*)ft_heap_realloc(q, TINY_BIN_SIZE); /* exactly bin */
	munit_assert_ptr_equal(r, p);

	char* s = (char*)ft_heap_realloc(r, TINY_BIN_SIZE - 64); /* shrink */
	munit_assert_ptr_equal(s, p);

	ft_heap_free(s);
	assert_class_empty_or_fully_free(FT_Z_TINY);
	return MUNIT_OK;
}

static MunitResult realloc_tiny_to_small_moves_and_copies(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	uint8_t* p = (uint8_t*)ft_heap_malloc(TINY_BIN_SIZE - 32);
	munit_assert_not_null(p);
	for (size_t i = 0; i < 128 && i < (size_t)(TINY_BIN_SIZE - 32); ++i)
		p[i] = (uint8_t)i;

	/* Force SMALL */
	uint8_t* q = (uint8_t*)ft_heap_realloc(p, (size_t)TINY_BIN_SIZE + 1);
	munit_assert_ptr_not_equal(q, p);
	munit_assert_size(ft_heap_zone_count(FT_Z_SMALL), ==, 1);

	for (size_t i = 0; i < 128 && i < (size_t)(TINY_BIN_SIZE - 32); ++i)
		munit_assert_uint8(q[i], ==, (uint8_t)i);

	ft_heap_free(q);
	assert_class_empty_or_fully_free(FT_Z_TINY);
	assert_class_empty_or_fully_free(FT_Z_SMALL);
	return MUNIT_OK;
}

static MunitResult find_owner_same_zone_for_tiny(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	void* a = ft_heap_malloc(64);
	void* b = ft_heap_malloc(120); /* both tiny */

	munit_assert_not_null(a);
	munit_assert_not_null(b);

	t_zone* za = ft_heap_find_owner(a);
	t_zone* zb = ft_heap_find_owner(b);
	munit_assert_not_null(za);
	munit_assert_not_null(zb);
	munit_assert_ptr_equal(za, zb);

	ft_heap_free(a);
	ft_heap_free(b);
	assert_class_empty_or_fully_free(FT_Z_TINY);
	return MUNIT_OK;
}

static MunitResult realloc_zero_frees(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	void* p = ft_heap_malloc(32);
	munit_assert_not_null(p);
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, 1);

	void* np = ft_heap_realloc(p, 0);
	munit_assert_null(np);
	assert_class_empty_or_fully_free(FT_Z_TINY);
	return MUNIT_OK;
}

static MunitResult free_invalid_is_ignored(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	void* p = ft_heap_malloc(64);
	munit_assert_not_null(p);
	size_t nz_before = ft_heap_zone_count(FT_Z_TINY);

	/* invalid free must not crash nor change valid state */
	ft_heap_free((void*)0x12345678);
	munit_assert_size(ft_heap_zone_count(FT_Z_TINY), ==, nz_before);

	ft_heap_free(p);
	assert_class_empty_or_fully_free(FT_Z_TINY);
	return MUNIT_OK;
}

static MunitResult show_alloc_mem_total_is_correct(const MunitParameter params[], void* user_data)
{
	(void)params; (void)user_data;

	void* t1 = ft_heap_malloc(1);                          /* tiny */
	void* t2 = ft_heap_malloc(TINY_BIN_SIZE - 8);          /* tiny */
	void* s1 = ft_heap_malloc((size_t)TINY_BIN_SIZE + 1);  /* small */
	size_t L = (size_t)SMALL_BIN_SIZE + 512;               /* large exact */
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
	{"/tiny_alloc_reuse_and_trim",            tiny_alloc_reuse_and_trim,            setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/small_alloc_zone_lifetime",            small_alloc_zone_lifetime,            setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/large_alloc_shrink_grow",              large_alloc_shrink_grow,              setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/realloc_within_slab_keeps_ptr",        realloc_within_slab_keeps_ptr,        setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/realloc_tiny_to_small_moves_and_copies", realloc_tiny_to_small_moves_and_copies, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/find_owner_same_zone_for_tiny",        find_owner_same_zone_for_tiny,        setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/realloc_zero_frees",                   realloc_zero_frees,                   setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/free_invalid_is_ignored",              free_invalid_is_ignored,              setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{"/show_alloc_mem_total_is_correct",      show_alloc_mem_total_is_correct,      setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
	{NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

static const MunitSuite suite = { "/heap_integration", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };

int main(int argc, char* argv[]) {
	return munit_suite_main(&suite, NULL, argc, argv);
}
