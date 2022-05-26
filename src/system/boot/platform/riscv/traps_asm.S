/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <arch_traps.h>


.globl MVec
.type  MVec, @function
.align 4
MVec:
	PushTrapFrame
	sd fp, 2*8(sp)
	csrr t0, mepc
	sd   t0, 31*8(sp)

	mv a0, sp
	call MTrap

	ld t0, 31*8(sp)
	csrw mepc, t0
	PopTrapFrame
	mret
.size	MVec, .-MVec


.globl MVecS
.type  MVecS, @function
.align 4
MVecS:
	csrrw sp, mscratch, sp

	PushTrapFrame

	csrr t0, mscratch
	sd t0, 2*8(sp) # save supervisor SP
	csrw mscratch, fp

	csrr t0, mepc
	sd   t0, 31*8(sp)

	la   t0,    MVec
	csrw mtvec, t0

	mv a0, sp
	call MTrap

	la   t0,    MVecS
	csrw mtvec, t0

	ld t0, 31*8(sp)
	csrw mepc, t0
	PopTrapFrame
	mret
.size	MVecS, .-MVecS
