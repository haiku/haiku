/*
** Copyright 2003, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#define FUNCTION(x) .global x; .type x,@function; x
#define LOCAL_FUNCTION(x) .type x,@function; x

/*
** General exception handling pseudocode:
**  save %r0 into sprg0
**  save %r2 into sprg1
**  save %r1 into sprg2
**  save cr into %r2 (will mess up cr in the conditional check below)
**  load saved msr into %r0
**  see if was in kernel mode
**  if not,
**   load kernel stack from EAR into %r1
**  restore cr from %r2
**  set up in BAT 0 (instruction and data) a identity mapping of 0x0
**  load old msr
**  merge old msr mmu bits with current msr
**  load new msr (should turn the mmu on)
**  save lr into sprg3
**  set up and branch to the next instruction (moving the program counter into kernel space)
**  remove the BAT mapping
**  set up stack frame and push everything
*/

#define VEC_ENTRY() \
	mtsprg0 %r0					; /* save %r0 */ \
	mtsprg1	%r2					; /* save %r2 */ \
	mtsprg2	%r1					; /* save the old stack */ \
	mfcr	%r2					; /* save cr */ \
\
	mfsrr1	%r0					; /* load saved msr */ \
	andi.	%r0, %r0, (1 << 14)	; /* see if it was in kernel mode */ \
	beq-	0f					; /* yep */ \
\
	/* load the kernel stack */ \
	mfear	%r1					; /* load the kernel stack pointer from the EAR reg */ \
0: \
	mtcrf	0xff, %r2			; /* restore the CR, it was messed up in the previous compare */ \
\
	/* we are going to turn on the mmu, lets have a BAT entry in place to keep us identity mapped */ \
	li		%r0, 0x2				; /* BATU_VS */ \
	mtibatu	0, %r0				; /* load the upper word of the instruction BAT */ \
	mtdbatu	0, %r0				; /* load the upper word of the data BAT */ \
	li		%r0, 0x10|0x2		; /* BATL_MC | BATL_PP_RW */ \
	mtibatl	0, %r0				; /* load the lower word of the instruction BAT */ \
	mtdbatl	0, %r0				; /* load the lower word of the data BAT */ \
	isync						; \
	sync						; \
\
	/* turn the mmu back on */ \
	mfsrr1	%r0					; /* load saved msr */ \
	rlwinm	%r0, %r0, 28, 30, 31	; /* extract mmu bits */ \
	mfmsr	%r2					; /* load the current msr */ \
	rlwimi  %r2, %r0, 4, 26, 27	; /* merge the mmu bits with the current msr */ \
	mtmsr	%r2					; /* load the new msr (turning the mmu back on */ \
	isync						; \
\
	mflr	%r0					; /* load the lr */ \
	mtsprg	3, %r0				; /* save it */ \
	lis		%r0, 1f@h 			; /* load the address of a label in a few instructions */ \
	ori		%r0, %r0, 1f@l		; /* we will jump to it to get the program counter into the kernel region */ \
	mtlr	%r0					; /* get ready to jump to this label */ \
	blr							; /* branch to the next instruction (with the mmu on) */ \
1: \
	/* turn the BAT back off */ \
	li		%r2, 0				; \
	mtibatu	0, %r2				; \
	mtdbatu	0, %r2				; \
	mtibatl	0, %r2				; \
	mtdbatl	0, %r2				; \
	isync						; \
	sync						; \
\
	bl		__save_regs			; /* dump an iframe on the stack */

.global __irqvec_start
__irqvec_start:
	.long	0

