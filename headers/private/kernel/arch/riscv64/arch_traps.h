/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _ARCH_TRAPS_H_
#define _ARCH_TRAPS_H_


# NOTE: this macro don't save SP, it should be saved manually
.macro PushTrapFrame extSize
	addi sp, sp, -(\extSize + 256)

	sd ra,  \extSize +  0*8(sp)
	sd t6,  \extSize +  1*8(sp)
#	sd sp,  \extSize +  2*8(sp) # sp
	sd gp,  \extSize +  3*8(sp)
	sd tp,  \extSize +  4*8(sp)
	sd t0,  \extSize +  5*8(sp)
	sd t1,  \extSize +  6*8(sp)
	sd t2,  \extSize +  7*8(sp)
	sd t5,  \extSize +  8*8(sp)
	sd s1,  \extSize +  9*8(sp)
	sd a0,  \extSize + 10*8(sp)
	sd a1,  \extSize + 11*8(sp)
	sd a2,  \extSize + 12*8(sp)
	sd a3,  \extSize + 13*8(sp)
	sd a4,  \extSize + 14*8(sp)
	sd a5,  \extSize + 15*8(sp)
	sd a6,  \extSize + 16*8(sp)
	sd a7,  \extSize + 17*8(sp)
	sd s2,  \extSize + 18*8(sp)
	sd s3,  \extSize + 19*8(sp)
	sd s4,  \extSize + 20*8(sp)
	sd s5,  \extSize + 21*8(sp)
	sd s6,  \extSize + 22*8(sp)
	sd s7,  \extSize + 23*8(sp)
	sd s8,  \extSize + 24*8(sp)
	sd s9,  \extSize + 25*8(sp)
	sd s10, \extSize + 26*8(sp)
	sd s11, \extSize + 27*8(sp)
	sd t3,  \extSize + 28*8(sp)
	sd t4,  \extSize + 29*8(sp)
	sd fp,  \extSize + 30*8(sp)

	addi fp, sp, \extSize + 256
.endm


.macro PopTrapFrame extSize
	ld ra,  \extSize +  0*8(sp)
	ld t6,  \extSize +  1*8(sp)
#	ld sp,  \extSize +  2*8(sp) restore later
	ld gp,  \extSize +  3*8(sp)
#	ld tp,  \extSize +  4*8(sp)
	ld t0,  \extSize +  5*8(sp)
	ld t1,  \extSize +  6*8(sp)
	ld t2,  \extSize +  7*8(sp)
	ld t5,  \extSize +  8*8(sp)
	ld s1,  \extSize +  9*8(sp)
	ld a0,  \extSize + 10*8(sp)
	ld a1,  \extSize + 11*8(sp)
	ld a2,  \extSize + 12*8(sp)
	ld a3,  \extSize + 13*8(sp)
	ld a4,  \extSize + 14*8(sp)
	ld a5,  \extSize + 15*8(sp)
	ld a6,  \extSize + 16*8(sp)
	ld a7,  \extSize + 17*8(sp)
	ld s2,  \extSize + 18*8(sp)
	ld s3,  \extSize + 19*8(sp)
	ld s4,  \extSize + 20*8(sp)
	ld s5,  \extSize + 21*8(sp)
	ld s6,  \extSize + 22*8(sp)
	ld s7,  \extSize + 23*8(sp)
	ld s8,  \extSize + 24*8(sp)
	ld s9,  \extSize + 25*8(sp)
	ld s10, \extSize + 26*8(sp)
	ld s11, \extSize + 27*8(sp)
	ld t3,  \extSize + 28*8(sp)
	ld t4,  \extSize + 29*8(sp)
	ld fp,  \extSize + 30*8(sp)

	ld sp,  \extSize +  2*8(sp)
.endm


#endif	// _ARCH_TRAPS_H_
