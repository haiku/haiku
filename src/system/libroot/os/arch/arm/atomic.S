/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <asm_defs.h>
#include <arch_config.h>


.text

#ifndef ATOMIC_FUNCS_ARE_SYSCALLS

/* int atomic_add(int *value, int increment)
 */
FUNCTION(atomic_add):
miss1:	       	ldrex       r12, [r0]
        	add         r2,  r12, r1
        	strex       r3,  r2,  [r0]
        	teq         r3,  #0
        	bne         miss1
        	mov         r0,  r12
	        bx          lr
FUNCTION_END(atomic_add)

/* int atomic_and(int *value, int andValue)
 */
FUNCTION(atomic_and):
miss2:	       	ldrex       r12, [r0]
        	and         r2,  r12, r1
        	strex       r3,  r2,  [r0]
        	teq         r3,  #0
        	bne         miss2
        	mov         r0,  r12
	        bx          lr
FUNCTION_END(atomic_and)

/* int atomic_or(int *value, int orValue)
 */
FUNCTION(atomic_or):
miss3:	       	ldrex       r12, [r0]
        	eor         r2,  r12, r1
        	strex       r3,  r2,  [r0]
        	teq         r3,  #0
        	bne         miss3
        	mov         r0,  r12
	        bx	lr
FUNCTION_END(atomic_or)

/* void atomic_set(int *value, int setTo)
 */
FUNCTION(atomic_set):
		str	r1, [r0]
		bx	lr
FUNCTION_END(atomic_set)

/* int atomic_get_and_set(int *value, int setTo)
 */
FUNCTION(atomic_get_and_set):
miss4:		ldrex   r12, [r0]
       		strex   r3, r1, [r0]
       		teq     r3, #0
       		bne     miss4
	        bx  lr
FUNCTION_END(atomic_get_and_set)

/* int atomic_test_and_set(int *value, int setTo, int testValue) 
 */
FUNCTION(atomic_test_and_set):
miss5:  ldrex       r12, [r0]                       @ load from the address and mark it exclusive
        cmp         r12, r2                         @ compare the value with the comperand(r2)
        strexeq     r3,  r1,  [r0]                  @ if they were equal, attempt to store the new value (r1)
        bne         differ                          @ if they were not equal jump to (differ) which clears the exclusive tag on the address and returns<
        cmp         r3,  #1                         @ check the status of the store (returned in r3)
        beq         miss5                           @ go back to the start if it failed (0=success, 1=failure)
        bne         same                            @ if it succeeded, jump to (same) and return. there is no need to clrex if strex succeeded
differ: clrex                           	    @ clrex
same:   mov         r0,  r12
        bx      lr
FUNCTION_END(atomic_test_and_set)

/* int atomic_get(int *value) 
 */
FUNCTION(atomic_get):
	ldr	r0, [r0]
        bx	lr
FUNCTION_END(atomic_get)

#endif /* ATOMIC_FUNCS_ARE_SYSCALLS */

#ifndef ATOMIC64_FUNCS_ARE_SYSCALLS

/* int64	atomic_add64(vint64 *value, int64 addValue) */
FUNCTION(atomic_add64):
	bx	lr
FUNCTION_END(atomic_add64)

/* int64	atomic_and64(vint64 *value, int64 andValue) */
FUNCTION(atomic_and64):
	bx	lr
FUNCTION_END(atomic_and64)

/* int64	atomic_or64(vint64 *value, int64 orValue) */
FUNCTION(atomic_or64):
	bx	lr
FUNCTION_END(atomic_or64)

/* int64	atomic_set64(vint64 *value, int64 newValue) */
FUNCTION(atomic_set64):
	bx	lr
FUNCTION_END(atomic_set64)

/* int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
FUNCTION(atomic_test_and_set64):
	bx	lr
FUNCTION_END(atomic_test_and_set64)

/* int64	atomic_get64(vint64 *value) */
FUNCTION(atomic_get64):
	bx	lr
FUNCTION_END(atomic_get64)

#endif /* ATOMIC64_FUNCS_ARE_SYSCALLS */
