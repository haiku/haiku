/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <asm_defs.h>

.text

/* uint16 __swap_int16(uint16 value)
 */
FUNCTION(__swap_int16):
		moveq.l	#0,%d0
		/* 32-bit aligned stack! */
		move.l	(4,%a7),%d1
		move.b	%d1,%d0
		lsr.w	#8,%d1
		lsl.w	#8,%d0
		move.b	%d1,%d0
		rts
FUNCTION_END(__swap_int16)

/* uint32 __swap_int32(uint32 value)
 */
FUNCTION(__swap_int32):
/*		moveq.l	#0,%d0*/
		move.b	(7,%a7),%d0
		lsl.l	#8,%d0
		move.b	(6,%a7),%d0
		lsl.l	#8,%d0
		move.b	(5,%a7),%d0
		lsl.l	#8,%d0
		move.b	(4,%a7),%d0
		rts
FUNCTION_END(__swap_int32)

/* uint64 __swap_int64(uint64 value)
 */
FUNCTION(__swap_int64):
/*		moveq.l	#0,%d0*/
		move.b	(7,%a7),%d1
		lsl.l	#8,%d1
		move.b	(6,%a7),%d1
		lsl.l	#8,%d1
		move.b	(5,%a7),%d1
		lsl.l	#8,%d1
		move.b	(4,%a7),%d1
		/**/
		move.b	(11,%a7),%d0
		lsl.l	#8,%d0
		move.b	(10,%a7),%d0
		lsl.l	#8,%d0
		move.b	(9,%a7),%d0
		lsl.l	#8,%d0
		move.b	(8,%a7),%d0
		rts
FUNCTION_END(__swap_int64)

/* TODO: The following functions can surely be optimized. A simple optimization
 * would be to define macros with the contents of the __swap_int{32,64}
 * functions and use those instead of calling the functions.
 */

/* float __swap_float(float value)
 */
FUNCTION(__swap_float):
		jmp		__swap_int32
		//rts
FUNCTION_END(__swap_float)


/* double __swap_double(double value)
 */
FUNCTION(__swap_double):
		jmp		__swap_int32
		//rts
#warning M68K: XXX:check sizeof(double)
FUNCTION_END(__swap_double)

