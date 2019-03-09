/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* float __swap_float(float value)
 *					  f1
 */
FUNCTION(__swap_float):
		// push a stack frame
		stwu	%r1, -32(%r1)
		mflr	%r0
		stw		%r0, 36(%r1)

		// %f1 -> %r3
		stfs	%f1, 20(%r1)
		lwz		%r3, 20(%r1)

		// let __swap_int32 convert %r3
		bl		__swap_int32

		// %r3 -> %f1
		stw		%r3, 20(%r1)
		lfs		%f1, 20(%r1)
		
		// pop the stack frame
		lwz		%r0, 36(%r1)
		mtlr	%r0
		addi	%r1, %r1, 32
		blr

/* double __swap_double(double value)
 *						f1
 */
FUNCTION(__swap_double):
		// push a stack frame
		stwu	%r1, -32(%r1)
		mflr	%r0
		stw		%r0, 36(%r1)

		// %f1 -> (%r3:%r4)
		stfd	%f1, 20(%r1)
		lwz		%r3, 20(%r1)
		lwz		%r4, 24(%r1)

		// let __swap_int64 convert %r3:%r4
		bl		__swap_int64

		// (%r3:%r4) -> %f1
		stw		%r3, 20(%r1)
		stw		%r4, 24(%r1)
		lfd		%f1, 20(%r1)
		
		// pop the stack frame
		lwz		%r0, 36(%r1)
		mtlr	%r0
		addi	%r1, %r1, 32
		blr
