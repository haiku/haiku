/*
 * Copyright 2018-2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include "asm_offsets.h"


.section .rodata

FUNCTION(_stac):
	stac
FUNCTION_END(_stac)

FUNCTION(_clac):
	clac
FUNCTION_END(_clac)

FUNCTION(_xsave):
	xsave64		(%rdi)
FUNCTION_END(_xsave)

FUNCTION(_xsavec):
	xsavec64	(%rdi)
FUNCTION_END(_xsavec)

FUNCTION(_xrstor):
	xrstor64	(%rdi)
FUNCTION_END(_xrstor)

FUNCTION(_vzeroall):
	vzeroall
FUNCTION_END(_vzeroall)

FUNCTION(_xrstor_initial):
	movabsq		$gInitialState + ARCH_THREAD_user_fpu_state, %rdi
	mov 		$0xFFFFFFFF, %eax
	mov 		$0xFFFFFFFF, %edx
	xrstor64	(%rdi)
FUNCTION_END(_xrstor_initial)
