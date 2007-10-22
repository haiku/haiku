/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* uint16 __swap_int16(uint16 value)
 *                     r3
 */
FUNCTION(__swap_int16):
		rlwinm	%r4, %r3, 8, 16, 23		// byte 4 -> byte 3 (clear other bits)
		rlwimi	%r4, %r3, 24, 24, 31	// byte 3 -> byte 4 (copy into)
		mr		%r3, %r4				// copy to result register
		blr


/* uint32 __swap_int32(uint32 value)
 *                     r3
 */
FUNCTION(__swap_int32):
		rlwinm	%r4, %r3, 24, 0, 31		// byte 4 to 1, byte 2 to 3
		rlwimi	%r4, %r3, 8, 8, 15		// byte 3 to 2
		rlwimi	%r4, %r3, 8, 24, 31		// byte 1 to 4
		mr		%r3, %r4
		blr


/* uint64 __swap_int64(uint64 value)
 *                     r3/r4
 */
FUNCTION(__swap_int64):
		rlwinm	%r5, %r3, 24, 0, 31		// byte 4 to 5, byte 2 to 7
		rlwimi	%r5, %r3, 8, 8, 15		// byte 3 to 6
		rlwimi	%r5, %r3, 8, 24, 31		// byte 1 to 8
		rlwinm	%r3, %r4, 24, 0, 31		// byte 8 to 1, byte 6 to 3
		rlwimi	%r3, %r4, 8, 8, 15		// byte 7 to 2
		rlwimi	%r3, %r4, 8, 24, 31		// byte 5 to 4
		mr		%r4, %r5				// copy lower 32 bits
		blr


/* TODO: The following functions can surely be optimized. A simple optimization
 * would be to define macros with the contents of the __swap_int{32,64}
 * functions and use those instead of calling the functions.
 */

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
