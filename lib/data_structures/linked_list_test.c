/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   linked_list_test.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 17:34:54 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/21 17:34:56 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "munit.h"
#include <stddef.h> // offsetof
#include "data_structures/linked_list.h"

/* Helper to get the container struct from a node pointer */
#define container_of(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))

/* Small container type embedding the intrusive node */
typedef struct {
    ft_ll_node node;
    int id;
} item_t;

/* --- Test: init puts node in detached state --- */
static MunitResult test_init(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    ft_ll_node n;
    ft_ll_init(&n);
    munit_assert_ptr_null(n.prev);
    munit_assert_ptr_null(n.next);
    return MUNIT_OK;
}

/* --- Test: push_front on empty list yields single node head --- */
static MunitResult test_push_front_single(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    ft_ll_node* head = NULL;
    item_t a = {.node = {0}, .id = 1};
    ft_ll_init(&a.node);

    ft_ll_push_front(&head, &a.node);

    munit_assert_ptr_equal(head, &a.node);
    munit_assert_ptr_null(a.node.prev);
    munit_assert_ptr_null(a.node.next);

    /* Note: current ft_ll_is_linked returns 0 for single-node (prev==next==NULL). */
    munit_assert_int(ft_ll_is_linked(&a.node), ==, 0);

    return MUNIT_OK;
}

/* --- Test: push_front order (new nodes become head) --- */
static MunitResult test_push_front_multiple(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    ft_ll_node* head = NULL;
    item_t a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    ft_ll_init(&a.node);
    ft_ll_init(&b.node);
    ft_ll_init(&c.node);

    ft_ll_push_front(&head, &a.node); // list: a
    ft_ll_push_front(&head, &b.node); // list: b -> a
    ft_ll_push_front(&head, &c.node); // list: c -> b -> a

    munit_assert_ptr_equal(head, &c.node);
    munit_assert_ptr_equal(head->next, &b.node);
    munit_assert_ptr_equal(head->next->next, &a.node);
    munit_assert_ptr_null(a.node.next);
    munit_assert_ptr_null(head->prev);
    munit_assert_ptr_equal(b.node.prev, &c.node);
    munit_assert_ptr_equal(a.node.prev, &b.node);

    return MUNIT_OK;
}

/* --- Test: push_back builds tail correctly --- */
static MunitResult test_push_back_order(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    ft_ll_node* head = NULL;
    item_t a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    ft_ll_init(&a.node);
    ft_ll_init(&b.node);
    ft_ll_init(&c.node);

    ft_ll_push_back(&head, &a.node); // a
    ft_ll_push_back(&head, &b.node); // a -> b
    ft_ll_push_back(&head, &c.node); // a -> b -> c

    munit_assert_ptr_equal(head, &a.node);
    munit_assert_ptr_equal(a.node.next, &b.node);
    munit_assert_ptr_equal(b.node.next, &c.node);
    munit_assert_ptr_equal(c.node.prev, &b.node);
    munit_assert_ptr_null(c.node.next);

    return MUNIT_OK;
}

/* --- Test: pop_front returns head and fixes links --- */
static MunitResult test_pop_front(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    ft_ll_node* head = NULL;
    item_t a = {.id = 1}, b = {.id = 2};
    ft_ll_init(&a.node);
    ft_ll_init(&b.node);

    ft_ll_push_back(&head, &a.node);
    ft_ll_push_back(&head, &b.node);

    ft_ll_node* n = ft_ll_pop_front(&head);
    munit_assert_ptr_equal(n, &a.node);
    munit_assert_ptr_equal(head, &b.node);
    munit_assert_ptr_null(b.node.prev);
    munit_assert_ptr_null(a.node.prev);
    munit_assert_ptr_null(a.node.next);

    n = ft_ll_pop_front(&head);
    munit_assert_ptr_equal(n, &b.node);
    munit_assert_ptr_null(head);

    n = ft_ll_pop_front(&head);
    munit_assert_ptr_null(n);

    return MUNIT_OK;
}

