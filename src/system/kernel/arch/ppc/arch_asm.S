/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#define FUNCTION(x) .global x; .type x,@function; x

#define MSR_EXCEPTIONS_ENABLED 15

.text

// ToDo: fixme
FUNCTION(reboot):
    b   .


/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
	mfmsr	%r3							// load msr

	li		%r4, 1
	insrwi  %r3, %r4, 1, 31 - MSR_EXCEPTIONS_ENABLED
		// sets bit 15, EE

	mtmsr	%r3							// put it back into the msr
	blr


/* int arch_int_disable_interrupts(void) 
 * r3
 */
FUNCTION(arch_int_disable_interrupts):
	mfmsr	%r4							// load msr

	mr		%r3, %r4					// save old state
	rlwinm  %r4, %r4, 0, 32 - MSR_EXCEPTIONS_ENABLED, 30 - MSR_EXCEPTIONS_ENABLED
		// clears bit 15, EE

	mtmsr	%r4							// put it back into the msr
	blr


/* void arch_int_restore_interrupts(int oldState) 
 *									r3
 */
FUNCTION(arch_int_restore_interrupts):
	mfmsr	%r4

	rlwimi  %r4, %r3, 0, 31 - MSR_EXCEPTIONS_ENABLED, 31 - MSR_EXCEPTIONS_ENABLED
		// clear or set bit 15, EE to the same state as in r3, oldState

	mtmsr	%r4
	blr

/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
	mfmsr	%r3							// load msr
	extrwi	%r3, %r3, 1, 31 - MSR_EXCEPTIONS_ENABLED				
		// mask out the EE bit
	blr


// ToDo: fixme
FUNCTION(dbg_save_registers):
	blr

/* long long get_time_base(void) */
FUNCTION(get_time_base):
1:
	mftbu	%r3							// get the upper time base register
	mftb	%r4							// get the lower time base register
	mftbu	%r5							// get the upper again
	cmpw	%r5, %r3					// see if it changed while we were reading the lower
	bne-	1b							// if so, repeat
	blr

/* void getibats(int bats[8]); */
FUNCTION(getibats):
	mfibatu 	%r0,0
	stw     	%r0,0(%r3)
	mfibatl 	%r0,0
	stwu		%r0,4(%r3)
	mfibatu 	%r0,1
	stwu		%r0,4(%r3)
	mfibatl 	%r0,1
	stwu		%r0,4(%r3)
	mfibatu 	%r0,2
	stwu		%r0,4(%r3)
	mfibatl 	%r0,2
	stwu		%r0,4(%r3)
	mfibatu 	%r0,3
	stwu		%r0,4(%r3)
	mfibatl 	%r0,3
	stwu		%r0,4(%r3)
	blr

// void setibats(int bats[8]);
FUNCTION(setibats):
	lwz			%r0,0(%r3)
	mtibatu 	0,%r0
	isync
	lwzu		%r0,4(%r3)
	mtibatl 	0,%r0
	isync
	lwzu		%r0,4(%r3)
	mtibatu 	1,%r0
	isync
	lwzu		%r0,4(%r3)
	mtibatl 	1,%r0
	isync
	lwzu		%r0,4(%r3)
	mtibatu 	2,%r0
	isync
	lwzu		%r0,4(%r3)
	mtibatl 	2,%r0
	isync
	lwzu		%r0,4(%r3)
	mtibatu 	3,%r0
	isync
	lwzu		%r0,4(%r3)
	mtibatl 	3,%r0
	isync

	blr

// void getdbats(int bats[8]);
FUNCTION(getdbats):
	mfdbatu 	%r0,0
	stw     	%r0,0(%r3)
	mfdbatl 	%r0,0
	stwu		%r0,4(%r3)
	mfdbatu 	%r0,1
	stwu		%r0,4(%r3)
	mfdbatl 	%r0,1
	stwu		%r0,4(%r3)
	mfdbatu 	%r0,2
	stwu		%r0,4(%r3)
	mfdbatl 	%r0,2
	stwu		%r0,4(%r3)
	mfdbatu		%r0,3
	stwu		%r0,4(%r3)
	mfdbatl 	%r0,3
	stwu		%r0,4(%r3)
	blr

