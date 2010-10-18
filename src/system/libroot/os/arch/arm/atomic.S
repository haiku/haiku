/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <asm_defs.h>


.text

/* int atomic_add(int *value, int increment)
 */
FUNCTION(atomic_add):
#if __ARM_ARCH__ >= 6
miss1:	       	ldrex       r12, [r0]
        	add         r2,  r12, r1
        	strex       r3,  r2,  [r0]
        	teq         r3,  #0
        	bne         miss1
        	mov         r0,  r12
	        bx          lr
#else
        /* disable interrupts, do the add, and reenable */
        mrs     r2, cpsr
        mov     r12, r2
        orr     r2, r2, #(3<<6)
        msr     cpsr_c, r2

        /* ints disabled, old cpsr state in r12 */

        /* do the add, leave the previous value in r0 */
        mov     r3, r0
        ldr     r0, [r3]
        add     r2, r0, r1
        str     r2, [r3]

        /* restore interrupts and exit */
        msr     cpsr_c, r12
        bx      lr
#endif
FUNCTION_END(atomic_add)




/* int atomic_and(int *value, int andValue)
 */
FUNCTION(atomic_and):
#if __ARM_ARCH__ >= 6
miss2:	       	ldrex       r12, [r0]
        	and         r2,  r12, r1
        	strex       r3,  r2,  [r0]
        	teq         r3,  #0
        	bne         miss2
        	mov         r0,  r12
	        bx          lr
#else
        /* disable interrupts, do the and, and reenable */
        mrs     r2, cpsr
        mov     r12, r2
        orr     r2, r2, #(3<<6)
        msr     cpsr_c, r2

        /* ints disabled, old cpsr state in r12 */

        /* do the and, leave the previous value in r0 */
        mov     r3, r0
        ldr     r0, [r3]
        and     r2, r0, r1
        str     r2, [r3]

        /* restore interrupts and exit */
        msr     cpsr_c, r12
        bx      lr
#endif

FUNCTION_END(atomic_and)

/* int atomic_or(int *value, int orValue)
 */
FUNCTION(atomic_or):
#if __ARM_ARCH__ >= 6

miss3:	       	ldrex       r12, [r0]
        	eor         r2,  r12, r1
        	strex       r3,  r2,  [r0]
        	teq         r3,  #0
        	bne         miss3
        	mov         r0,  r12
	        bx	lr
#else
        /* disable interrupts, do the or, and reenable */
        mrs     r2, cpsr
        mov     r12, r2
        orr     r2, r2, #(3<<6)
        msr     cpsr_c, r2

        /* ints disabled, old cpsr state in r12 */

        /* do the or, leave the previous value in r0 */
        mov     r3, r0
        ldr     r0, [r3]
        orr     r2, r0, r1
        str     r2, [r3]

        /* restore interrupts and exit */
        msr     cpsr_c, r12
        bx      lr
#endif

FUNCTION_END(atomic_or)

/* int atomic_set(int *value, int setTo)
 */
FUNCTION(atomic_set):
#if __ARM_ARCH__ >= 6
miss4:		ldrex   r12, [r0]
       		strex   r3, r1, [r0]
       		teq     r3, #0
       		bne     miss4
	        bx  lr
#else
	mov	r3, r0
	swp	r0, r1, [r3]
        bx      lr
#endif
FUNCTION_END(atomic_set)

/* int atomic_test_and_set(int *value, int setTo, int testValue) 
 */
FUNCTION(atomic_test_and_set):
#if __ARM_ARCH__ >= 6
miss5:  ldrex       r12, [r0]                       @ load from the address and mark it exclusive
        cmp         r12, r2                         @ compare the value with the comperand(r2)
        strexeq     r3,  r1,  [r0]                  @ if they were equal, attempt to store the new value (r1)
        bne         differ                          @ if they were not equal jump to (differ) which clears the exclusive tag on the address and returns<
        cmp         r3,  #1                         @ check the status of the store (returned in r3)
        beq         miss5                           @ go back to the start if it failed (0=success, 1=failure)
        bne         same                            @ if it succeeded, jump to (same) and return. there is no need to clrex if strex succeeded
