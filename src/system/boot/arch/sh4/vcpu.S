/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define SAVE_CPU_STATE 0

.text
.align 2

.globl _vector_base
_vector_base:
	.skip	0x100
exception_ent_1:
	mov.l	expevt_addr1,r0
	mov.l	@r0,r0
	shlr2	r0
	shlr	r0			/* shift the exception code over 3 bits */

	cmp/eq	#0x10,r0		/* test if its a initial page write */
	bt	_tlb_ipw
	nop

	/* not a inital page write exception */
	/* just enter the kernel and let it deal with it */
	bra switch_banks_and_enter_kernel
	nop

_tlb_ipw:
	/* deal with inital page write exception */
	/* we can stay on reg bank 1, switch to a tlb stack */
	mov.l	tlb_stack_addr1,r15

	/* save some of the floating point registers */
	/* they seem to be used by some of the libgcc stuff */
	/* saving: fpscr, fpul, dr0, dr2, dr4 */
	fmov.s	fr0,@-r15
	fmov.s	fr1,@-r15
	fmov.s	fr2,@-r15
	fmov.s	fr3,@-r15
	fmov.s	fr4,@-r15
	fmov.s	fr5,@-r15
	sts.l	fpul,@-r15
	sts.l	fpscr,@-r15

	mov	r0,r4			/* arg 1, excode */
	shlr2	r4			/* shift arg 1 over two more bits */
	mov.l	r4,@-r15		/* save the excode for after the call */
	sts.l	pr,@-r15		/* save the pr reg */
	stc	spc,r5			/* arg 2, spc */
	mov.l	initial_page_write_handler,r1
	jsr	@r1
	nop

	/* restore regs */
	lds.l	@r15+,pr
	mov.l	@r15+,r1
	lds.l	@r15+,fpscr
	lds.l	@r15+,fpul
	fmov.s	@r15+,fr5
	fmov.s	@r15+,fr4
	fmov.s	@r15+,fr3
	fmov.s	@r15+,fr2
	fmov.s	@r15+,fr1
	fmov.s	@r15+,fr0

	/* see if the tlb handler returned another exception code */
	/* if so, the tlb ipw just elevated to a page fault or other such */
	/* 'soft' faults. */

	cmp/eq	r0,r1
	bf	_soft_fault
	nop

	/* return from the tlb miss */
	stc	sgr,r15
	rte
	nop

_soft_fault:
	/* the tlb ipw handler returned another exception code */
	/* we now need to enter the kernel with the soft fault */
	shll2	r0
	mov.l	tea_addr1,r1
	mov.l	@r1,r1		/* load the faulted address */
	bra switch_banks_and_enter_kernel
	nop

.align 2
tea_addr1:	.long	0xff00000c
expevt_addr1:	.long	0xff000024
initial_page_write_handler:	.long	_tlb_initial_page_write
tlb_stack_addr1:	.long	tlb_stack_end-4

exception_ent_1_end:
	.skip 0x300-(exception_ent_1_end-exception_ent_1)
TLB_miss_ent:
	mov.l	expevt_addr2,r4
	mov.l	@r4,r4
	shlr2	r4
	shlr2	r4
	shlr	r4			/* shift the exception code over 5 bits */

#if SAVE_CPU_STATE
	/* dump the entire cpu state */
	mov.l	cpu_state_addr,r0

	stc		r0_bank,r1
	mov.l	r1,@(0,r0)
	stc		r1_bank,r1
	mov.l	r1,@(4,r0)
	stc		r2_bank,r1
	mov.l	r1,@(8,r0)
	stc		r3_bank,r1
	mov.l	r1,@(12,r0)
	stc		r4_bank,r1
	mov.l	r1,@(16,r0)
	stc		r5_bank,r1
	mov.l	r1,@(20,r0)
	stc		r6_bank,r1
	mov.l	r1,@(24,r0)
	stc		r7_bank,r1
	mov.l	r1,@(28,r0)

	mov.l	r8,@(32,r0)
	mov.l	r9,@(36,r0)
	mov.l	r10,@(40,r0)
	mov.l	r11,@(44,r0)
	mov.l	r12,@(48,r0)
	mov.l	r13,@(52,r0)
	mov.l	r14,@(56,r0)

	mov.l	r15,@(60,r0)

	add		#64,r0

	stc		gbr,r1
	mov.l	r1,@(0,r0)
	sts		mach,r1
	mov.l	r1,@(4,r0)
	sts		macl,r1
	mov.l	r1,@(8,r0)
	sts		pr,r1
	mov.l	r1,@(12,r0)

	stc		spc,r1
	mov.l	r1,@(16,r0)
	stc		sgr,r1
	mov.l	r1,@(20,r0)
	stc		ssr,r1
	mov.l	r1,@(24,r0)

	sts		fpul,r1
	mov.l	r1,@(28,r0)
	sts		fpscr,r1
	mov.l	r1,@(32,r0)

