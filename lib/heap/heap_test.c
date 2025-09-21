/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_test.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 17:44:55 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/22 19:16:02 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// gcc -std=c11 -Wall -Wextra -Werror -I. tests/test_heap_core.c helpers/helpers.c heap.c -o test_heap_core
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "heap.h"
#include "helpers/helpers.h"

static void test_init_destroy_knobs(void) {
    ft_heap_init(123, 456);
    assert(g_ft_heap.tiny == NULL);
    assert(g_ft_heap.small == NULL);
    assert(g_ft_heap.large == NULL);
    assert(g_ft_heap.tiny_min_blocks  == 123);
    assert(g_ft_heap.small_min_blocks == 456);


    ft_heap_destroy();
    ft_heap_destroy(); // verify idempotency
    assert(g_ft_heap.tiny  == NULL);
    assert(g_ft_heap.small == NULL);
    assert(g_ft_heap.large == NULL);
    assert(g_ft_heap.tiny_min_blocks  == 0);
    assert(g_ft_heap.small_min_blocks == 0);
}

static void test_classify(void) {
    // pick some project-wide thresholds
    // (Make sure TINY_MAX and SMALL_MAX are visible via heap.h)
    assert(ft_heap_classify(0) == FT_Z_TINY);            // policy: treat 0 as 1
    assert(ft_heap_classify(1) == FT_Z_TINY);
    assert(ft_heap_classify(TINY_MAX) == FT_Z_TINY);
    assert(ft_heap_classify(TINY_MAX + 1) == FT_Z_SMALL);
    assert(ft_heap_classify(SMALL_MAX) == FT_Z_SMALL);
    assert(ft_heap_classify(SMALL_MAX + 1) == FT_Z_LARGE);
}

static void test_pick_bin_size(void) {
    // These depend on your TINY/SMALL bins in heap.c
    // Adjust if you change them.
    assert(ft_heap_pick_bin_size(FT_Z_TINY, 1)   == 16);
    assert(ft_heap_pick_bin_size(FT_Z_TINY, 16)  == 16);
    assert(ft_heap_pick_bin_size(FT_Z_TINY, 17)  == 32);
    assert(ft_heap_pick_bin_size(FT_Z_TINY, 65)  == 128);
    assert(ft_heap_pick_bin_size(FT_Z_TINY, 129) == 0);   // "no bin" → LARGE

    assert(ft_heap_pick_bin_size(FT_Z_SMALL, 129)  == 256);
    assert(ft_heap_pick_bin_size(FT_Z_SMALL, 256)  == 256);
    assert(ft_heap_pick_bin_size(FT_Z_SMALL, 257)  == 512);
    assert(ft_heap_pick_bin_size(FT_Z_SMALL, 4096) == 4096);
    assert(ft_heap_pick_bin_size(FT_Z_SMALL, 4097) == 0); // "no bin" → LARGE
}

static void test_malloc_tiny_alignment_and_owner(void) {
    ft_heap_init(100, 100);

    void *p1 = ft_heap_malloc(1);
    void *p2 = ft_heap_malloc(15);
    void *p3 = ft_heap_malloc(16);

    assert(p1 && ((uintptr_t)p1 % FT_ALIGN) == 0);
    assert(p2 && ((uintptr_t)p2 % FT_ALIGN) == 0);
    assert(p3 && ((uintptr_t)p3 % FT_ALIGN) == 0);

    ft_zone *z1 = ft_heap_find_owner(p1);
    ft_zone *z2 = ft_heap_find_owner(p2);
    ft_zone *z3 = ft_heap_find_owner(p3);
    assert(z1 && z2 && z3);
    assert(z1->klass == FT_Z_TINY);
    assert(z2->klass == FT_Z_TINY);
    assert(z3->klass == FT_Z_TINY);

    assert(ft_zone_contains(z1, p1));
    assert(ft_zone_contains(z2, p2));
    assert(ft_zone_contains(z3, p3));

    ft_heap_destroy();
}

