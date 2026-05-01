/*
 * Copyright 2018-2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


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