// void setdbats(int bats[8]);
FUNCTION(setdbats):
	lwz			%r0,0(%r3)
	mtdbatu 	0,%r0
	lwzu		%r0,4(%r3)
	mtdbatl 	0,%r0
	lwzu		%r0,4(%r3)
	mtdbatu 	1,%r0
	lwzu		%r0,4(%r3)
	mtdbatl 	1,%r0
	lwzu		%r0,4(%r3)
	mtdbatu 	2,%r0
	lwzu		%r0,4(%r3)
	mtdbatl 	2,%r0
	lwzu		%r0,4(%r3)
	mtdbatu 	3,%r0
	lwzu		%r0,4(%r3)
	mtdbatl 	3,%r0
	sync

	blr

// unsigned int gethid0();
FUNCTION(gethid0):
	mfspr		%r3, 1008
	blr

// void sethid0(unsigned int val);
FUNCTION(sethid0):
	mtspr		1008, %r3
	blr

// unsigned int getl2cr();
FUNCTION(getl2cr):
	mfspr		%r3, 1017
	blr

// void setl2cr(unsigned int val);
FUNCTION(setl2cr):
	mtspr		1017, %r3
	blr

	
// void ppc_context_switch(addr_t *old_sp, addr_t new_sp);
FUNCTION(ppc_context_switch):

	// regs to push on the stack: r13-r31, cr, r2, lr

	// push the old regs we need to save on the stack
	// r31-13, r2
	stwu		%r31, -4(%r1)
	stwu		%r30, -4(%r1)
	stwu		%r29, -4(%r1)
	stwu		%r28, -4(%r1)
	stwu		%r27, -4(%r1)
	stwu		%r26, -4(%r1)
	stwu		%r25, -4(%r1)
	stwu		%r24, -4(%r1)
	stwu		%r23, -4(%r1)
	stwu		%r22, -4(%r1)
	stwu		%r21, -4(%r1)
	stwu		%r20, -4(%r1)
	stwu		%r19, -4(%r1)
	stwu		%r18, -4(%r1)
	stwu		%r17, -4(%r1)
	stwu		%r16, -4(%r1)
	stwu		%r15, -4(%r1)
	stwu		%r14, -4(%r1)
	stwu		%r13, -4(%r1)
	stwu		%r2, -4(%r1)

	// CR and LR
	mfcr		%r0
	stwu		%r0, -4(%r1)
	mflr		%r0
	stwu		%r0, -4(%r1)

	// save the old stack pointer
	stwu		%r1, 0(%r3)

	// restore the new stack pointer
	mr			%r1, %r4

	// restore the new regs
	// LR and CR
	lwz			%r0, 0(%r1)
	mtlr		%r0
	lwzu		%r0, 4(%r1)
	mtcr		%r0

	// r2, r13-31
	lwzu		%r2, 4(%r1)
	lwzu		%r13, 4(%r1)
	lwzu		%r14, 4(%r1)
	lwzu		%r15, 4(%r1)
	lwzu		%r16, 4(%r1)
	lwzu		%r17, 4(%r1)
	lwzu		%r18, 4(%r1)
	lwzu		%r19, 4(%r1)
	lwzu		%r20, 4(%r1)
	lwzu		%r21, 4(%r1)
	lwzu		%r22, 4(%r1)
	lwzu		%r23, 4(%r1)
	lwzu		%r24, 4(%r1)
	lwzu		%r25, 4(%r1)
	lwzu		%r26, 4(%r1)
	lwzu		%r27, 4(%r1)
	lwzu		%r28, 4(%r1)
	lwzu		%r29, 4(%r1)
	lwzu		%r30, 4(%r1)
	lwzu		%r31, 4(%r1)

	addi		%r1, %r1, 4

	blr


// void arch_thread_switch_kstack_and_call(addr_t new_kstack,
//		void (*func)(void *), void *arg)
FUNCTION(arch_thread_switch_kstack_and_call):
	mr			%r1, %r3	// set the new stack pointer
	mtctr		%r4			// move the target function into CTR
	mr			%r3, %r5	// move the arg to this func to the new arg
	bctr


// ppc_kernel_thread_root(): parameters in r13-r15, the functions to call
// (in that order). The function is used when spawing threads. It usually calls
// an initialization function, the actual thread function, and a function that
// destroys the thread.
FUNCTION(ppc_kernel_thread_root):
	mtlr		%r13
	blrl
	mtlr		%r14
	blrl
	mtlr		%r15
	blrl

	// We should never get here. If we do, it's time to enter the kernel
	// debugger (without a message at the moment).
	li			%r3, 0
	b			kernel_debugger