static void test_malloc_small_vs_large(void) {
    ft_heap_init(64, 64);

    void *ps = ft_heap_malloc(SMALL_MAX);
    void *pl = ft_heap_malloc(SMALL_MAX + 1);

    assert(ps && pl);
    assert(((uintptr_t)ps % FT_ALIGN) == 0);
    assert(((uintptr_t)pl % FT_ALIGN) == 0);

    ft_zone *zs = ft_heap_find_owner(ps);
    ft_zone *zl = ft_heap_find_owner(pl);
    assert(zs && zl);
    assert(zs->klass != FT_Z_LARGE);
    assert(zl->klass == FT_Z_LARGE);
    assert(pl >= zl->mem_begin && pl < zl->mem_end);

    ft_heap_destroy();
}

static void test_free_slab_increases_freecount(void) {
    ft_heap_init(100, 100);

    void *p1 = ft_heap_malloc(24);  // assume 32-bin
    void *p2 = ft_heap_malloc(24);

    ft_zone *z = ft_heap_find_owner(p1);
    assert(z && z->klass != FT_Z_LARGE);
    size_t before = z->free_count;

    ft_heap_free(p1);
    assert(z->free_count == before + 1);

    ft_heap_free(p2);
    assert(z->free_count == before + 2);

    // free(NULL) is a no-op
    ft_heap_free(NULL);

    ft_heap_destroy();
}

static void test_free_large_unmaps(void) {
    ft_heap_init(100, 100);

    void *pl = ft_heap_malloc(SMALL_MAX + 1234);  // force large
    assert(pl);
    ft_zone *zl = ft_heap_find_owner(pl);
    assert(zl && zl->klass == FT_Z_LARGE);

    ft_heap_free(pl);

    // owner should be gone
    assert(ft_heap_find_owner(pl) == NULL);

    ft_heap_destroy();
}

static void test_realloc_keep_in_place_when_fits(void) {
    ft_heap_init(100, 100);

    void *p = ft_heap_malloc(24);   // 32-bin likely
    assert(p);
    ft_zone *z = ft_heap_find_owner(p);
    assert(z && z->klass != FT_Z_LARGE);

    // shrinking or staying below bin keeps the same pointer
    void *q = ft_heap_realloc(p, 16);
    assert(q == p);

    // even modest growth within same bin (still <= bin_size) keeps pointer
    q = ft_heap_realloc(p, z->bin_size);
    assert(q == p);

    ft_heap_free(p);
    ft_heap_destroy();
}

static void test_realloc_grow_moves_and_copies(void) {
    ft_heap_init(100, 100);

    // put a known pattern
    const char msg[] = "hello, allocator!";
    void *p = ft_heap_malloc(sizeof msg);
    assert(p);
    memcpy(p, msg, sizeof msg);

    // grow enough to force a larger bin or large mapping
    void *q = ft_heap_realloc(p, 4096);
    assert(q);
    // content preserved (prefix min(old,new))
    assert(memcmp(q, msg, sizeof msg) == 0);

    // realloc(NULL, n)
    void *r = ft_heap_realloc(NULL, 50);
    assert(r && ((uintptr_t)r % FT_ALIGN) == 0);

    // realloc(ptr, 0) -> frees and returns NULL
    void *s = ft_heap_realloc(r, 0);
    assert(s == NULL);

    ft_heap_free(q);
    ft_heap_destroy();
}

int main(void) {
    test_init_destroy_knobs();
    test_classify();
    test_pick_bin_size();

    // malloc tests
    test_malloc_tiny_alignment_and_owner();
    test_malloc_small_vs_large();

    // free tests
    test_free_slab_increases_freecount();
    test_free_large_unmaps();

    // realloc tests
    test_realloc_keep_in_place_when_fits();
    test_realloc_grow_moves_and_copies();
    puts("test_heap_core: OK");
    return 0;
}
