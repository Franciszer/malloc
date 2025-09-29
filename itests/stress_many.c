/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   stress_many.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 21:45:28 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/29 17:01:01 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// itests/stress_many.c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Keep these in sync with your allocator defaults */
#ifndef TEST_TINY_MAX
#  define TEST_TINY_MAX  128
#endif
#ifndef TEST_SMALL_MAX
#  define TEST_SMALL_MAX 4096
#endif

/* Tunables (can override with -D at compile time) */
#ifndef STRESS_SLOTS
#  define STRESS_SLOTS 12000      /* number of live slots tracked */
#endif
#ifndef STRESS_OPS
#  define STRESS_OPS   200000     /* number of operations */
#endif

/* Deterministic PRNG (xorshift) */
static uint32_t rng_state = 0xC0FFEEu;
static inline uint32_t xr(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}
static inline uint32_t urand(uint32_t n) { return (n == 0) ? 0 : (xr() % n); }

/* Test slot */
typedef struct {
    void   *p;
    size_t  sz;
    uint32_t tag;   /* increments on each (re)alloc for this slot */
    int      used;
} Slot;

static inline uint8_t pat_byte(int idx, uint32_t tag) {
    /* simple, deterministic pattern per slot + generation */
    return (uint8_t)((idx * 131u) ^ (tag * 0xA5u) ^ 0x5Eu);
}

static void fill_pattern(Slot *s, int idx) {
    if (!s->p || s->sz == 0) return; /* don’t touch malloc(0) */
    uint8_t *b = (uint8_t *)s->p;
    uint8_t pv = pat_byte(idx, s->tag);
    for (size_t i = 0; i < s->sz; ++i) b[i] = (uint8_t)(pv + (uint8_t)i);
}

static int check_pattern(const Slot *s, int idx) {
    if (!s->p || s->sz == 0) return 1; /* nothing to check */
    const uint8_t *b = (const uint8_t *)s->p;
    uint8_t pv = pat_byte(idx, s->tag);
    for (size_t i = 0; i < s->sz; ++i) {
        if (b[i] != (uint8_t)(pv + (uint8_t)i)) return 0;
    }
    return 1;
}

/* Size chooser biased toward tiny/small with some large */
static size_t choose_size(void) {
    uint32_t r = urand(1000);
    if (r < 650) {
        /* tiny: [1..TEST_TINY_MAX] */
        return 1 + urand(TEST_TINY_MAX);
    } else if (r < 950) {
        /* small: [TEST_TINY_MAX+1 .. TEST_SMALL_MAX] */
        return (TEST_TINY_MAX + 1) + urand(TEST_SMALL_MAX - TEST_TINY_MAX);
    } else {
        /* large: a few pages up to ~128 KiB */
        size_t pages = 1 + urand(32);
        return pages * 4096u + urand(2048);
    }
}

static void do_alloc(Slot *s, int idx) {
    assert(!s->used);
    size_t n = choose_size();
    /* inject occasional malloc(0) */
    if ((xr() & 0xFF) == 0) n = 0;

    void *p = malloc(n);
    if (!p && n != 0) {
        fprintf(stderr, "alloc failed (n=%zu)\n", n);
        exit(1);
    }
    s->p = p;
    s->sz = n;
    s->tag = 1;
    s->used = 1;
    fill_pattern(s, idx);
}

static void do_free(Slot *s) {
    if (!s->used) return;
    free(s->p);
    s->p = NULL;
    s->sz = 0;
    s->tag = 0;
    s->used = 0;
}

int main(void) {
    Slot *slots = (Slot *)calloc(STRESS_SLOTS, sizeof(Slot));
    if (!slots) {
        fprintf(stderr, "calloc slots failed\n");
        return 1;
    }

    /* Warm-up: allocate a handful */
    for (int i = 0; i < STRESS_SLOTS / 8; ++i) {
        int idx = urand(STRESS_SLOTS);
        if (!slots[idx].used) do_alloc(&slots[idx], idx);
    }

    for (int op = 0; op < STRESS_OPS; ++op) {
        int idx = urand(STRESS_SLOTS);
        uint32_t action = xr() % 100;

        if (!slots[idx].used) {
            /* Allocate in empty slot most of the time */
            if (action < 85) {
                do_alloc(&slots[idx], idx);
            } else {
                /* realloc(NULL, n) */
                size_t sz = choose_size();
                void *p = realloc(NULL, sz);
                if (!p && sz != 0) {
                    fprintf(stderr, "realloc(NULL,%zu) failed\n", sz);
                    return 1;
                }
                slots[idx].p = p;
                slots[idx].sz = sz;
                slots[idx].tag = 1;
                slots[idx].used = (sz != 0);
                fill_pattern(&slots[idx], idx);
            }
        } else {
            if (action < 20) {
                do_free(&slots[idx]);
            } else if (action < 60) {
                /* grow/shrink */
                /* We need to check BEFORE and AFTER; handle carefully inside here. */
                /* Check existing content first */
                if (!check_pattern(&slots[idx], idx)) {
                    fprintf(stderr, "pattern mismatch pre-realloc at %d\n", idx);
                    return 2;
                }
                size_t old_sz = slots[idx].sz;
                uint32_t old_tag = slots[idx].tag;
                size_t new_sz = choose_size();
                if ((xr() & 0x3FF) == 0) new_sz = 0; /* occasional free via realloc */
                void *np = realloc(slots[idx].p, new_sz);
                if (!np && new_sz != 0) {
                    fprintf(stderr, "realloc failed\n");
                    return 3;
                }
                slots[idx].p = np;
                slots[idx].sz = new_sz;
                if (new_sz == 0) {
                    slots[idx].tag = 0;
                    slots[idx].used = 0;
                } else {
                    /* Check prefix retained */
                    size_t keep = (old_sz < new_sz) ? old_sz : new_sz;
                    /* Temporarily reconstruct expected bytes from old_tag */
                    if (keep > 0) {
                        uint8_t expect0 = pat_byte(idx, old_tag);
                        const uint8_t *b = (const uint8_t *)slots[idx].p;
                        for (size_t i = 0; i < keep; ++i) {
                            if (b[i] != (uint8_t)(expect0 + (uint8_t)i)) {
                                fprintf(stderr, "prefix mismatch after realloc at %d\n", idx);
                                return 4;
                            }
                        }
                    }
                    /* New generation pattern */
                    slots[idx].tag = old_tag + 1;
                    fill_pattern(&slots[idx], idx);
                    if (!check_pattern(&slots[idx], idx)) {
                        fprintf(stderr, "pattern mismatch post-realloc at %d\n", idx);
                        return 5;
                    }
                }
            } else {
                /* Just validate occasionally */
                if (!check_pattern(&slots[idx], idx)) {
                    fprintf(stderr, "pattern mismatch at %d\n", idx);
                    return 6;
                }
            }
        }
    }

    /* Final verification & cleanup */
    for (int i = 0; i < STRESS_SLOTS; ++i) {
        if (slots[i].used) {
            if (!check_pattern(&slots[i], i)) {
                fprintf(stderr, "final check mismatch at %d\n", i);
                return 7;
            }
            free(slots[i].p);
            slots[i].p = NULL;
            slots[i].used = 0;
        }
    }
    free(slots);

    puts("stress_many: OK");
    return 0;
}
