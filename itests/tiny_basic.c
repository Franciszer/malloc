/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tiny_basic.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 21:25:15 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/24 21:25:16 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap/heap.h"  // for TINY_MAX

int main(void) {
    const size_t n1 = TINY_MAX;       // boundary inclusive
    const size_t n2 = TINY_MAX / 2;   // smaller tiny

    char *a = malloc(n1);
    char *b = malloc(n2);
    if (!a || !b) { fprintf(stderr, "tiny alloc failed\n"); return 1; }

    memset(a, 0xAA, n1);
    memset(b, 0xBB, n2);

    // sanity
    if (a[0] != (char)0xAA || b[0] != (char)0xBB) { fprintf(stderr, "tiny pattern wrong\n"); return 1; }

    free(a);
    free(b);
    puts("tiny_basic: OK");
    return 0;
}
