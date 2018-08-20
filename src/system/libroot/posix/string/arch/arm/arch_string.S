/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


.align 4
FUNCTION(memcpy):
FUNCTION(__aeabi_memcpy):
        push    {r4, r5, r6, r7, r8, r10, r11, lr}
        add     r11, sp, #24
        cmp     r0, r1
        cmpne   r2, #0
        bne     .LBB0_2
.LBB0_1:
        pop     {r4, r5, r6, r7, r8, r10, r11, pc}
.LBB0_2:
        orr     r3, r1, r0
        tst     r3, #3
        beq     .LBB0_9
        eor     r3, r1, r0
        and     r7, r0, #3
        tst     r3, #3
        mov     r12, r2
        rsbeq   r12, r7, #4
        cmp     r2, #4
        movlo   r12, r2
        sub     r2, r2, r12
        cmp     r12, #1
        blt     .LBB0_9
        mvn     r7, r12
        cmn     r7, #2
        mvn     r3, #1
        mvn     r6, #1
        movgt   r3, r7
        add     r3, r12, r3
        add     r3, r3, #2
        cmp     r3, #16
        blo     .LBB0_22
        cmn     r7, #2
        movgt   r6, r7
        add     r7, r12, r6
        add     r7, r7, #2
        add     r6, r1, r7
        cmp     r6, r0
        addhi   r7, r0, r7
        cmphi   r7, r1
        bhi     .LBB0_22
        bic     r4, r3, #15
        mov     r7, r0
        add     r5, r0, r4
        add     lr, r1, r4
        sub     r12, r12, r4
        mov     r6, r4
.LBB0_7:
        vld1.8  {d16, d17}, [r1]!
        subs    r6, r6, #16
        vst1.8  {d16, d17}, [r7]!
        bne     .LBB0_7
        cmp     r3, r4
        bne     .LBB0_23
        b       .LBB0_25
.LBB0_9:
        mov     r5, r0
        mov     r3, #0
        cmp     r3, r2, lsr #2
        beq     .LBB0_19
.LBB0_10:
        bic     r12, r2, #3
        lsr     r3, r2, #2
        cmp     r2, #16
        blo     .LBB0_15
        add     r7, r1, r12
        cmp     r5, r7
        addlo   r7, r5, r12
        cmplo   r1, r7
        blo     .LBB0_15
        and     lr, r3, #3
        mov     r7, r1
        sub     r8, r3, lr
        mov     r6, r5
        sub     r3, r12, lr, lsl #2
        add     r4, r5, r3
        add     r3, r1, r3
.LBB0_13:
        vld1.32 {d16, d17}, [r7]!
        subs    r8, r8, #4
        vst1.32 {d16, d17}, [r6]!
        bne     .LBB0_13
        cmp     lr, #0
        bne     .LBB0_16
        b       .LBB0_18
.LBB0_15:
        mov     lr, r3
        mov     r3, r1
        mov     r4, r5
.LBB0_16:
        add     r6, lr, #1
.LBB0_17:
        ldr     r7, [r3], #4
        sub     r6, r6, #1
        str     r7, [r4], #4
        cmp     r6, #1
        bgt     .LBB0_17
.LBB0_18:
        add     r5, r5, r12
        add     r1, r1, r12
.LBB0_19:
        ands    r2, r2, #3
        beq     .LBB0_1
        add     r2, r2, #1
.LBB0_21:
        ldrb    r3, [r1], #1
        sub     r2, r2, #1
        strb    r3, [r5], #1
        cmp     r2, #1
        bgt     .LBB0_21
        b       .LBB0_1
.LBB0_22:
        mov     lr, r1
        mov     r5, r0
.LBB0_23:
        add     r1, r12, #1
.LBB0_24:
        ldrb    r3, [lr], #1
        sub     r1, r1, #1
        strb    r3, [r5], #1
        cmp     r1, #1
        bgt     .LBB0_24
.LBB0_25:
        mov     r1, lr
        mov     r3, #0
        cmp     r3, r2, lsr #2
        bne     .LBB0_10
        b       .LBB0_19
FUNCTION_END(memcpy)
FUNCTION_END(__aeabi_memcpy)
