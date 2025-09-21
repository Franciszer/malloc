/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers_test.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 17:16:40 by frthierr          #+#    #+#             */
/*   Updated: 2025/09/22 17:25:22 by frthierr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <assert.h>
#include <stdio.h>
#include "helpers.h"

int main(void)
{
	/* Non power-of-two, with duplicates */
	static const size_t A[] = {3, 5, 5, 8, 13, 21};

	assert(LB_GE(A, 0) == 3);
	assert(LB_GE(A, 3) == 3);
	assert(LB_GE(A, 4) == 5);
	assert(LB_GE(A, 5) == 5); /* first equal (duplicate) */
	assert(LB_GE(A, 6) == 8);
	assert(LB_GE(A, 20) == 21);
	assert(LB_GE(A, 22) == 0); /* not found → sentinel 0 */

	/* Another mix, includes equal neighbors */
	static const size_t B[] = {16, 24, 40, 64, 64, 128};

	assert(LB_GE(B, 1) == 16);
	assert(LB_GE(B, 16) == 16);
	assert(LB_GE(B, 17) == 24);
	assert(LB_GE(B, 64) == 64);
	assert(LB_GE(B, 65) == 128);
	assert(LB_GE(B, 129) == 0);

	/* len == 0 cases */
	assert(lb_ge_size_t(NULL, 0, 10) == 0);
	assert(lb_ge_size_t(B, 0, 10) == 0);

	puts("helpers_test: OK");
	return 0;
}
