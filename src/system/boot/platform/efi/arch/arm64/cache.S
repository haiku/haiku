/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#include <asm_defs.h>
#include <kernel/arch/arm64/arm_registers.h>

	.text

/* Based on 12.1 The Translation Lookaside Buffer
 * ARM DEN0024A (ID050815)
 */
FUNCTION(_arch_mmu_invalidate_tlb_all):
    dsb st /* ensure write has completed*/
//  cmp	x0, 3
//  b.eq	el3
	cmp	x0, 2
	b.eq   el2
	cmp	x0, 1
	b.eq   el1

el3:
	tlbi alle3
	dsb sy
	isb
	ret
el2:
	tlbi alle2
	dsb sy
	isb
	ret
el1:
	tlbi vmalle1
	dsb sy
	isb
	ret
FUNCTION_END(_arch_mmu_invalidate_tlb_all)


/* Based on Example 11-3 Cleaning to Point of Coherency
 * ARM DEN0024A (ID050815)
 */
FUNCTION(_arch_cache_clean_poc):
    MRS X0, CLIDR_EL1
    AND W3, W0, #0x07000000    // Get 2 x Level of Coherence
    LSR W3, W3, #23
    CBZ W3, Finished
    MOV W10, #0    // W10 = 2 x cache level
    MOV W8, #1    //    W8 = constant 0b1
Loop1:
    ADD W2, W10, W10, LSR #1    // Calculate 3 x cache level
    LSR W1, W0, W2    // extract 3-bit cache type for this level
    AND W1, W1, #0x7
    CMP W1, #2
    B.LT Skip    // No data or unified cache at this level
    MSR CSSELR_EL1, X10    //    Select this cache level
    ISB    //    Synchronize change of CSSELR
    MRS X1, CCSIDR_EL1    //    Read CCSIDR
    AND W2, W1, #7    //    W2 = log2(linelen)-4
    ADD W2, W2, #4    //    W2 = log2(linelen)
    UBFX W4, W1, #3, #10    //  W4 = max way number, right aligned
    CLZ W5, W4    /* W5 = 32-log2(ways), bit position of way in DC operand */
    LSL W9, W4, W5 /* W9 = max way number, aligned to position in DC operand */
    LSL W16, W8, W5 // W16 = amount to decrement way number per iteration
Loop2:
    UBFX W7, W1, #13, #15 // W7 = max set number, right aligned
    LSL W7, W7, W2    /* W7 = max set number, aligned to position in DC operand */
    LSL W17, W8, W2    // W17 = amount to decrement set number per iteration
Loop3:
    ORR W11, W10, W9 // W11 = combine way number and cache number...
    ORR W11, W11, W7 // ... and set number for DC operand
    DC CSW, X11    // Do data cache clean by set and way
    SUBS W7, W7, W17 // Decrement set number
    B.GE Loop3
    SUBS X9, X9, X16 // Decrement way number
    B.GE Loop2
Skip:
    ADD W10, W10, #2 // Increment 2 x cache level
    CMP W3, W10
    DSB  sy  /* Ensure completion of previous cache maintenance operation */
    B.GT Loop1
Finished:
    ret
FUNCTION_END(_arch_cache_clean_poc)
