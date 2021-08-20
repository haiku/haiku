/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <asm_defs.h>

.text


/* float __swap_float(float value)
 */
FUNCTION(__swap_float):
/*		moveq.l #0,%d0*/
		move.b  (7,%a7),%d0
		lsl.l   #8,%d0
		move.b  (6,%a7),%d0
		lsl.l   #8,%d0
		move.b  (5,%a7),%d0
		lsl.l   #8,%d0
		move.b  (4,%a7),%d0
	rts
FUNCTION_END(__swap_float)


/* double __swap_double(double value)
 */
FUNCTION(__swap_double):
/*		moveq.l #0,%d0*/
		move.b  (7,%a7),%d1
		lsl.l   #8,%d1
		move.b  (6,%a7),%d1
		lsl.l   #8,%d1
		move.b  (5,%a7),%d1
		lsl.l   #8,%d1
		move.b  (4,%a7),%d1
		/**/
		move.b  (11,%a7),%d0
		lsl.l   #8,%d0
		move.b  (10,%a7),%d0
		lsl.l   #8,%d0
		move.b  (9,%a7),%d0
		lsl.l   #8,%d0
		move.b  (8,%a7),%d0
		rts
#warning M68K: XXX:check sizeof(double)
FUNCTION_END(__swap_double)

