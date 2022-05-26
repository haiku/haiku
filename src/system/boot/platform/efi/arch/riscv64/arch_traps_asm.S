/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <arch_traps.h>


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
