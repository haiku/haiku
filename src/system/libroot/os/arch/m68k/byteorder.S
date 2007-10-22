/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* uint16 __swap_int16(uint16 value)
 */
FUNCTION(__swap_int16):
		moveq.l	#0,%d0
		move.b	(5,%a7),%d0
		lsl.w	#8,%d0
		move.b	(4,%a7),%d0
		rts


/* uint32 __swap_int32(uint32 value)
 */
FUNCTION(__swap_int32):
/*		moveq.l	#0,%d0*/
		move.b	(7,%a7),%d0
		lsl.w	#8,%d0
		move.b	(6,%a7),%d0
		lsl.w	#8,%d0
		move.b	(5,%a7),%d0
		lsl.w	#8,%d0
		move.b	(4,%a7),%d0
		rts


/* uint64 __swap_int64(uint64 value)
 */
FUNCTION(__swap_int64):
/*		moveq.l	#0,%d0*/
		move.b	(7,%a7),%d0
		lsl.w	#8,%d0
		move.b	(6,%a7),%d0
		lsl.w	#8,%d0
		move.b	(5,%a7),%d0
		lsl.w	#8,%d0
		move.b	(4,%a7),%d0
		move.l	%d0,(%a0)
		move.b	(11,%a7),%d0
		lsl.w	#8,%d0
		move.b	(10,%a7),%d0
		lsl.w	#8,%d0
		move.b	(9,%a7),%d0
		lsl.w	#8,%d0
		move.b	(8,%a7),%d0
		move.l	%d0,(4,%a0)
		rts


/* TODO: The following functions can surely be optimized. A simple optimization
 * would be to define macros with the contents of the __swap_int{32,64}
 * functions and use those instead of calling the functions.
 */

/* float __swap_float(float value)
 *					  f1
 */
FUNCTION(__swap_float):
XXX:	TODO
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
XXX:	TODO
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