/* --- Test: remove head, middle, tail --- */
static MunitResult test_remove_positions(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    ft_ll_node* head = NULL;
    item_t a = {.id = 1}, b = {.id = 2}, c = {.id = 3}, d = {.id = 4};
    ft_ll_init(&a.node);
    ft_ll_init(&b.node);
    ft_ll_init(&c.node);
    ft_ll_init(&d.node);

    /* Build a -> b -> c -> d */
    ft_ll_push_back(&head, &a.node);
    ft_ll_push_back(&head, &b.node);
    ft_ll_push_back(&head, &c.node);
    ft_ll_push_back(&head, &d.node);

    /* Remove head (a) */
    ft_ll_remove(&head, &a.node);
    munit_assert_ptr_equal(head, &b.node);
    munit_assert_ptr_null(b.node.prev);
    munit_assert_ptr_null(a.node.prev);
    munit_assert_ptr_null(a.node.next);

    /* Remove middle (c) */
    ft_ll_remove(&head, &c.node);
    munit_assert_ptr_equal(b.node.next, &d.node);
    munit_assert_ptr_equal(d.node.prev, &b.node);
    munit_assert_ptr_null(c.node.prev);
    munit_assert_ptr_null(c.node.next);

    /* Remove tail (d) */
    ft_ll_remove(&head, &d.node);
    munit_assert_ptr_equal(head, &b.node);
    munit_assert_ptr_null(b.node.next);
    munit_assert_ptr_null(d.node.prev);
    munit_assert_ptr_null(d.node.next);

    /* Remove last remaining (b) -> empty */
    ft_ll_remove(&head, &b.node);
    munit_assert_ptr_null(head);

    /* Removing already-detached is a no-op */
    ft_ll_remove(&head, &b.node);
    munit_assert_ptr_null(b.node.prev);
    munit_assert_ptr_null(b.node.next);

    return MUNIT_OK;
}

/* --- Test: iteration macros, including SAFE removal during iteration --- */
static MunitResult test_iteration_and_safe_remove(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    ft_ll_node* head = NULL;
    item_t n1 = {.id = 1}, n2 = {.id = 2}, n3 = {.id = 3}, n4 = {.id = 4}, n5 = {.id = 5};
    ft_ll_init(&n1.node);
    ft_ll_init(&n2.node);
    ft_ll_init(&n3.node);
    ft_ll_init(&n4.node);
    ft_ll_init(&n5.node);

    /* Build 1->2->3->4->5 */
    ft_ll_push_back(&head, &n1.node);
    ft_ll_push_back(&head, &n2.node);
    ft_ll_push_back(&head, &n3.node);
    ft_ll_push_back(&head, &n4.node);
    ft_ll_push_back(&head, &n5.node);

    /* Count with FT_LL_FOR_EACH */
    int count = 0;
    FT_LL_FOR_EACH(it, head)
    {
        (void)it;
        count++;
    }
    munit_assert_int(count, ==, 5);

    /* Remove even ids using SAFE variant while iterating */
    FT_LL_FOR_EACH_SAFE(it, tmp, head)
    {
        item_t* it_item = container_of(it, item_t, node);
        if ((it_item->id % 2) == 0) {
            ft_ll_remove(&head, it);
        }
    }

    /* Now list should be 1->3->5 */
    munit_assert_ptr_equal(head, &n1.node);
    munit_assert_ptr_equal(n1.node.next, &n3.node);
    munit_assert_ptr_equal(n3.node.next, &n5.node);
    munit_assert_ptr_null(n5.node.next);

    return MUNIT_OK;
}

/* --- Assemble suite --- */
static MunitTest tests[] = {
    {"/init", test_init, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/push_front_single", test_push_front_single, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/push_front_multiple", test_push_front_multiple, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/push_back_order", test_push_back_order, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/pop_front", test_pop_front, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/remove_positions", test_remove_positions, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/iteration_and_safe_remove",
     test_iteration_and_safe_remove,
     NULL,
     NULL,
     MUNIT_TEST_OPTION_NONE,
     NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite suite = {"/linked_list", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char* argv[])
{
    return munit_suite_main(&suite, NULL, argc, argv);
}
