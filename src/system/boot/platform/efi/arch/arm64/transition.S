/*
 * Copyright 2021-2022, Oliver Ruiz Dorantes. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*-
 * Copyright (c) 2012-2014 Andrew Turner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <asm_defs.h>
#include <kernel/arch/arm64/arm_registers.h>
#include <kernel/arch/arm64/arch_hypervisor.h>


	.text

.macro mreg origin, destination, temporal
    mrs \temporal, \origin
    msr \destination, \temporal
.endm


FUNCTION(_arch_transition_EL2_EL1):
	// Translation Table Base Register
	mreg TTBR0_EL2, TTBR0_EL1, x10
	// Memory Attribute Indirection Register
	mreg MAIR_EL2, MAIR_EL1, x10
	// Vector Base Address Register
	mreg vbar_el2, vbar_el1, x10
	// Migrate SP
	mov x10, sp
	msr sp_el1, x10

	// Enable FP/SIMD
	mov x10, #3 << 20
	msr cpacr_el1, x10

    b drop_to_el1
    // eret will return to caller
FUNCTION_END(_arch_transition_EL2_EL1)


/*
 * If we are started in EL2, configure the required hypervisor
 * registers and drop to EL1.
 */
FUNCTION(drop_to_el1):
	mrs	x1, CurrentEL
	lsr	x1, x1, #2
	cmp	x1, #0x2
	b.eq	1f
	ret
1:
	/* Configure the Hypervisor */
	mov	x2, #(HCR_RW)
	msr	hcr_el2, x2

	/* Load the Virtualization Process ID Register */
	mrs	x2, midr_el1
	msr	vpidr_el2, x2

	/* Load the Virtualization Multiprocess ID Register */
	mrs	x2, mpidr_el1
	msr	vmpidr_el2, x2

	/* Set the bits that need to be 1 in sctlr_el1 */
	ldr	x2, .Lsctlr_res1
	msr	sctlr_el1, x2

	/* Don't trap to EL2 for exceptions */
	mov	x2, #CPTR_RES1
	msr	cptr_el2, x2

	/* Don't trap to EL2 for CP15 traps */
	msr	hstr_el2, xzr

	/* Enable access to the physical timers at EL1 */
	mrs	x2, cnthctl_el2
	orr	x2, x2, #(CNTHCTL_EL1PCTEN | CNTHCTL_EL1PCEN)
	msr	cnthctl_el2, x2

	/* Set the counter offset to a known value */
	msr	cntvoff_el2, xzr

	/* Hypervisor trap functions */
//	adr	x2, hyp_vectors
//	msr	vbar_el2, x2

	mov	x2, #(PSR_F | PSR_I | PSR_A | PSR_D | PSR_M_EL1h)
	msr	spsr_el2, x2

	/* Configure GICv3 CPU interface */
	mrs	x2, id_aa64pfr0_el1
	/* Extract GIC bits from the register */
	ubfx	x2, x2, #ID_AA64PFR0_GIC_SHIFT, #ID_AA64PFR0_GIC_BITS
	/* GIC[3:0] == 0001 - GIC CPU interface via special regs. supported */
	cmp	x2, #(ID_AA64PFR0_GIC_CPUIF_EN >> ID_AA64PFR0_GIC_SHIFT)
	b.ne	2f

	mrs	x2, S3_4_C12_C9_5
	orr	x2, x2, #ICC_SRE_EL2_EN	/* Enable access from insecure EL1 */
	orr	x2, x2, #ICC_SRE_EL2_SRE	/* Enable system registers */
	msr	S3_4_C12_C9_5, x2
2:

	/* Set the address to return to our return address */
	msr	elr_el2, x30
	isb

	eret
FUNCTION_END(drop_to_el1)

/* Macro Definitions */
	.align 3
.Lsctlr_res1:
	.quad SCTLR_RES1
