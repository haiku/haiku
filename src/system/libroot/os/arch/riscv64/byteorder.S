/*
 * Copyright 2018-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Optimized byteswap functions from the rv8 test
 * suite licensed under the MIT License
 * https://github.com/rv8-io/rv8/tree/master/src/test
 *
 */

#include <asm_defs.h>

.text

/* float __swap_float(float value)
 */
FUNCTION(__swap_float):
		// Assumes single precision
	1:	auipc   a4, %pcrel_hi(__bswap64_c1)
		ld      a4, %pcrel_lo(1b)(a4)
		slli    a5, a0, 8
		and     a5, a5, a4
		srli    a0, a0, 8
		srli    a4, a4, 8
		and     a0, a0, a4
		or      a5, a5, a0
	1:	auipc   a4, %pcrel_hi(__bswap64_c2)
		ld      a4, %pcrel_lo(1b)(a4)
		slli    a0, a5, 16
		and     a0, a0, a4
		srli    a5, a5, 16
		srli    a4, a4, 16
		and     a5, a5, a4
		or      a5, a5, a0
		slli    a0, a5, 32
		srli    a5, a5, 32
		or      a0, a0, a5
		ret
FUNCTION_END(__swap_float)


/* double __swap_double(double value)
 */
FUNCTION(__swap_double):
		// Assumes double is int64 on RV64
	1:	auipc   a4, %pcrel_hi(__bswap64_c1)
		ld      a4, %pcrel_lo(1b)(a4)
		slli    a5, a0, 8
		and     a5, a5, a4
		srli    a0, a0, 8
		srli    a4, a4, 8
		and     a0, a0, a4
		or      a5, a5, a0
	1:	auipc   a4, %pcrel_hi(__bswap64_c2)
		ld      a4, %pcrel_lo(1b)(a4)
		slli    a0, a5, 16
		and     a0, a0, a4
		srli    a5, a5, 16
		srli    a4, a4, 16
		and     a5, a5, a4
		or      a5, a5, a0
		slli    a0, a5, 32
		srli    a5, a5, 32
		or      a0, a0, a5
		ret
FUNCTION_END(__swap_double)


.section .rodata
__bswap32_c1: .4byte 0xFF00FF00ULL
__bswap64_c1: .8byte 0xFF00FF00FF00FF00ULL
__bswap64_c2: .8byte 0xFFFF0000FFFF0000ULL
