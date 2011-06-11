/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include "interrupts.h"


// #pragma mark - interrupt functions


#define INTERRUPT_FUNCTION(vector)			\
.align 8;									\
FUNCTION(interrupt_function##vector):		\
	pushl	$0;								\
	pusha;									\
	pushl	$vector;						\
	call	handle_exception;

#undef INTERRUPT_FUNCTION_ERROR
#define INTERRUPT_FUNCTION_ERROR(vector)	\
.align 8;									\
FUNCTION(interrupt_function##vector):		\
	pusha;									\
	pushl	$vector;						\
	call	handle_exception;

INTERRUPT_FUNCTIONS()


// #pragma mark - interrupt functions during exception


#undef INTERRUPT_FUNCTION
#define INTERRUPT_FUNCTION(vector)					\
.align 8;											\
FUNCTION(exception_interrupt_function##vector):		\
	call	handle_exception_exception;

#undef INTERRUPT_FUNCTION_ERROR
#define INTERRUPT_FUNCTION_ERROR(vector)	INTERRUPT_FUNCTION(vector)

INTERRUPT_FUNCTIONS()