differ: clrex                           	    @ clrex
same:   mov         r0,  r12
#else
        /* disable interrupts, and save state */
        mrs     r3, cpsr
        mov     r12, r3
        orr     r3, r3, #(3<<6)
        msr     cpsr_c, r3

	mov	r3, r0
	ldr	r0, [r3]
	cmp	r0, r2
	streq	r1, [r3]

        /* restore interrupts and exit */
        msr     cpsr_c, r12
#endif
        bx      lr
FUNCTION_END(atomic_test_and_set)

/* int atomic_get(int *value) 
 */
FUNCTION(atomic_get):
	ldr	r0, [r0]
        bx	lr
FUNCTION_END(atomic_get)


/* int64	atomic_add64(vint64 *value, int64 addValue) */
//FUNCTION(atomic_add64):
//		movem.l	%d2-%d3/%a2,-(%a7)
//		move.l	(4,%a7),%a2
//		lea.l	(4,%a2),%a1
//		// addValue
//		move.l	(12,%a7),%d3	/*LSB*/
//		move.l	(8,%a7),%d2		/*MSB*/
//miss5:	// old value
//		move.l	(%a1),%d1		/*LSB*/
//		move.l	(%a2),%d0		/*MSB*/
//		add.l	%d1,%d3
//		addx.l	%d0,%d2
//		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
//		bne		miss5
//		// return value d0:d1
//		movem.l	(%a7)+,%d2-%d3/%a2
//		rts
//FUNCTION_END(atomic_add64)

/* int64	atomic_and64(vint64 *value, int64 andValue) */
//FUNCTION(atomic_and64):
//FUNCTION_END(atomic_and64)

/* int64	atomic_or64(vint64 *value, int64 orValue) */
//FUNCTION(atomic_or64):
//FUNCTION_END(atomic_or64)

/* int64	atomic_set64(vint64 *value, int64 newValue) */
//FUNCTION(atomic_set64):
//		movem.l	%d2-%d3/%a2,-(%a7)
//		move.l	(4,%a7),%a2
//		lea.l	(4,%a2),%a1
//		// new value
//		move.l	(12,%a7),%d3	/*LSB*/
//		move.l	(8,%a7),%d2		/*MSB*/
//		// old value
//		move.l	(%a1),%d1		/*LSB*/
//		move.l	(%a2),%d0		/*MSB*/
//miss8:	cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
//		bne		miss8
//		// return value d0:d1
//		movem.l	(%a7)+,%d2-%d3/%a2
//		rts
//FUNCTION_END(atomic_set64)

/* int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
//FUNCTION(atomic_test_and_set64):
//		movem.l	%d2-%d3/%a2,-(%a7)
//		move.l	(4,%a7),%a2
//		lea.l	(4,%a2),%a1
//		// new value
//		move.l	(12,%a7),%d3	/*LSB*/
//		move.l	(8,%a7),%d2		/*MSB*/
//		// test against value
//		move.l	(20,%a7),%d1	/*LSB*/
//		move.l	(16,%a7),%d0	/*MSB*/
//		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
//		// return value d0:d1
//		movem.l	(%a7)+,%d2-%d3/%a2
//		rts
//FUNCTION_END(atomic_test_and_set64)

/* int64	atomic_get64(vint64 *value) */
//FUNCTION(atomic_get64):
//		movem.l	%d2-%d3/%a2,-(%a7)
//		move.l	(4,%a7),%a2
//		lea.l	(4,%a2),%a1
//		move.l	(%a1),%d1	/*LSB*/
//		move.l	(%a2),%d0		/*MSB*/
//		move.l	%d1,%d3
//		move.l	%d0,%d2
//		// we must use cas... so we change to the same value if matching,
//		// else we get the correct one anyway
//		cas2.l	%d0:%d1,%d2:%d3,(%a2):(%a1)
//		// return value
//		movem.l	(%a7)+,%d2-%d3/%a2
//		rts
//FUNCTION_END(atomic_get64)
