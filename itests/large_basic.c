/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   large_basic.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 21:26:01 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 21:26:02 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap/heap.h"  // SMALL_MAX

int main(void) {
    const size_t n1 = SMALL_MAX + 1;   // large lower bound
    const size_t n2 = (SMALL_MAX + 1) * 4;

    char *a = malloc(n1);
    char *b = malloc(n2);
    if (!a || !b) { fprintf(stderr, "large alloc failed\n"); return 1; }

    a[0] = 0x5A; a[n1-1] = 0x5B;
    b[0] = 0x6A; b[n2-1] = 0x6B;

    if (a[0] != 0x5A || a[n1-1] != 0x5B || b[0] != 0x6A || b[n2-1] != 0x6B) {
        fprintf(stderr, "large pattern wrong\n"); return 1;
    }

    free(a);
    free(b);
    puts("large_basic: OK");
    return 0;
}
