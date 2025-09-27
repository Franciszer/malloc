/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   small_basic.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 21:25:12 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 21:25:13 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap/heap.h"  // TINY_MAX, SMALL_MAX

int main(void) {
    const size_t n1 = TINY_MAX + 1;          // small lower bound
    const size_t n2 = (SMALL_MAX + TINY_MAX) / 2;

    char *a = malloc(n1);
    char *b = malloc(n2);
    if (!a || !b) { fprintf(stderr, "small alloc failed\n"); return 1; }

    memset(a, 0x11, n1);
    memset(b, 0x22, n2);

    if (a[0] != 0x11 || b[0] != 0x22) { fprintf(stderr, "small pattern wrong\n"); return 1; }

    free(a);
    free(b);
    puts("small_basic: OK");
    return 0;
}
