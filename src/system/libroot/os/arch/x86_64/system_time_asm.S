/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <asm_defs.h>


/* saves the conversion factor needed for system_time */
.lcomm cv_factor				8
.lcomm cv_factor_nsecs			8


.text


FUNCTION(__x86_setup_system_time):
	movq	%rdi, cv_factor
	movq	%rsi, cv_factor_nsecs
	ret
FUNCTION_END(__x86_setup_system_time)


/* int64 system_time(); */
FUNCTION(system_time):
	// (rdtsc * cv_factor) >> 32.
	// Factor is pre-shifted left by 32 bits.

	movq	cv_factor, %rcx

	// Load 64-bit TSC into %eax (low), %edx (high).
	rdtsc

	// Convert into a single 64-bit value.
	shl		$32, %rdx
	orq		%rdx, %rax

	// Multiply by conversion factor, result in %rax (low), %rdx (high).
	mulq	%rcx

	// Due to pre-shifting of the factor the whole result in high.
	movq	%rdx, %rax
	ret
FUNCTION_END(system_time)


/* int64 system_time_nsecs(); */
FUNCTION(system_time_nsecs):
	// Same algorithm as system_time(), but with a different factor.
	// (rdtsc * cv_factor_nsecs) >> 32.
	// Factor has not been pre-shifted here, otherwise we may lose the upper
	// 32 bits.

	movq	cv_factor_nsecs, %rcx

	// Load 64-bit TSC into %eax (low), %edx (high).
	rdtsc

	// Convert into a single 64-bit value.
	shl		$32, %rdx
	orq		%rdx, %rax

	// Multiply by conversion factor, result in %rax (low), %rdx (high).
	mulq	%rcx

	// Shift the result right by 32 bits.
	shr		$32, %rax
	shl		$32, %rdx
	orq		%rdx, %rax
	ret
FUNCTION_END(system_time_nsecs)
