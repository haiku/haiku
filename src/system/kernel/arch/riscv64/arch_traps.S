/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>
#include "arch_traps.h"
#include "asm_offsets.h"


.align 4
FUNCTION(SVec):
	PushTrapFrame IFRAME_ra
	sd fp, IFRAME_sp(sp)
	csrr t0, sepc
	sd   t0, IFRAME_epc(sp)

	csrr t0, sstatus
	sd   t0, IFRAME_status(sp)
	csrr t0, scause
	sd   t0, IFRAME_cause(sp)
	csrr t0, stval
	sd   t0, IFRAME_tval(sp)

	mv a0, sp
	call STrap

FUNCTION(SVecRet):
	ld   t0, IFRAME_status(sp)
	csrw sstatus, t0

	ld t0, IFRAME_epc(sp)
	csrw sepc, t0
	PopTrapFrame IFRAME_ra
	sret
FUNCTION_END(SVec)


.align 4
FUNCTION(SVecU):
	# switch to kernel stack, SSCRATCH will hold user SP
	csrrw sp, sscratch, sp

	PushTrapFrame IFRAME_ra
	csrr t0, sscratch
	sd   t0, IFRAME_sp(sp)
	csrr t0, sepc
	sd   t0, IFRAME_epc(sp)

	csrr t0, sstatus
	sd   t0, IFRAME_status(sp)
	csrr t0, scause
	sd   t0, IFRAME_cause(sp)
	csrr t0, stval
	sd   t0, IFRAME_tval(sp)

	ld tp, ARCH_STACK_thread(fp)

	la   t0,    SVec
	csrw stvec, t0

	mv a0, sp
	call STrap

FUNCTION(SVecURet):
	ld   t0, IFRAME_status(sp)
	csrw sstatus, t0

	csrw sscratch, fp # save kernel SP

	la   t0,    SVecU
	csrw stvec, t0

	ld tp, IFRAME_tp(sp)
	ld t0, IFRAME_epc(sp)
	csrw sepc, t0
	PopTrapFrame IFRAME_ra
	sret
FUNCTION_END(SVecU)
