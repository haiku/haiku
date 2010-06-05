/*
 * Copyright 2006-2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "asm_offsets.h"

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
	isync
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
	isync
	blr


/* void arch_int_restore_interrupts(int oldState)
 *									r3
 */
FUNCTION(arch_int_restore_interrupts):
	mfmsr	%r4

	rlwimi  %r4, %r3, 0, 31 - MSR_EXCEPTIONS_ENABLED, 31 - MSR_EXCEPTIONS_ENABLED
		// clear or set bit 15, EE to the same state as in r3, oldState

	mtmsr	%r4
	isync
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
	isync
	mtspr		1008, %r3
	isync
	blr

// unsigned int getl2cr();
FUNCTION(getl2cr):
	mfspr		%r3, 1017
	blr

// void setl2cr(unsigned int val);
FUNCTION(setl2cr):
	isync
	mtspr		1017, %r3
	isync
	blr


// void ppc_context_switch(addr_t *old_sp, addr_t new_sp);
FUNCTION(ppc_context_switch):

	// regs to push on the stack: f13-f31, r13-r31, cr, r2, lr

	// push the old regs we need to save on the stack
	// f31-13
	stfdu		%f31, -8(%r1)
	stfdu		%f30, -8(%r1)
	stfdu		%f29, -8(%r1)
	stfdu		%f28, -8(%r1)
	stfdu		%f27, -8(%r1)
	stfdu		%f26, -8(%r1)
	stfdu		%f25, -8(%r1)
	stfdu		%f24, -8(%r1)
	stfdu		%f23, -8(%r1)
	stfdu		%f22, -8(%r1)
	stfdu		%f21, -8(%r1)
	stfdu		%f20, -8(%r1)
	stfdu		%f19, -8(%r1)
	stfdu		%f18, -8(%r1)
	stfdu		%f17, -8(%r1)
	stfdu		%f16, -8(%r1)
	stfdu		%f15, -8(%r1)
	stfdu		%f14, -8(%r1)
	stfdu		%f13, -8(%r1)

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

	// f13-31
	lfdu		%f13, 4(%r1)
	lfdu		%f14, 8(%r1)
	lfdu		%f15, 8(%r1)
	lfdu		%f16, 8(%r1)
	lfdu		%f17, 8(%r1)
	lfdu		%f18, 8(%r1)
	lfdu		%f19, 8(%r1)
	lfdu		%f20, 8(%r1)
	lfdu		%f21, 8(%r1)
	lfdu		%f22, 8(%r1)
	lfdu		%f23, 8(%r1)
	lfdu		%f24, 8(%r1)
	lfdu		%f25, 8(%r1)
	lfdu		%f26, 8(%r1)
	lfdu		%f27, 8(%r1)
	lfdu		%f28, 8(%r1)
	lfdu		%f29, 8(%r1)
	lfdu		%f30, 8(%r1)
	lfdu		%f31, 8(%r1)

	addi		%r1, %r1, 8

	blr


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


/*!	\fn void arch_debug_call_with_fault_handler(cpu_ent* cpu,
		jmp_buf jumpBuffer, void (*function)(void*), void* parameter)

	Called by debug_call_with_fault_handler() to do the dirty work of setting
	the fault handler and calling the function. If the function causes a page
	fault, the arch_debug_call_with_fault_handler() calls longjmp() with the
	given \a jumpBuffer. Otherwise it returns normally.

	debug_call_with_fault_handler() has already saved the CPU's fault_handler
	and fault_handler_stack_pointer and will reset them later, so
	arch_debug_call_with_fault_handler() doesn't need to care about it.

	\param cpu The \c cpu_ent for the current CPU.
	\param jumpBuffer Buffer to be used for longjmp().
	\param function The function to be called.
	\param parameter The parameter to be passed to the function to be called.
*/
FUNCTION(arch_debug_call_with_fault_handler):
	// prolog: setup stack frame (16-byte aligned)
	mflr	%r0
	stw		%r0, 4(%r1)		// store LR
	stwu	%r1, -16(%r1)	// store back chain
	stw		%r4, 8(%r1)		// store jumpBuffer

	// set cpu->fault_handler_stack_pointer
	stw		%r1, CPU_ENT_fault_handler_stack_pointer(%r3)

	// set cpu->fault_handler
	lis		%r11, 1f@ha
	ori		%r11, %r11, 1f@l
	stw		%r11, CPU_ENT_fault_handler(%r3)

	// call the given function
	mr		%r3, %r6
	mtlr	%r5
	blrl
	
	// epilog: restore stack frame
	lwz		%r0, 16 + 4(%r1)	// load LR
	mtlr	%r0
	addi	%r1, %r1, 16		// restore SP

	blr

	// fault -- return via longjmp(jumpBuffer, 1)
1:
	lwz		%r3, 8(%r1) 		// load jumpBuffer

	// call longjmp
	li		%r4, 1
	lis		%r0, longjmp@ha
	ori		%r0, %r0, longjmp@l
	mtlr	%r0
	blr
