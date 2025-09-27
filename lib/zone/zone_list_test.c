/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zone_list_test.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:09:11 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/27 19:30:40 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "zone/zone_list.h"
#include "zone/zone.h"
#include "helpers/helpers.h"
#include "data_structures/linked_list.h"
#include "munit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---- stdout capture helper: calls fn(label, head) and returns captured text ---- */
static char* capture_call_size(size_t* out_len,
                               size_t (*fn)(const char*, t_ll_node*),
                               const char* label,
                               t_ll_node* head)
{
    int pipefd[2];
    if (pipe(pipefd) != 0) return NULL;

    int saved = dup(STDOUT_FILENO);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    (void)fn(label, head); /* we assert by scanning output */

    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);

    char* buf = NULL;
    size_t cap = 0, len = 0;
    for (;;) {
        if (cap - len < 512) {
            size_t ncap = cap ? cap * 2 : 1024;
            char* nb = (char*)realloc(buf, ncap);
            if (!nb) { free(buf); return NULL; }
            buf = nb; cap = ncap;
        }
        ssize_t n = read(pipefd[0], buf + len, cap - len);
        if (n <= 0) break;
        len += (size_t)n;
    }
    close(pipefd[0]);

    if (buf) {
        if (len == cap) {
            char* nb = (char*)realloc(buf, cap + 1);
            if (!nb) { free(buf); return NULL; }
            buf = nb;
        }
        buf[len] = '\0';
    }
    if (out_len) *out_len = len;
    return buf;
}

/* ---------------- destroy tests ---------------- */

static MunitResult test_destroy_null_head(const MunitParameter params[], void* data)
{
    (void)params; (void)data;
    ft_zone_ll_destroy(NULL); /* no crash */
    return MUNIT_OK;
}

static MunitResult test_destroy_empty_list(const MunitParameter params[], void* data)
{
    (void)params; (void)data;
    t_ll_node* head = NULL;
    ft_zone_ll_destroy(&head);
    munit_assert_ptr_null(head);
    return MUNIT_OK;
}

static MunitResult test_destroy_mixed_list(const MunitParameter params[], void* data)
{
    (void)params; (void)data;

    t_ll_node* head = NULL;

    t_zone* a = ft_zone_new(FT_Z_TINY,  ft_align_up(64,  FT_ALIGN), 3);
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
    (void)params; (void)data;
    t_ll_node* head = NULL;
    munit_assert_ptr_null(ft_zone_ll_first_with_space(head));
    return MUNIT_OK;
}