/* called by the tail end of the VEC_ENTRY macro
** register expectations:
**  %r1 - stack
**  sprg0 - old %r0
**  sprg1 - old %r2
**  sprg2 - old stack (%r1)
**  sprg3 - old lr
** all other regs should have been unmodified by the exception handler,
** and ready to be saved
*/
FUNCTION(__save_regs):
	mfsprg	%r0, 0
	stwu	%r0, -4(%r1)		/* push %r0 */
	mfsprg	%r0, 2
	stwu	%r0, -4(%r1)		/* push old %r1 (stack) */
	mfsprg	%r0, 1
	stwu	%r0, -4(%r1)		/* push %r2 */
	stwu	%r3, -4(%r1)		/* push %r3-%r31 */
	stwu	%r4, -4(%r1)		/* push %r3-%r31 */
	stwu	%r5, -4(%r1)		/* push %r3-%r31 */
	stwu	%r6, -4(%r1)		/* push %r3-%r31 */
	stwu	%r7, -4(%r1)		/* push %r3-%r31 */
	stwu	%r8, -4(%r1)		/* push %r3-%r31 */
	stwu	%r9, -4(%r1)		/* push %r3-%r31 */
	stwu	%r10, -4(%r1)		/* push %r3-%r31 */
	stwu	%r11, -4(%r1)		/* push %r3-%r31 */
	stwu	%r12, -4(%r1)		/* push %r3-%r31 */

	/* strictly speaking, we dont need to save %r13-%r31, but I will for now */
	stwu	%r13, -4(%r1)		/* push %r3-%r31 */
	stwu	%r14, -4(%r1)		/* push %r3-%r31 */
	stwu	%r15, -4(%r1)		/* push %r3-%r31 */
	stwu	%r16, -4(%r1)		/* push %r3-%r31 */
	stwu	%r17, -4(%r1)		/* push %r3-%r31 */
	stwu	%r18, -4(%r1)		/* push %r3-%r31 */
	stwu	%r19, -4(%r1)		/* push %r3-%r31 */
	stwu	%r20, -4(%r1)		/* push %r3-%r31 */
	stwu	%r21, -4(%r1)		/* push %r3-%r31 */
	stwu	%r22, -4(%r1)		/* push %r3-%r31 */
	stwu	%r23, -4(%r1)		/* push %r3-%r31 */
	stwu	%r24, -4(%r1)		/* push %r3-%r31 */
	stwu	%r25, -4(%r1)		/* push %r3-%r31 */
	stwu	%r26, -4(%r1)		/* push %r3-%r31 */
	stwu	%r27, -4(%r1)		/* push %r3-%r31 */
	stwu	%r28, -4(%r1)		/* push %r3-%r31 */
	stwu	%r29, -4(%r1)		/* push %r3-%r31 */
	stwu	%r30, -4(%r1)		/* push %r3-%r31 */
	stwu	%r31, -4(%r1)		/* push %r3-%r31 */

	/* save some of the other regs */
	mfctr	%r0
	stwu	%r0, -4(%r1)		/* push CTR */
	mfxer	%r0
	stwu	%r0, -4(%r1)		/* push XER */
	mfcr	%r0
	stwu	%r0, -4(%r1)		/* push CR */
	mfsprg	%r0, 3
	stwu	%r0, -4(%r1)		/* push LR */
	mfspr	%r0, %dsisr
	stwu	%r0, -4(%r1)		/* push DSISR */
	mfspr	%r0, %dar
	stwu	%r0, -4(%r1)		/* push DAR */
	mfspr	%r0, %srr1
	stwu	%r0, -4(%r1)		/* push SRR1 */
	mfspr	%r0, %srr0
	stwu	%r0, -4(%r1)		/* push SRR0 */

	addi	%r1, %r1, -8		/* adjust the stack pointer to leave some padding on it for C */

	/* get outta here */
	blr

/* not enough space for __restore_regs_and_rfi here, see below */

.skip	0x100 - (. - __irqvec_start)
FUNCTION(system_reset_exception):
	VEC_ENTRY();
	li		%r3, 0x100
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x200 - (. - __irqvec_start)
FUNCTION(machine_check_exception):
	VEC_ENTRY();
	li		%r3, 0x200
	addi	%r4, %r1, 8
	bl		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x300 - (. - __irqvec_start)
FUNCTION(DSI_exception):
	VEC_ENTRY();
	li		%r3, 0x300
	addi	%r4, %r1, 8
	bl		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x400 - (. - __irqvec_start)
FUNCTION(ISI_exception):
	VEC_ENTRY();
	li		%r3, 0x400
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x500 - (. - __irqvec_start)
FUNCTION(external_interrupt_exception):
	VEC_ENTRY();
	li		%r3, 0x500
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x600 - (. - __irqvec_start)
FUNCTION(alignment_exception):
	VEC_ENTRY();
	li		%r3, 0x600
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x700 - (. - __irqvec_start)
FUNCTION(program_exception):
	VEC_ENTRY();
	li		%r3, 0x700
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x800 - (. - __irqvec_start)
FUNCTION(FP_unavailable_exception):
	VEC_ENTRY();
	li		%r3, 0x800
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x900 - (. - __irqvec_start)
FUNCTION(decrementer_exception):
	VEC_ENTRY();
	li		%r3, 0x900
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

