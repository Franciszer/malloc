/* itests/show_test.c */
#include "malloc.h"   /* our header, found via -Ilib or -I. */
#include <stdio.h>
#include <string.h>

#ifndef TINY_MAX
#  define TINY_MAX 128
#endif
#ifndef SMALL_MAX
#  define SMALL_MAX 4096
#endif

int main(void) {
    show_alloc_mem();
    
    puts("t1 alloc");
    void *t1 = malloc(32);
    puts("t2 alloc");
    void *t2 = malloc(TINY_MAX);
    // void *s2 = malloc(SMALL_MAX);
    // void *s3 = malloc(SMALL_MAX);
    // void *s4 = malloc(SMALL_MAX);
    // void *s1 = malloc(TINY_MAX + 1);
    // void *L  = malloc(64*1024 + 123);

    memset(t1, 0xAA, 32);
    memset(t2, 0xBB, TINY_MAX);
    // memset(s1, 0xCC, TINY_MAX + 1);
    // memset(s2, 0xDD, SMALL_MAX);
    // memset(L,  0xEE, 64*1024 + 123);

    puts("---- show_alloc_mem (after allocs) ----");

    show_alloc_mem();

    /* free a couple, then show again */
    // free(t2);
    free(t1);

    puts("---- show_alloc_mem (after partial frees) ----");
    show_alloc_mem();

    /* clean up */
    // free(t1);
    free(t2);

    puts("---- show_alloc_mem (after all frees) ----");
    show_alloc_mem();
    // free(s2);
    // free(s3);
    // free(s4);
    // free(L);

    puts("show_test: OK");
    return 0;
}
