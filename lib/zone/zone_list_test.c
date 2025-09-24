/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_list_test.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:09:11 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 19:03:31 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zone/zone_list.h"
#include "zone/zone.h"
#include "helpers/helpers.h"
#include "munit.h"

/* ---------------- destroy tests ---------------- */

static MunitResult test_destroy_null_head(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;
	ft_zone_ll_destroy(NULL); /* no crash */
	return MUNIT_OK;
}

static MunitResult test_destroy_empty_list(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;
	t_ll_node* head = NULL;
	ft_zone_ll_destroy(&head);
	munit_assert_ptr_null(head);
	return MUNIT_OK;
}

static MunitResult test_destroy_mixed_list(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	t_ll_node* head = NULL;

	t_zone* a = ft_zone_new(FT_Z_TINY, ft_align_up(64, FT_ALIGN), 3);
	t_zone* b = ft_zone_new(FT_Z_SMALL, ft_align_up(256, FT_ALIGN), 2);
	t_zone* c = ft_zone_new(FT_Z_LARGE, 777, 1);

	munit_assert_ptr_not_null(a);
	munit_assert_ptr_not_null(b);
	munit_assert_ptr_not_null(c);

	ft_ll_push_front(&head, &a->link);
	ft_ll_push_front(&head, &b->link);
	ft_ll_push_front(&head, &c->link);

	ft_zone_ll_destroy(&head);
	munit_assert_ptr_null(head);
	return MUNIT_OK;
}

/* ---------------- first_with_space tests ---------------- */

static MunitResult test_first_with_space_empty(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;
	t_ll_node* head = NULL;
	munit_assert_ptr_null(ft_zone_ll_first_with_space(head));
	return MUNIT_OK;
}

static MunitResult test_first_with_space_skips_full(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;
	t_ll_node* head = NULL;

	t_zone* z_full = ft_zone_new(FT_Z_TINY, ft_align_up(32, FT_ALIGN), 2);
	t_zone* z_avail = ft_zone_new(FT_Z_TINY, ft_align_up(32, FT_ALIGN), 3);
	munit_assert_ptr_not_null(z_full);
	munit_assert_ptr_not_null(z_avail);

	/* Fill first zone completely */
	while (ft_zone_alloc_block(z_full)) {
	}
	munit_assert_size(z_full->free_count, ==, 0);
	munit_assert_false(ft_zone_has_space(z_full));
	munit_assert_true(ft_zone_has_space(z_avail));

	/* Push full first, then available → helper should skip full and return avail */
	ft_ll_push_front(&head, &z_full->link);
	ft_ll_push_front(&head, &z_avail->link);

	t_zone* z = ft_zone_ll_first_with_space(head);
	munit_assert_ptr_equal(z, z_avail);

	/* Allocate directly from the returned zone */
	size_t before = z->free_count;
	void* p = ft_zone_alloc_block(z);
	munit_assert_ptr_not_null(p);
	munit_assert_true(ft_zone_contains(z, p));
	munit_assert_size(z->free_count, ==, before - 1);

	ft_zone_ll_destroy(&head);
	return MUNIT_OK;
}

/* Rotation: keep returning first zone with space; once it’s full, move to next */
static MunitResult test_first_with_space_rotation(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;
	t_ll_node* head = NULL;

	t_zone* a = ft_zone_new(FT_Z_TINY, ft_align_up(64, FT_ALIGN), 2);
	t_zone* b = ft_zone_new(FT_Z_TINY, ft_align_up(64, FT_ALIGN), 2);
	munit_assert_ptr_not_null(a);
	munit_assert_ptr_not_null(b);

	/* push a then b (b is at head visually, but iteration depends on your list macros) */
	ft_ll_push_front(&head, &a->link);
	ft_ll_push_front(&head, &b->link);

	/* Keep allocating from whatever ft_zone_ll_first_with_space returns */
	for (;;) {
		t_zone* z = ft_zone_ll_first_with_space(head);
		if (!z)
			break; /* all full */
		void* p = ft_zone_alloc_block(z);
		munit_assert_ptr_not_null(p);
		munit_assert_true(ft_zone_contains(z, p));
	}

	munit_assert_false(ft_zone_has_space(a));
	munit_assert_false(ft_zone_has_space(b));
	munit_assert_ptr_null(ft_zone_ll_first_with_space(head));

	ft_zone_ll_destroy(&head);
	return MUNIT_OK;
}

/* ---------------- find_container_of tests ---------------- */

static MunitResult test_find_owner_basic(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	t_ll_node* head = NULL;

	t_zone* ztiny = ft_zone_new(FT_Z_TINY, ft_align_up(64, FT_ALIGN), 4);
	t_zone* zsmall = ft_zone_new(FT_Z_SMALL, ft_align_up(256, FT_ALIGN), 2);
	munit_assert_ptr_not_null(ztiny);
	munit_assert_ptr_not_null(zsmall);

	ft_ll_push_front(&head, &ztiny->link);
	ft_ll_push_front(&head, &zsmall->link);

	/* interior pointers */
	void* p1 = (void*)((uintptr_t)ztiny->mem_begin + 8);
	void* p2 = (void*)((uintptr_t)zsmall->mem_begin + 128);

	munit_assert_ptr_equal(ft_zone_ll_find_container_of(head, p1), ztiny);
	munit_assert_ptr_equal(ft_zone_ll_find_container_of(head, p2), zsmall);

	/* outside pointer */
	int dummy;
	munit_assert_ptr_null(ft_zone_ll_find_container_of(head, &dummy));

	ft_zone_ll_destroy(&head);
	return MUNIT_OK;
}

static MunitResult test_find_owner_large_and_bounds(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;

	t_ll_node* head = NULL;

	t_zone* zL = ft_zone_new(FT_Z_LARGE, 777, 1);
	munit_assert_ptr_not_null(zL);
	ft_ll_push_front(&head, &zL->link);

	/* mem_begin is inside, mem_end is exclusive */
	munit_assert_ptr_equal(ft_zone_ll_find_container_of(head, zL->mem_begin), zL);
	munit_assert_ptr_null(ft_zone_ll_find_container_of(head, zL->mem_end));

	/* middle */
	uintptr_t mid = (uintptr_t)zL->mem_begin + 100;
	munit_assert_ptr_equal(ft_zone_ll_find_container_of(head, (void*)mid), zL);

	ft_zone_ll_destroy(&head);
	return MUNIT_OK;
}

/* ---------------- registry ---------------- */

static MunitTest tests[] = {
	{"/zone_list/destroy_null_head",
	 test_destroy_null_head,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone_list/destroy_empty", test_destroy_empty_list, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
	{"/zone_list/destroy_mixed", test_destroy_mixed_list, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},

	{"/zone_list/first_with_space_empty",
	 test_first_with_space_empty,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone_list/first_with_space_skip",
	 test_first_with_space_skips_full,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone_list/first_with_space_rotate",
	 test_first_with_space_rotation,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},

	{"/zone_list/find_owner_basic",
	 test_find_owner_basic,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},
	{"/zone_list/find_owner_large",
	 test_find_owner_large_and_bounds,
	 NULL,
	 NULL,
	 MUNIT_TEST_OPTION_NONE,
	 NULL},

	{NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite suite = {"/zone_list", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
	return munit_suite_main(&suite, NULL, argc, argv);
}
