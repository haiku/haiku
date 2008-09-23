/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


/* uint16 __swap_int16(uint16 value) */
FUNCTION(__swap_int16):
	movl	4(%esp), %eax
	bswap	%eax
	shr		$16, %eax
	ret
FUNCTION_END(__swap_int16)

/* this one is much faster on a P4, courtesy of Marcus Overhagen,
 * a good candidate for per processor optimizations: */
/*
FUNCTION(__swap_int16_p4):
	movl	4(%esp), %eax
	rolw	$8, %ax
	ret
*/

/* uint32 __swap_int32(uint32 value) */
FUNCTION(__swap_int32):
	movl	4(%esp), %eax
	bswap	%eax
	ret
FUNCTION_END(__swap_int32)

/* uint64 __swap_int64(uint64 value) */
FUNCTION(__swap_int64):
	movl	4(%esp), %edx	/* the 32-bits registers are swapped here */
	movl	8(%esp), %eax
	bswap	%eax
	bswap	%edx
	ret
FUNCTION_END(__swap_int64)

/* float __swap_float(float value) */
FUNCTION(__swap_float):
	movl	4(%esp), %eax
	bswap	%eax
	movl	%eax, 4(%esp)
	fld		4(%esp)
	ret
FUNCTION_END(__swap_float)

/* double __swap_double(double value) */
FUNCTION(__swap_double):
	movl	4(%esp), %edx	/* the 32-bits registers are swapped here */
	movl	8(%esp), %eax
	bswap	%eax
	bswap	%edx
	movl	%eax, 4(%esp)
	movl	%edx, 8(%esp)
	fldl	4(%esp)
	ret
FUNCTION_END(__swap_double)