static MunitResult test_first_with_space_skips_full(const MunitParameter params[], void* data)
{
    (void)params; (void)data;
    t_ll_node* head = NULL;

    t_zone* z_full  = ft_zone_new(FT_Z_TINY, ft_align_up(32, FT_ALIGN), 2);
    t_zone* z_avail = ft_zone_new(FT_Z_TINY, ft_align_up(32, FT_ALIGN), 3);
    munit_assert_ptr_not_null(z_full);
    munit_assert_ptr_not_null(z_avail);

    /* Fill first zone completely */
    while (ft_zone_alloc_block(z_full)) {}
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
    (void)params; (void)data;
    t_ll_node* head = NULL;

    t_zone* a = ft_zone_new(FT_Z_TINY, ft_align_up(64, FT_ALIGN), 2);
    t_zone* b = ft_zone_new(FT_Z_TINY, ft_align_up(64, FT_ALIGN), 2);
    munit_assert_ptr_not_null(a);
    munit_assert_ptr_not_null(b);

    /* push a then b */
    ft_ll_push_front(&head, &a->link);
    ft_ll_push_front(&head, &b->link);

    for (;;) {
        t_zone* z = ft_zone_ll_first_with_space(head);
        if (!z) break; /* all full */
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
    (void)params; (void)data;

    t_ll_node* head = NULL;

    t_zone* ztiny  = ft_zone_new(FT_Z_TINY,  ft_align_up(64,  FT_ALIGN), 4);
    t_zone* zsmall = ft_zone_new(FT_Z_SMALL, ft_align_up(256, FT_ALIGN), 2);
    munit_assert_ptr_not_null(ztiny);
    munit_assert_ptr_not_null(zsmall);

    ft_ll_push_front(&head, &ztiny->link);
    ft_ll_push_front(&head, &zsmall->link);

    /* interior pointers */
    void* p1 = (void*)((uintptr_t)ztiny->mem_begin  + 8);
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
    (void)params; (void)data;

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

/* ---------------- sorted printing tests ---------------- */

static MunitResult test_print_sorted_slab(const MunitParameter params[], void* user_data)
{
    (void)params; (void)user_data;

    const size_t bin = 16;
    t_zone* z1 = ft_zone_new(FT_Z_TINY, bin, 8);
    t_zone* z2 = ft_zone_new(FT_Z_TINY, bin, 8);
    munit_assert_not_null(z1);
    munit_assert_not_null(z2);

    /* Allocate some blocks in both */
    (void)ft_zone_alloc_block(z1);
    (void)ft_zone_alloc_block(z1);
    (void)ft_zone_alloc_block(z2);

    /* Build list in reverse address order to exercise sorting */
    t_ll_node* head = NULL;
    ft_ll_push_front(&head, &z1->link);
    ft_ll_push_front(&head, &z2->link);

    size_t outlen = 0;
    char* out = capture_call_size(&outlen, ft_zone_ll_print_sorted, "TINY", head);
    munit_assert_not_null(out);

    /* The first zone line should correspond to the smaller mem_begin */
    t_zone* first  = ((uintptr_t)z1->mem_begin < (uintptr_t)z2->mem_begin) ? z1 : z2;
    t_zone* second = (first == z1) ? z2 : z1;

    char needle1[64], needle2[64];
    snprintf(needle1, sizeof needle1, "TINY : %p", first->mem_begin);
    snprintf(needle2, sizeof needle2, "TINY : %p", second->mem_begin);

    char *p1 = strstr(out, needle1);
    char *p2 = strstr(out, needle2);
    munit_assert_ptr_not_null(p1);
    munit_assert_ptr_not_null(p2);
    munit_assert_true(p1 < p2);

    free(out);
    ft_zone_destroy(z1);
    ft_zone_destroy(z2);
    return MUNIT_OK;
}

static MunitResult test_print_sorted_large(const MunitParameter params[], void* user_data)
{
    (void)params; (void)user_data;

    t_zone* L1 = ft_zone_new(FT_Z_LARGE, 5000, 1);
    t_zone* L2 = ft_zone_new(FT_Z_LARGE, 8000, 1);
    munit_assert_not_null(L1);
    munit_assert_not_null(L2);

    t_ll_node* head = NULL;
    ft_ll_push_front(&head, &L1->link);
    ft_ll_push_front(&head, &L2->link);

    size_t outlen = 0;
    char* out = capture_call_size(&outlen, ft_zone_ll_print_sorted, "LARGE", head);
    munit_assert_not_null(out);

    t_zone* first = ((uintptr_t)L1->mem_begin < (uintptr_t)L2->mem_begin) ? L1 : L2;
    char needle[64];
    snprintf(needle, sizeof needle, "LARGE : %p", first->mem_begin);
    munit_assert_ptr_not_null(strstr(out, needle));

    free(out);
    ft_zone_destroy(L1);
    ft_zone_destroy(L2);
    return MUNIT_OK;
}

/* ---------------- registry ---------------- */

static MunitTest tests[] = {
    { "/zone_list/destroy_null_head",        test_destroy_null_head,        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/zone_list/destroy_empty",            test_destroy_empty_list,       NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/zone_list/destroy_mixed",            test_destroy_mixed_list,       NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },

    { "/zone_list/first_with_space_empty",   test_first_with_space_empty,   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/zone_list/first_with_space_skip",    test_first_with_space_skips_full, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/zone_list/first_with_space_rotate",  test_first_with_space_rotation, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },

    { "/zone_list/find_owner_basic",         test_find_owner_basic,         NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/zone_list/find_owner_large",         test_find_owner_large_and_bounds, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },

    { "/zone_list/print_sorted_slab",        test_print_sorted_slab,        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { "/zone_list/print_sorted_large",       test_print_sorted_large,       NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },

    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite suite = {
    "/zone_list", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&suite, NULL, argc, argv);
}
