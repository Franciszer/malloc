// itests/smoke.c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    enum { N = 1000, SZ = 123 };
    char *p[N];

    for (int i = 0; i < N; i++) {
        p[i] = (char*)malloc(SZ);
        if (!p[i]) {
            fprintf(stderr, "malloc failed at i=%d\n", i);
            return 1;
        }
        memset(p[i], (unsigned char)(i & 0xFF), SZ);
    }

    // quick correctness sniff
    long sum = 0;
    for (int i = 0; i < N; i++) sum += p[i][0];
    printf("smoke: sum=%ld\n", sum);

    for (int i = 0; i < N; i++) free(p[i]);

    puts("smoke: OK");
    return 0;
}
