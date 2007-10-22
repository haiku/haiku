/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
 * gcc/config/m68k/m68k.h:CALL_USED_REGISTERS:
 * d0,d1,a0,a1 are scratch regs, not to be saved.
 */

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* int atomic_add(int *value, int increment)
 */
FUNCTION(atomic_add):
		move.l	(4,%a7),%a0
		move.l	(%a0),%d0
miss1:	move.l	%d0,%d1
		add.l	(8,%a7),%d1
		cas.l	%d0,%d1,(%a0)
		bne		miss1
		// d0 = old value
		rts

/* int atomic_and(int *value, int andValue)
 */
FUNCTION(atomic_and):
		move.l	(4,%a7),%a0
		move.l	(%a0),%d0
miss2:	move.l	%d0,%d1
		and.l	(8,%a7),%d1
		cas.l	%d0,%d1,(%a0)
		bne		miss2
		// d0 = old value
		rts

/* int atomic_or(int *value, int orValue)
 */
FUNCTION(atomic_or):
		move.l	(4,%a7),%a0
		move.l	(%a0),%d0
miss3:	move.l	%d0,%d1
		or.l	(8,%a7),%d1
		cas.l	%d0,%d1,(%a0)
		bne		miss3
		rts

/* int atomic_set(int *value, int setTo)
 */
FUNCTION(atomic_set):
		move.l	(4,%a7),%a0
		move.l	(%a0),%d0
miss4:	move.l	%d0,%d1
		move.l	(8,%a7),%d1
		cas.l	%d0,%d1,(%a0)
		bne		miss4
		rts

/* int atomic_test_and_set(int *value, int setTo, int testValue) 
 */
FUNCTION(atomic_test_and_set):
		move.l	(4,%a7),%a0
		move.l	(8,%a7),%d1
		move.l	(12,%a7),%d0
		cas.l	%d0,%d1,(%a0)
//XXX:
		rts

lost5:	lwarx	%r6, 0, %r3
		cmpw	%r6, %r5
		bne		out5
		stwcx.	%r4, 0, %r3
out5:	mr		%r3, %r6
		blr

/* int atomic_get(int *value) 
 *                  r3
 */
FUNCTION(atomic_get):
lost6:	lwarx	%r5, 0, %r3
		stwcx.	%r5, 0, %r3
		bne-	lost6
		mr		%r3, %r5
		blr