#endif

	/* we can stay on reg bank 1, switch to a tlb stack */
	mov.l	tlb_stack_addr2,r15

	/* save some of the floating point registers */
	/* they seem to be used by some of the libgcc stuff */
	/* saving: fpscr, fpul, dr0, dr2, dr4 */
	fmov.s	fr0,@-r15
	fmov.s	fr1,@-r15
	fmov.s	fr2,@-r15
	fmov.s	fr3,@-r15
	fmov.s	fr4,@-r15
	fmov.s	fr5,@-r15
	sts.l	fpul,@-r15
	sts.l	fpscr,@-r15

	mov.l	r4,@-r15		/* save the exception code */
	sts.l	pr,@-r15		/* save the pr reg */

	/* arg 1 is exception code already in r4 */
	stc	spc,r5			/* arg 2, spc */
	mov.l	tlb_miss_handler,r1
	jsr	@r1
	nop

	/* see if the tlb miss handler returned another exception code */
	/* if so, the tlb miss just elevated to a page fault or other such */
	/* 'soft' faults. */
	lds.l	@r15+,pr		/* restore pr */
	mov.l	@r15+,r1		/* restore the original exception code */

	/* restore the saved floating point registers */
	lds.l	@r15+,fpscr
	lds.l	@r15+,fpul
	fmov.s	@r15+,fr5
	fmov.s	@r15+,fr4
	fmov.s	@r15+,fr3
	fmov.s	@r15+,fr2
	fmov.s	@r15+,fr1
	fmov.s	@r15+,fr0

	cmp/eq	r0,r1			/* check against the stored original exception code */
	bf	_soft_fault1
	nop

	/* return from the tlb miss */
	stc	sgr,r15
	rte
	nop

_soft_fault1:
	/* the tlb miss handler returned another exception code */
	/* we now need to enter the kernel with the soft fault */

	shll2	r0
	mov.l	tea_addr2,r1
	mov.l	@r1,r1		/* load the faulted address */
	bra switch_banks_and_enter_kernel
	nop

.align 2
tea_addr2:	.long	0xff00000c
expevt_addr2:	.long	0xff000024
tlb_miss_handler:	.long	_tlb_miss
tlb_stack_addr2:	.long	tlb_stack_end-4
#if SAVE_CPU_STATE
cpu_state_addr:	.long	_last_ex_cpu_state
#endif

TLB_miss_ent_end:
	.skip 0x200-(TLB_miss_ent_end-TLB_miss_ent)
interrupt_ent:
	mov.l	intevt_addr,r0
	mov.l	@r0,r0
	shlr2	r0
	shlr	r0			/* shift the exception code over 3 bits */

	/*
	** args to here are
	** r0: exception number (shifted left 3 bits from the cpus version
	** r1: page fault address (if applicable, it still gets saved)
	*/
switch_banks_and_enter_kernel:
	/* disable interrupts */
	mov.l	imask,r3
	stc		sr,r2
	or		r3,r2
	ldc		r2,sr

	/* check to see if we are not already using the saved_* locations */
	mov.l	is_pushing_registers,r2
	mov		#1,r3
	and		r2,r3
	bf		_save_regs

	/* okay, we are in a bad state right now.
	** apparently we were already in the process of moving saved registers
	** from the saved_* locations. This will happen if we take a page fault
	** while pushing registers to the kernel stack, for example. We are toast.
	*/
	mov.l	tlb_stack_addr3,r15
	mov.l	reentrant_fault_handler,r2
	jsr		@r2
	nop

_save_regs:
	/* save the saved registers into memory */
	/* this ends up with the important registers in saved_spc and friends */
	mov.l	is_pushing_registers_addr,r2
	mov		#1,r3
	mov.l	r3,@r2

	mov.l	save_stack,r2
	stc.l	spc,@-r2
	stc.l	ssr,@-r2
	stc.l	sgr,@-r2
	mov.l	r0,@-r2			/* put the modified exception code there too */
	mov.l	r1,@-r2			/* put the saved page fault address */

	/* see if we need to load the kernel stack or stay on the current one */
	stc		ssr,r0
	mov.l	md_bit_mask,r1
	and		r1,r0
	cmp/eq	r1,r0
	bt		_keep_current_stack
	nop

	/* we need to set the stack because we came from user space */
	mov.l	kstack,r15
	bra		_have_set_stack
	nop

_keep_current_stack:
	stc		sgr,r15

