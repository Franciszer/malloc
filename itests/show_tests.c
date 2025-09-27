/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_tests.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/25 19:18:44 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/25 19:19:02 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// itests/show_alloc_mem.c
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/malloc.h"

#ifndef TEST_TINY_MAX
#  define TEST_TINY_MAX  128
#endif
#ifndef TEST_SMALL_MAX
#  define TEST_SMALL_MAX 4096
#endif

// capture stdout of a function into a malloc'd buffer (caller free)
static char* capture_stdout(void (*fn)(void), size_t *out_len) {
    char path[] = "/tmp/showXXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return NULL;

    FILE *fp = fdopen(fd, "w+");
    if (!fp) { close(fd); unlink(path); return NULL; }

    fflush(stdout);
    int saved = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);

    fn(); // <-- show_alloc_mem()

    fflush(stdout);
    fsync(STDOUT_FILENO);
    dup2(saved, STDOUT_FILENO);
    close(saved);

    // read back
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    if (n < 0) n = 0;
    fseek(fp, 0, SEEK_SET);

    char *buf = (char*)malloc((size_t)n + 1);
    if (!buf) { fclose(fp); unlink(path); return NULL; }

    size_t rd = fread(buf, 1, (size_t)n, fp);
    buf[rd] = '\0';

    if (out_len) *out_len = rd;

    fclose(fp);
    unlink(path);
    return buf;
}

static int contains(const char *hay, const char *needle) {
    return hay && needle && strstr(hay, needle) != NULL;
}

int main(void) {
    // Make sure each class is represented
    void *t1 = malloc(32);                             // TINY
    void *t2 = malloc(TEST_TINY_MAX);                  // TINY (edge)
    void *s1 = malloc(TEST_TINY_MAX + 1);              // SMALL
    void *s2 = malloc(TEST_SMALL_MAX);                 // SMALL (edge)
    void *L  = malloc(64*1024 + 123);                  // LARGE-ish

    // Fill a bit so the blocks aren’t optimized away by silly tooling
    memset(t1, 0xAA, 32);
    memset(t2, 0xBB, TEST_TINY_MAX);
    memset(s1, 0xCC, TEST_TINY_MAX + 1);
    memset(s2, 0xDD, TEST_SMALL_MAX);
    memset(L,  0xEE, 64*1024 + 123);

    size_t out_len = 0;
    char *out = capture_stdout(show_alloc_mem, &out_len);
    if (!out) {
        fprintf(stderr, "failed to capture show_alloc_mem output\n");
        return 1;
    }

    // Very loose assertions: just check the headers and some hex/bytes substrings.
    // We do NOT check exact addresses or totals.
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

    // Clean up
    free(out);

    // You *may* free here; leak detection is done by valgrind for the process.
    free(t1);
    free(t2);
    free(s1);
    free(s2);
    free(L);

    puts("show_alloc_mem: OK");
    return 0;
}
