// itests/show_tests.c
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>            // <-- add this

#include "malloc.h"           // your header that declares show_alloc_mem (ok)

#ifndef TEST_TINY_MAX
#  define TEST_TINY_MAX  128
#endif
#ifndef TEST_SMALL_MAX
#  define TEST_SMALL_MAX 4096
#endif

// unchanged: capture_stdout() ...

static int contains(const char *hay, const char *needle) {
    return hay && needle && strstr(hay, needle) != NULL;
}

int main(void) {
    // Make sure each class is represented
    void *t1 = malloc(32);
    void *t2 = malloc(TEST_TINY_MAX);
    void *s1 = malloc(TEST_TINY_MAX + 1);
    void *s2 = malloc(TEST_SMALL_MAX);
    void *L  = malloc(64*1024 + 123);

    memset(t1, 0xAA, 32);
    memset(t2, 0xBB, TEST_TINY_MAX);
    memset(s1, 0xCC, TEST_TINY_MAX + 1);
    memset(s2, 0xDD, TEST_SMALL_MAX);
    memset(L,  0xEE, 64*1024 + 123);

    // Resolve show_alloc_mem at runtime from the preloaded allocator
    typedef void (*show_fn_t)(void);
    show_fn_t fn = (show_fn_t)dlsym(RTLD_DEFAULT, "show_alloc_mem");
    if (!fn) {
        fprintf(stderr, "show_alloc_mem not found (is LD_PRELOAD set?)\n");
        // 77 is a common “skip” code in test suites
        return 77;
    }

    size_t out_len = 0;
    char *out = capture_stdout(fn, &out_len);
    if (!out) {
        fprintf(stderr, "failed to capture show_alloc_mem output\n");
        return 1;
    }

    int ok = 1;
    ok &= contains(out, "TINY");
    ok &= contains(out, "SMALL");
    ok &= contains(out, "LARGE");
    ok &= contains(out, "0x");
    ok &= contains(out, "bytes");

    if (!ok) {
        fprintf(stderr, "show_alloc_mem output unexpected:\n%s\n", out);
        free(out);
        return 2;
    }

    free(out);

    free(t1); free(t2); free(s1); free(s2); free(L);

    puts("show_alloc_mem: OK");
    return 0;
}
