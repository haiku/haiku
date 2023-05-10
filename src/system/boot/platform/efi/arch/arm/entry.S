/*
 * Copyright 2011, François Revol <revol@free.fr>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


	.arch_extension virt
	.text

/*	status_t arch_enter_kernel(uint32_t ttbr, struct kernel_args *kernelArgs,
		addr_t kernelEntry, addr_t kernelStackTop);

	r0	- ttbr
	r1	- kernelArgs
	r2	- kernelEntry
	r3	- kernelStackTop
*/
FUNCTION(arch_enter_kernel):

	// check whether we are running in HYP mode
	mrs		r9, cpsr
	and		r9, r9, #0x1f

	// proceed to _pl1_entry if we're not in HYP mode
	cmp		r9, #0x1a
	bne		_pl1_entry

	// set SVC mode in SPSR
	mrs		r9, cpsr
	bic		r9, r9, #0x1f
	orr		r9, r9, #0x13
	msr		spsr_cxsf, r9

	// load PL1 entry point address
	adr		lr, _pl1_entry
	msr		elr_hyp, lr

	// drop to PL1 SVC mode
	eret

_pl1_entry:

	mov		r5,r0
	mov		r4,r2

	// set up kernel _start args
	mov		r0,r1	// kernelArgs
	mov		r1,#0	// currentCPU=0

	// enable full access for coprocessors P10, P11
	// by setting the required flags in Access Control Register
	MRC p15, #0, r9, c1, c0, #2
	orr r9, r9, #0x00f00000
	MCR p15, #0, r9, c1, c0, #2

	// flush prefetch buffer
	mov r9, #0
	MCR p15, #0, r9, c7, c5, #4

	// enable FPU
	mov r9, #0x40000000
	FMXR FPEXC, r9

	// flush TLB
	MCR p15, 0, r1, c8, c7, 0

	// set TTBR0
	// cacheability attributes for the page tables are:
	// Normal Memory, Inner/Outer Write-Back no Write-Allocate Cacheable

	// Note that this relies on ARMv7 multiprocessing extensions
	// on uniprocessors we need only the flag 0x01 i.e. Inner Cacheable

	orr		r5, r5, #0x59
	mcr		p15, 0, r5, c2, c0, 0

	// initialize TTBCR to zero (no LPAE, use only TTBR0)
	MCR p15, 0, r1, c2, c0, 2

	// flush TLB (again)
	MCR p15, 0, r1, c8, c7, 0

	// write DACR
	mov		r9, #0x00000001
	mcr		p15, 0, r9, c3, c0, 0

	// enable MMU and caches
	mrc		p15, 0, r9, c1, c0, 0
	orr		r9, r9, #0x20000000		// access flag enabled
	bic		r9, r9, #0x10000000		// TEX remap disabled
	orr		r9, r9, #0x00001000		// i-cache enabled
	orr		r9, r9, #0x00000004		// d-cache enabled
	orr		r9, r9, #0x00000001		// MMU enabled
	mcr		p15, 0, r9, c1, c0, 0

	// set the kernel stack
	mov		sp,r3

	// call the kernel
	mov		pc,r4

	// return
	mov		r0,#-1	// B_ERROR
	mov		pc,lr

FUNCTION_END(arch_enter_kernel)

