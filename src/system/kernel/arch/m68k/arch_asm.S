/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/m68k/arch_cpu.h>

#define FUNCTION(x) .global x; .type x,@function; x

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
	ori.w	#0x0700,%d1
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
	andi.w	#0x0700,%d1
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


#warning M68K: FIX m68k_context_switch
// XXX:sync with arch_thread.c:arch_thread_init_kthread_stack
// void m68k_context_switch(addr_t *old_sp, addr_t new_sp);
FUNCTION(m68k_context_switch):
	// save fp ?
	//move.w		%sr,-(%sp)
	movem.l		%d0-%d7/%a0-%a7,-(%sp)
	fmovem		%fp0-%fp7,-(%sp)
	fsave		-(%sp)
#warning M68K: use fixed size for fsave

	// XXX
	
	frestore	(%sp)+
	fmovem		(%sp)+,%fp0-%fp7
	movem.l		(%sp)+,%d0-%d7/%a0-%a7
	//move.w		(%sp)+,%sr

	rts


// void m68k_switch_stack_and_call(addr_t newKstack,
//		void (*func)(void *), void *arg)
FUNCTION(m68k_switch_stack_and_call):
#warning M68K: check
	move.l		4(%sp),%a0	// new stack
	move.l		8(%sp),%a1	// func
	move.l		12(%sp),%d0	// args
	move.l		%a0,%sp		// switch the stack
	move.l		%d0,-(%sp)	// push the argument
	jsr		(%a1)		// call the target function
_loop:
	bra		_loop


// m68k_kernel_thread_root(): parameters in r13-r15, the functions to call
// (in that order). The function is used when spawing threads. It usually calls
// an initialization function, the actual thread function, and a function that
// destroys the thread.
FUNCTION(m68k_kernel_thread_root):
#warning M68K: check
	move.l		4(%sp),%a0
	jsr		(%a0)
	move.l		8(%sp),%a0
	jsr		(%a0)
	move.l		12(%sp),%a0
	jsr		(%a0)

	// We should never get here. If we do, it's time to enter the kernel
	// debugger (without a message at the moment).
	clr.l		-(%sp)
	jmp		kernel_debugger

