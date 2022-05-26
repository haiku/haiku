/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <asm_defs.h>

/*
 * gcc/config/m68k/m68k.h:CALL_USED_REGISTERS:
 * d0,d1,a0,a1 are scratch regs, not to be saved.
 */

/*
 * see http://mail-index.netbsd.org/amiga/1995/06/
 * for concerns about TAS, CAS and Amiga.
 * I suppose as long as we don't use it on a variable in 
 * chip ram it shouldn't be a problem, and since we don't
 * have any SMP Amiga anyway it should be ok.
 */

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
FUNCTION_END(atomic_add)

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
FUNCTION_END(atomic_and)

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
FUNCTION_END(atomic_or)

/* int atomic_set(int *value, int setTo)
 */
FUNCTION(atomic_set):
		move.l	(4,%a7),%a0
		move.l	(%a0),%d0
		move.l	(8,%a7),%d1
miss4:	cas.l	%d0,%d1,(%a0)
		bne		miss4
		rts
FUNCTION_END(atomic_set)

/* int atomic_test_and_set(int *value, int setTo, int testValue) 
 */
FUNCTION(atomic_test_and_set):
		move.l	(4,%a7),%a0
		move.l	(8,%a7),%d1
		move.l	(12,%a7),%d0
		cas.l	%d0,%d1,(%a0)
		rts
FUNCTION_END(atomic_test_and_set)

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
FUNCTION_END(atomic_get)

/* m68k elf convention is to return structs in (a0)
 * but use d0/d1 for int64 and small structs.
 * d0 MSB, d1 LSB
 */
#warning M68K: 68060 doesn't have CAS2: use spinlock ??
/* see http://retropc.net/x68000/software/develop/as/has060/m68k.htm */

/* int64	atomic_add64(vint64 *value, int64 addValue) */
FUNCTION(atomic_add64):
		movem.l	%d2-%d3/%a2,-(%a7)
		move.l	(4,%a7),%a2
		lea.l	(4,%a2),%a1
		// addValue
		move.l	(12,%a7),%d3	/*LSB*/
		move.l	(8,%a7),%d2		/*MSB*/
miss5:	// old value
		move.l	(%a1),%d1		/*LSB*/
		move.l	(%a2),%d0		/*MSB*/
		add.l	%d1,%d3
		addx.l	%d0,%d2
		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
		bne		miss5
		// return value d0:d1
		movem.l	(%a7)+,%d2-%d3/%a2
		rts
FUNCTION_END(atomic_add64)

/* int64	atomic_and64(vint64 *value, int64 andValue) */
FUNCTION(atomic_and64):
		movem.l	%d2-%d3/%a2,-(%a7)
		move.l	(4,%a7),%a2
		lea.l	(4,%a2),%a1
		// addValue
		move.l	(12,%a7),%d3	/*LSB*/
		move.l	(8,%a7),%d2		/*MSB*/
miss6:	// old value
		move.l	(%a1),%d1		/*LSB*/
		move.l	(%a2),%d0		/*MSB*/
		and.l	%d1,%d3
		and.l	%d0,%d2
		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
		bne		miss6
		// return value d0:d1
		movem.l	(%a7)+,%d2-%d3/%a2
		rts
FUNCTION_END(atomic_and64)

/* int64	atomic_or64(vint64 *value, int64 orValue) */
FUNCTION(atomic_or64):
		movem.l	%d2-%d3/%a2,-(%a7)
		move.l	(4,%a7),%a2
		lea.l	(4,%a2),%a1
		// addValue
		move.l	(12,%a7),%d3	/*LSB*/
		move.l	(8,%a7),%d2		/*MSB*/
miss7:	// old value
		move.l	(%a1),%d1		/*LSB*/
		move.l	(%a2),%d0		/*MSB*/
		or.l	%d1,%d3
		or.l	%d0,%d2
		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
		bne		miss7
		// return value d0:d1
		movem.l	(%a7)+,%d2-%d3/%a2
		rts
FUNCTION_END(atomic_or64)

/* int64	atomic_set64(vint64 *value, int64 newValue) */
FUNCTION(atomic_set64):
		movem.l	%d2-%d3/%a2,-(%a7)
		move.l	(4,%a7),%a2
		lea.l	(4,%a2),%a1
		// new value
		move.l	(12,%a7),%d3	/*LSB*/
		move.l	(8,%a7),%d2		/*MSB*/
		// old value
		move.l	(%a1),%d1		/*LSB*/
		move.l	(%a2),%d0		/*MSB*/
miss8:	cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
		bne		miss8
		// return value d0:d1
		movem.l	(%a7)+,%d2-%d3/%a2
		rts
FUNCTION_END(atomic_set64)

/* int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
FUNCTION(atomic_test_and_set64):
		movem.l	%d2-%d3/%a2,-(%a7)
		move.l	(4,%a7),%a2
		lea.l	(4,%a2),%a1
		// new value
		move.l	(12,%a7),%d3	/*LSB*/
		move.l	(8,%a7),%d2		/*MSB*/
		// test against value
		move.l	(20,%a7),%d1	/*LSB*/
		move.l	(16,%a7),%d0	/*MSB*/
		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
		// return value d0:d1
		movem.l	(%a7)+,%d2-%d3/%a2
		rts
FUNCTION_END(atomic_test_and_set64)

/* int64	atomic_get64(vint64 *value) */
FUNCTION(atomic_get64):
		movem.l	%d2-%d3/%a2,-(%a7)
		move.l	(4,%a7),%a2
		lea.l	(4,%a2),%a1
		move.l	(%a1),%d1	/*LSB*/
		move.l	(%a2),%d0		/*MSB*/
		move.l	%d1,%d3
		move.l	%d0,%d2
		// we must use cas... so we change to the same value if matching,
		// else we get the correct one anyway
		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
		// return value
		movem.l	(%a7)+,%d2-%d3/%a2
		rts
FUNCTION_END(atomic_get64)
.text

#warning IMPLEMENT GCC 64-bit ATOMICS ON m68k!

/* These are to fill in 64-bit atomic calls emitted by
 * by GCC when 64-bit atomics are unavailable.
 */

FUNCTION(__atomic_fetch_add_8):
		nop
		rts
FUNCTION_END(__atomic_fetch_add_8)

FUNCTION(__atomic_store_8):
		nop
		rts
FUNCTION_END(__atomic_store_8)

FUNCTION(__atomic_load_8):
		nop
		rts
FUNCTION_END(__atomic_load_8)

