/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "arch_traps.h"


.globl SVec
.type  SVec, @function
.align 4
SVec:
	PushTrapFrame
	sd fp, 2*8(sp)
	csrr t0, sepc
	sd   t0, 31*8(sp)

	mv a0, sp
	call STrap

	ld t0, 31*8(sp)
	csrw sepc, t0
	PopTrapFrame
	sret
.size	SVec, .-SVec


.globl SVecU
.type  SVecU, @function
.align 4
SVecU:
	csrrw t0, sscratch, t0   # t0: &arch_thread
	ld    tp,        0*8(t0) # tp = arch_thread.thread
	ld    t0, (1 + 13)*8(t0) # t0 = arch_thread.context.sp
	sd    sp,  2*8 - 256(t0) # save user SP
	mv    sp, t0             # switch to kernel stack
	csrr  t0, sscratch

	PushTrapFrame

	csrr t0, sepc
	sd   t0, 31*8(sp)

	la   t0,    SVec
	csrw stvec, t0

	mv a0, sp
	call STrap

.globl SVecURet
.type  SVecURet, @function
SVecURet:
	call RestoreUserRegs

	csrr  t0, sscratch
	sd    fp, (1 + 13)*8(t0) # arch_thread.context.sp = fp

	la   t0,    SVecU
	csrw stvec, t0

	ld t0, 31*8(sp)
	csrw sepc, t0
	PopTrapFrame
	sret
.size	SVecU, .-SVecU
