/*
** Copyright 2003, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#define FUNCTION(x) .global x; .type x,@function; x

.text

// ToDo: fixme
FUNCTION(reboot):
    b   .


/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
	mfmsr	%r3							// load msr

/*
	li		%r4, 1
	insrwi	%r3, %r4, 1, 16				// set the EE bit
*/

	li		%r4, 0
	ori		%r4, %r4, (1 << 15)			// create a mask for EE bit of MSR
	or		%r3, %r3, %r4				// set the EE bit

	mtmsr	%r3							// put it back into the msr
	blr

/* void arch_int_disable_interrupts(void) */
FUNCTION(arch_int_disable_interrupts):
	mfmsr	%r3							// load msr

/*
	li		%r4, 0
	rlwinm	%r3, %r4, 15, 16, 16		// mask the EE bit
*/

	li		%r4, 0
	ori		%r4, %r4, (1 << 15)			// create a mask for EE bit of MSR
	andc	%r3, %r3, %r4				// mask the EE bit

	mtmsr	%r3							// put it back into the msr
	blr

/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
	mfmsr	%r3							// load msr
	extrwi	%r3, %r3, 1, 16				// mask out the EE bit
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
#define SAVE_FRAME_SIZE (((31 - 13 + 1) + 1 + 1 + 1) * 4)

	// push the old regs we need to save on the stack
	addi		%sp, %sp, -SAVE_FRAME_SIZE
	stmw		%r13, 12(%sp)
	stw			%r2, 8(%sp)
	mfcr		%r0
	stw			%r0, 4(%sp)
	mflr		%r0
	stw			%r0, 0(%sp)

	// save the old stack pointer
	stw			%sp, 0(%r3)

	// restore the new stack pointer
	mr			%sp, %r4

	// restore the new regs
	lwz			%r0, 0(%sp)
	mtlr		%r0
	lwz			%r0, 4(%sp)
	mtcr		%r0
	lwz			%r2, 8(%sp)
	lmw			%r13, 12(%sp)
	addi		%sp, %sp, SAVE_FRAME_SIZE

	blr

// void arch_thread_switch_kstack_and_call(addr_t new_kstack, void (*func)(void *), void *arg)
FUNCTION(arch_thread_switch_kstack_and_call):
	mr			%sp, %r3	// set the new stack pointer
	mtctr		%r4			// move the target function into CTR
	mr			%r3, %r5	// move the arg to this func to the new arg
	bctr

