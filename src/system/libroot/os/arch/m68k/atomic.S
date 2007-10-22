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
		move.l	(8,%a7),%d1
miss4:	cas.l	%d0,%d1,(%a0)
		bne		miss4
		rts

/* int atomic_test_and_set(int *value, int setTo, int testValue) 
 */
FUNCTION(atomic_test_and_set):
		move.l	(4,%a7),%a0
		move.l	(8,%a7),%d1
		move.l	(12,%a7),%d0
		cas.l	%d0,%d1,(%a0)
		rts

/* int atomic_get(int *value) 
 */
FUNCTION(atomic_get):
		move.l	(4,%a7),%a0
		move.l	(%a0),%d0
		move.l	%d0,%d1
		cas.l	%d0,%d1,(%a0)
		// we must use cas... so we change to the same value if matching,
		// else we get the correct one anyway
		rts

/* m68k elf convention is to return structs in (a0) */
/* other conventions use d0/d1 but which is MSB ?? */

/* int64	atomic_add64(vint64 *value, int64 addValue) */
FUNCTION(atomic_add64):
		move.l	%d2,-(%a7)
		move.l	%d3,-(%a7)
		move.l	(4,%a7),%a1
		// addValue
		move.l	(12,%a7),%d2	/*LSB*/
		move.l	(8,%a7),%d3		/*MSB*/
miss5:	// old value
		move.l	(4,%a1),%d0		/*LSB*/
		move.l	(%a1),%d1		/*MSB*/
		add.l	%d0,%d2
		addx.l	%d1,%d3
		cas2.l	%d0:%d1,%d2:%d3,(4,%a1):(%a1)
		bne		miss5
		// return value
		move.l	%d0,(4,%a0)
		move.l	%d1,(%a0)
		move.l	(%a7)+,%d3
		move.l	(%a7)+,%d2
		rts

/* int64	atomic_and64(vint64 *value, int64 andValue) */
FUNCTION(atomic_and64):
		move.l	%d2,-(%a7)
		move.l	%d3,-(%a7)
		move.l	(4,%a7),%a1
		// addValue
		move.l	(12,%a7),%d2	/*LSB*/
		move.l	(8,%a7),%d3		/*MSB*/
miss6:	// old value
		move.l	(4,%a1),%d0		/*LSB*/
		move.l	(%a1),%d1		/*MSB*/
		and.l	%d0,%d2
		and.l	%d1,%d3
		cas2.l	%d0:%d1,%d2:%d3,(4,%a1):(%a1)
		bne		miss6
		// return value
		move.l	%d0,(4,%a0)
		move.l	%d1,(%a0)
		move.l	(%a7)+,%d3
		move.l	(%a7)+,%d2
		rts

/* int64	atomic_or64(vint64 *value, int64 orValue) */
FUNCTION(atomic_or64):
		move.l	%d2,-(%a7)
		move.l	%d3,-(%a7)
		move.l	(4,%a7),%a1
		// addValue
		move.l	(12,%a7),%d2	/*LSB*/
		move.l	(8,%a7),%d3		/*MSB*/
miss7:	// old value
		move.l	(4,%a1),%d0		/*LSB*/
		move.l	(%a1),%d1		/*MSB*/
		or.l	%d0,%d2
		or.l	%d1,%d3
		cas2.l	%d0:%d1,%d2:%d3,(4,%a1):(%a1)
		bne		miss7
		// return value
		move.l	%d0,(4,%a0)
		move.l	%d1,(%a0)
		move.l	(%a7)+,%d3
		move.l	(%a7)+,%d2
		rts

/* int64	atomic_set64(vint64 *value, int64 newValue) */
FUNCTION(atomic_set64):
		move.l	%d2,-(%a7)
		move.l	%d3,-(%a7)
		move.l	(4,%a7),%a1
		// new value
		move.l	(12,%a7),%d2	/*LSB*/
		move.l	(8,%a7),%d3		/*MSB*/
		// old value
		move.l	(4,%a1),%d0		/*LSB*/
		move.l	(%a1),%d1		/*MSB*/
miss8:	cas2.l	%d0:%d1,%d2:%d3,(4,%a1):(%a1)
		bne		miss8
		// return value
		move.l	%d0,(4,%a0)
		move.l	%d1,(%a0)
		move.l	(%a7)+,%d3
		move.l	(%a7)+,%d2
		rts

/* int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
FUNCTION(atomic_test_and_set64):
		move.l	%d2,-(%a7)
		move.l	%d3,-(%a7)
		move.l	(4,%a7),%a1
		// new value
		move.l	(12,%a7),%d2	/*LSB*/
		move.l	(8,%a7),%d3		/*MSB*/
		// test against value
		move.l	(20,%a7),%d0	/*LSB*/
		move.l	(16,%a7),%d1	/*MSB*/
		cas2.l	%d0:%d1,%d2:%d3,(4,%a1):(%a1)
		// return value
		move.l	%d0,(4,%a0)
		move.l	%d1,(%a0)
		move.l	(%a7)+,%d3
		move.l	(%a7)+,%d2
		rts

/* int64	atomic_get64(vint64 *value) */
FUNCTION(atomic_get64):
		move.l	%d2,-(%a7)
		move.l	%d3,-(%a7)
		move.l	(4,%a7),%a1
		move.l	(4,%a1),%d0	/*LSB*/
		move.l	(%a1),%d1		/*MSB*/
		move.l	%d0,%d2
		move.l	%d1,%d3
		// we must use cas... so we change to the same value if matching,
		// else we get the correct one anyway
		cas2.l	%d0:%d1,%d2:%d3,(4,%a1):(%a1)
		// return value
		move.l	%d0,(4,%a0)
		move.l	%d1,(%a0)
		move.l	(%a7)+,%d3
		move.l	(%a7)+,%d2
		rts

