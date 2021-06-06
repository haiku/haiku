/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _ARCH_TRAPS_H_
#define _ARCH_TRAPS_H_


# NOTE: this macro don't save SP, it should be saved manually
.macro PushTrapFrame
	addi sp, sp, -256

	sd ra,   0*8(sp)
	sd t6,   1*8(sp)
#	sd sp,   2*8(sp) # sp
	sd gp,   3*8(sp)
	sd tp,   4*8(sp)
	sd t0,   5*8(sp)
	sd t1,   6*8(sp)
	sd t2,   7*8(sp)
	sd t5,   8*8(sp)
	sd s1,   9*8(sp)
	sd a0,  10*8(sp)
	sd a1,  11*8(sp)
	sd a2,  12*8(sp)
	sd a3,  13*8(sp)
	sd a4,  14*8(sp)
	sd a5,  15*8(sp)
	sd a6,  16*8(sp)
	sd a7,  17*8(sp)
	sd s2,  18*8(sp)
	sd s3,  19*8(sp)
	sd s4,  20*8(sp)
	sd s5,  21*8(sp)
	sd s6,  22*8(sp)
	sd s7,  23*8(sp)
	sd s8,  24*8(sp)
	sd s9,  25*8(sp)
	sd s10, 26*8(sp)
	sd s11, 27*8(sp)
	sd t3,  28*8(sp)
	sd t4,  29*8(sp)
	sd fp,  30*8(sp)

	addi fp, sp, 256
.endm


.macro PopTrapFrame
	ld ra,   0*8(sp)
	ld t6,   1*8(sp)
#	ld sp,   2*8(sp) restore later
	ld gp,   3*8(sp)
#	ld tp,   4*8(sp)
	ld t0,   5*8(sp)
	ld t1,   6*8(sp)
	ld t2,   7*8(sp)
	ld t5,   8*8(sp)
	ld s1,   9*8(sp)
	ld a0,  10*8(sp)
	ld a1,  11*8(sp)
	ld a2,  12*8(sp)
	ld a3,  13*8(sp)
	ld a4,  14*8(sp)
	ld a5,  15*8(sp)
	ld a6,  16*8(sp)
	ld a7,  17*8(sp)
	ld s2,  18*8(sp)
	ld s3,  19*8(sp)
	ld s4,  20*8(sp)
	ld s5,  21*8(sp)
	ld s6,  22*8(sp)
	ld s7,  23*8(sp)
	ld s8,  24*8(sp)
	ld s9,  25*8(sp)
	ld s10, 26*8(sp)
	ld s11, 27*8(sp)
	ld t3,  28*8(sp)
	ld t4,  29*8(sp)
	ld fp,  30*8(sp)

	ld sp,   2*8(sp)
.endm


#endif	// _ARCH_TRAPS_H_