_have_set_stack:
	/* enable exceptions & swap banks back to 0 */
	mov.l	bl_rb_bit_mask,r0
	stc		sr,r1
	and		r0,r1
	ldc		r1,sr

	/*
	** From now on we can take exceptions, though taking any between now
	** and the time at which we move saved stuff out of saved_sgr and other
	** fixed save points would be fatal. We are only pushing it on the kernel
	** stack, which has to be present always anyway, so its not a problem, since
	** a page fault is the only one we can realistically take right now anyway.
	*/

	/* start pushing registers */
	mov.l	r8,@-r15
	mov.l	r9,@-r15
	mov.l	r10,@-r15
	mov.l	r11,@-r15
	mov.l	r12,@-r15
	mov.l	r13,@-r15
	mov.l	r14,@-r15

	/* push r0-r7 */
	mov.l	r0,@-r15
	mov.l	r1,@-r15
	mov.l	r2,@-r15
	mov.l	r3,@-r15
	mov.l	r4,@-r15
	mov.l	r5,@-r15
	mov.l	r6,@-r15
	mov.l	r7,@-r15

_after_r0r7_save:
	/* save the floating point registers */
	/* XXX see about optimizing this later */
	fmov.s	fr0,@-r15
	fmov.s	fr1,@-r15
	fmov.s	fr2,@-r15
	fmov.s	fr3,@-r15
	fmov.s	fr4,@-r15
	fmov.s	fr5,@-r15
	fmov.s	fr6,@-r15
	fmov.s	fr7,@-r15
	fmov.s	fr8,@-r15
	fmov.s	fr9,@-r15
	fmov.s	fr10,@-r15
	fmov.s	fr11,@-r15
	fmov.s	fr12,@-r15
	fmov.s	fr13,@-r15
	fmov.s	fr14,@-r15
	fmov.s	fr15,@-r15
	frchg
	fmov.s	fr0,@-r15
	fmov.s	fr1,@-r15
	fmov.s	fr2,@-r15
	fmov.s	fr3,@-r15
	fmov.s	fr4,@-r15
	fmov.s	fr5,@-r15
	fmov.s	fr6,@-r15
	fmov.s	fr7,@-r15
	fmov.s	fr8,@-r15
	fmov.s	fr9,@-r15
	fmov.s	fr10,@-r15
	fmov.s	fr11,@-r15
	fmov.s	fr12,@-r15
	fmov.s	fr13,@-r15
	fmov.s	fr14,@-r15
	fmov.s	fr15,@-r15
	frchg
	sts.l	fpul,@-r15
	sts.l	fpscr,@-r15

	/* can save most of the special registers we need in r8-r14 */
	/* the abi we are working with saves r8-14 on function calls */
	stc.l	gbr,@-r15
	sts.l	mach,@-r15
	sts.l	macl,@-r15
	sts.l	pr,@-r15
	mov.l	saved_sgr,r12
	mov.l	r12,@-r15
	mov.l	saved_ssr,r12
	mov.l	r12,@-r15
	mov.l	saved_spc,r12
	mov.l	r12,@-r15
	mov.l	saved_excode,r12
	shlr2	r12
	mov.l	r12,@-r15
	mov.l	saved_pfault,r12
	mov.l	r12,@-r15

	/* record that we are not in a critical section now where we cant */
	/* take an exception */
	mov.l	is_pushing_registers_addr,r2
	mov		#0,r3
	mov.l	r3,@r2

	/* arg1: address of the iframe */
	mov		r15,r4

	/* jump through the vector table into the kernel */
	mov.l	vector_table_addr,r0
	mov.l	saved_excode,r2
	mov.l	@(r0,r2),r1
	jsr	@r1
	nop

	/* entering a critical section now where we cant */
	/* take an exception */
	mov.l	is_pushing_registers_addr,r2
	mov		#1,r3
	mov.l	r3,@r2

	/* restore everything and get outta here */
	add		#0x8,r15		/* pop the pfault and excode data from the stack */
	mov.l	saved_spc_addr,r1
	mov.l	@r15+,r2
	mov.l	r2,@r1
	mov.l	saved_ssr_addr,r1
	mov.l	@r15+,r2
	mov.l	r2,@r1
	mov.l	saved_sgr_addr,r1
	mov.l	@r15+,r2
	mov.l	r2,@r1
	lds.l	@r15+,pr
	lds.l	@r15+,macl
	lds.l	@r15+,mach
	ldc.l	@r15+,gbr

	/* restore the floating point registers */
	lds.l	@r15+,fpscr
	lds.l	@r15+,fpul
	frchg
	fmov.s	@r15+,fr15
	fmov.s	@r15+,fr14
	fmov.s	@r15+,fr13
	fmov.s	@r15+,fr12
	fmov.s	@r15+,fr11
	fmov.s	@r15+,fr10
	fmov.s	@r15+,fr9
	fmov.s	@r15+,fr8
	fmov.s	@r15+,fr7
	fmov.s	@r15+,fr6
	fmov.s	@r15+,fr5
	fmov.s	@r15+,fr4
	fmov.s	@r15+,fr3
	fmov.s	@r15+,fr2
	fmov.s	@r15+,fr1
	fmov.s	@r15+,fr0
	frchg
	fmov.s	@r15+,fr15
	fmov.s	@r15+,fr14
	fmov.s	@r15+,fr13
	fmov.s	@r15+,fr12
	fmov.s	@r15+,fr11
	fmov.s	@r15+,fr10
	fmov.s	@r15+,fr9
	fmov.s	@r15+,fr8
	fmov.s	@r15+,fr7
	fmov.s	@r15+,fr6
	fmov.s	@r15+,fr5
	fmov.s	@r15+,fr4
	fmov.s	@r15+,fr3
	fmov.s	@r15+,fr2
	fmov.s	@r15+,fr1
	fmov.s	@r15+,fr0

	mov.l	@r15+,r7
	mov.l	@r15+,r6
	mov.l	@r15+,r5
	mov.l	@r15+,r4
	mov.l	@r15+,r3
	mov.l	@r15+,r2
	mov.l	@r15+,r1
	mov.l	@r15+,r0

