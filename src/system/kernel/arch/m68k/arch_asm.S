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

// ToDo: fixme -- platform dependant ?
FUNCTION(reboot):
	reset
	rts


/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
	andi	#0xf8ff,%sr
	rts


/* int arch_int_disable_interrupts(void) 
 */
FUNCTION(arch_int_disable_interrupts):
	clr.l	%d0
	move	%sr,%d0
	move.l	%d0,%d1
	ori.w	#%x0700,%d1
	move	%d1,%sr
	// return value: previous IPM
	lsr.l	#8,%d0
	andi.l	#7,%d0
	rts


/* void arch_int_restore_interrupts(int oldState) 
 */
FUNCTION(arch_int_restore_interrupts):
	move.l	(4,%a7),%d0
	// make sure we only have IPM bits
	andi.w	#7,%d0
	lsl.w	#8,%d0
	move	%sr,%d1
	andi.w	#0xf8ff,%d1
	or.w	%d0,%d1
	move	%d1,%sr
	rts


/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
	clr.l	%d0
	move	%sr,%d1
	andi.w	0x0700,%d1
	bne	arch_int_are_interrupts_enabled_no
	moveq.l	#1,%d0
arch_int_are_interrupts_enabled_no:
	rts


// ToDo: fixme
FUNCTION(dbg_save_registers):
#warning M68K: implement dbx_save_registers!
	rts


/* long long get_time_base(void) */
FUNCTION(get_time_base):
#warning M68K: implement get_time_base!
	clr.l	%d0
	clr.l	%d1
	//passed through a0 or d0:d1 ?
	rts


#warning M68K: checkme
// void m68k_context_switch(addr_t *old_sp, addr_t new_sp);
FUNCTION(m68k_context_switch):
	// save fp ?
	//move.w		%sr,-(%sp)
	movem.l		%d0-%d7/%a0-%a7,-(%sp)
	fpmovem		%fp0-%fp7,-(%sp)
	fpsave		-(%sp)
	
	fprestore	(%sp)+
	fpmovem		(%sp)+,%fp0-%fp7
	movem.l		(%sp)+,%d0-%d7/%a0-%a7
	//move.w		(%sp)+,%sr

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


// void m68k_switch_stack_and_call(addr_t newKstack,
//		void (*func)(void *), void *arg)
FUNCTION(m68k_switch_stack_and_call):
	mr			%r1, %r3	// set the new stack pointer
	mtctr		%r4			// move the target function into CTR
	mr			%r3, %r5	// move the arg to this func to the new arg
	bctr


// m68k_kernel_thread_root(): parameters in r13-r15, the functions to call
// (in that order). The function is used when spawing threads. It usually calls
// an initialization function, the actual thread function, and a function that
// destroys the thread.
FUNCTION(m68k_kernel_thread_root):
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

