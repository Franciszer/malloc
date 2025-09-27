#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap/heap.h"

enum { N = 2048 };

static size_t pick_size(unsigned seed) {
    // deterministic pseudo-random mix across classes
    unsigned r = seed * 1103515245u + 12345u;
    switch (r % 3u) {
        case 0: return (r % TINY_MAX) + 1;                 // tiny
        case 1: return TINY_MAX + 1 + (r % (SMALL_MAX - TINY_MAX)); // small
        default: return SMALL_MAX + 1 + (r % 4096);        // large up to ~4K over
    }
}

int main(void) {
    unsigned char *ptrs[N] = {0};
    size_t sizes[N] = {0};

    // allocate a bunch
    for (int i = 0; i < N; ++i) {
        size_t n = pick_size((unsigned)i);
        sizes[i] = n;
        ptrs[i] = malloc(n);
        if (!ptrs[i]) { fprintf(stderr, "alloc failed at %d\n", i); return 1; }
        memset(ptrs[i], (unsigned char)(i & 0xFF), n);
    }
    // verify a sample
    for (int i = 0; i < N; i += 137) {
        if (ptrs[i][0] != (unsigned char)(i & 0xFF)) { fprintf(stderr, "pattern mismatch\n"); return 1; }
    }
    // free in a different order
    for (int i = N-1; i >= 0; --i) free(ptrs[i]);

    puts("churn_mix: OK");
    return 0;
}