_after_r0r7_restore:
	/* We need to calculate a new sr with exceptions off & register bank 1*/
	mov.l	bl_rb_bit_mask,r8
	not	r8,r8
	stc	sr,r9
	or	r9,r8
	mov.l	modified_sr_addr,r9
	mov.l	r8,@r9

	/* pop the last few registers */
	mov.l	@r15+,r14
	mov.l	@r15+,r13
	mov.l	@r15+,r12
	mov.l	@r15+,r11
	mov.l	@r15+,r10
	mov.l	@r15+,r9
	mov.l	@r15+,r8

	/* now we only have r15 to use */
	mov.l	modified_sr,r15
	ldc	r15,sr

	/* restore the ssr & spc registers */
	mov.l	saved_spc,r15
	ldc	r15,spc
	mov.l	saved_ssr,r15
	ldc	r15,ssr
	mov.l	saved_sgr,r15

	/* record that we are not in a critical section now where we cant */
	/* take an exception */
	mov.l	is_pushing_registers_addr,r2
	mov		#0,r3
	mov.l	r3,@r2

	/* get out of here */
	rte
	nop

.align 2
reentrant_fault_handler:	.long	_reentrant_fault
modified_sr_addr:	.long	modified_sr
vector_table_addr: .long	vector_table
trap_exception:	.long	0x2c
vector_base_addr: .long	_vector_base
md_bit_mask:	.long	0x40000000
bl_rb_bit_mask:	.long	0xcfffffff
saved_page_fault_addr: .long	saved_pfault
saved_excode_addr: .long	saved_excode
saved_spc_addr:	.long	saved_spc
saved_ssr_addr:	.long	saved_ssr
saved_sgr_addr:	.long	saved_sgr
tlb_stack_addr3:	.long	tlb_stack_end-4
intevt_addr:	.long	0xff000028
tra_addr:	.long	0xff000020
imask:		.long	0x000000f0

/* the next memory addresses are used for temporary storage or
   data structures for the kernel to write in. They are in the
   text segment, I know, but its much faster if we can reference
   them pcrel. */
saved_pfault:	.long	0
saved_excode:	.long	0
saved_sgr:	.long	0
saved_ssr:	.long	0
saved_spc:	.long	0		/* these four addresses are used to
					save the saved registers while we switch to another
					stack and register set */
save_stack:	.long	saved_spc+4
modified_sr:	.long	0

/* stores whether or not we are in a state at which we cannot take a fault */
is_pushing_registers:	.long	0
is_pushing_registers_addr:	.long	is_pushing_registers

/* These memory locations are written to by the kernel */
.globl _kernel_struct
_kernel_struct:
kernel_pgdir:	.long	0
user_pgdir:	.long	0
kernel_asid:	.long	0
user_asid:	.long 	0
kstack:		.long	0
vector_table:
	.rep	0x100
	.long	0
	.endr

.data
.align 4
tlb_stack:
	.rep	0x1000
	.long	0
	.endr
tlb_stack_end:

#if SAVE_CPU_STATE
.align 4
.global _last_ex_cpu_state
_last_ex_cpu_state:
	.rep	0x1000
	.long	99
	.endr
#endif