/* called at the tail end of each of the exceptions
** placed here because of the space between these two
** exception handlers.
*/
FUNCTION(__restore_regs_and_rfi):
	addi	%r1, %r1, 8		/* adjust the stack pointer to get it back to the base of the iframe */

	lwz		%r0, 0(%r1)		/* SRR0 */
	mtspr	%srr0, %r0
	lwzu	%r0, 4(%r1)		/* SRR1 */
	mtspr	%srr1, %r0
	lwzu	%r0, 4(%r1)		/* DAR */
	mtspr	%dar, %r0
	lwzu	%r0, 4(%r1)		/* DSISR */
	mtspr	%dsisr, %r0
	lwzu	%r0, 4(%r1)		/* LR */
	mtlr	%r0
	lwzu	%r0, 4(%r1)		/* CR */
	mtcr	%r0
	lwzu	%r0, 4(%r1)		/* XER */
	mtxer	%r0
	lwzu	%r0, 4(%r1)		/* CTR */
	mtctr	%r0

	/* strictly speaking, we dont really need to have saved these regs */
	lwzu	%r31, 4(%r1)
	lwzu	%r30, 4(%r1)
	lwzu	%r29, 4(%r1)
	lwzu	%r28, 4(%r1)
	lwzu	%r27, 4(%r1)
	lwzu	%r26, 4(%r1)
	lwzu	%r25, 4(%r1)
	lwzu	%r24, 4(%r1)
	lwzu	%r23, 4(%r1)
	lwzu	%r22, 4(%r1)
	lwzu	%r21, 4(%r1)
	lwzu	%r20, 4(%r1)
	lwzu	%r19, 4(%r1)
	lwzu	%r18, 4(%r1)
	lwzu	%r17, 4(%r1)
	lwzu	%r16, 4(%r1)
	lwzu	%r15, 4(%r1)
	lwzu	%r14, 4(%r1)
	lwzu	%r13, 4(%r1)

	lwzu	%r12, 4(%r1)
	lwzu	%r11, 4(%r1)
	lwzu	%r10, 4(%r1)
	lwzu	%r9, 4(%r1)
	lwzu	%r8, 4(%r1)
	lwzu	%r7, 4(%r1)
	lwzu	%r6, 4(%r1)
	lwzu	%r5, 4(%r1)
	lwzu	%r4, 4(%r1)
	lwzu	%r3, 4(%r1)
	lwzu	%r2, 4(%r1)
	lwz		%r0, 8(%r1)
	lwz		%r1, 4(%r1)

	/* get out of here */
	rfi

.skip	0xc00 - (. - __irqvec_start)
FUNCTION(system_call_exception):
	VEC_ENTRY();
	li		%r3, 0xc00
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0xd00 - (. - __irqvec_start)
FUNCTION(trace_exception):
	VEC_ENTRY();
	li		%r3, 0xd00
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0xe00 - (. - __irqvec_start)
FUNCTION(FP_assist_exception):
	VEC_ENTRY();
	li		%r3, 0xe00
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0xf00 - (. - __irqvec_start)
FUNCTION(perf_monitor_exception):
	/* XXX deal with this, normal VEC_ENTRY code is too big to fit here */
	rfi

.skip	0xf20 - (. - __irqvec_start)
FUNCTION(altivec_unavailable_exception):
	VEC_ENTRY();
	li		%r3, 0xf20
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x1000 - (. - __irqvec_start)
FUNCTION(ITLB_miss_exception):
	VEC_ENTRY();
	li		%r3, 0x1000
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x1100 - (. - __irqvec_start)
FUNCTION(DTLB_miss_on_load_exception):
	VEC_ENTRY();
	li		%r3, 0x1100
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x1200 - (. - __irqvec_start)
FUNCTION(DTLB_miss_on_store_exception):
	VEC_ENTRY();
	li		%r3, 0x1200
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x1300 - (. - __irqvec_start)
FUNCTION(instruction_address_breakpoint_exception):
	VEC_ENTRY();
	li		%r3, 0x1300
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x1400 - (. - __irqvec_start)
FUNCTION(system_management_exception):
	VEC_ENTRY();
	li		%r3, 0x1400
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x1600 - (. - __irqvec_start)
FUNCTION(altivec_assist_exception):
	VEC_ENTRY();
	li		%r3, 0x1600
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.skip	0x1700 - (. - __irqvec_start)
FUNCTION(thermal_management_exception):
	VEC_ENTRY();
	li		%r3, 0x1700
	addi	%r4, %r1, 8
	bl 		ppc_exception_entry
	bl		__restore_regs_and_rfi

.global __irqvec_end
__irqvec_end:

