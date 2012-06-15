/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <asm_defs.h>

FUNCTION(__x86_setup_system_time):
	ret
FUNCTION_END(__x86_setup_system_time)

// TODO: Implement these.

/* int64 system_time(); */
FUNCTION(system_time):
	ud2a
	ret
FUNCTION_END(system_time)


/* int64 system_time_nsecs(); */
FUNCTION(system_time_nsecs):
	ud2a
	ret
FUNCTION_END(system_time_nsecs)

#if 0
/* saves the conversion factor needed for system_time */
.lcomm cv_factor				4
.lcomm cv_factor_nsecs			4
.lcomm cv_factor_nsecs_shift	1


.text


FUNCTION(__x86_setup_system_time):
	movl	4(%esp), %eax
	movl	%eax, cv_factor
	movl	8(%esp), %eax
	movl	%eax, cv_factor_nsecs
	movb	12(%esp), %al
	movb	%al, cv_factor_nsecs_shift
	ret
FUNCTION_END(__x86_setup_system_time)


/* int64 system_time(); */
FUNCTION(system_time):
	pushl	%ebx
	pushl	%ecx
	movl	cv_factor, %ebx

	/* load 64-bit factor into %eax (low), %edx (high) */
	rdtsc		/* time in %edx,%eax */

	movl	%edx, %ecx	/* save high half */
	mull	%ebx 		/* truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* save high half of low */
	mull	%ebx /*, %eax*/
	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%ebx, %edx
	popl	%ecx
	popl	%ebx
	ret
FUNCTION_END(system_time)


/* int64 system_time_nsecs(); */
FUNCTION(system_time_nsecs):
	testb	$0, cv_factor_nsecs_shift
	jne		1f

	/* same algorithm as system_time(), just with a different factor */

	pushl	%ebx
	pushl	%ecx
	movl	cv_factor_nsecs, %ebx

	/* load 64-bit factor into %eax (low), %edx (high) */
	rdtsc		/* time in %edx,%eax */

	movl	%edx, %ecx	/* save high half */
	mull	%ebx 		/* truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* save high half of low */
	mull	%ebx /*, %eax*/
	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%ebx, %edx
	popl	%ecx
	popl	%ebx
	ret

1:
	/* TSC frequency is less than 1 GHz -- we shift everything up 16 bit */

	pushl	%ebx
	pushl	%ecx
	pushl	%esi
	movl	cv_factor_nsecs, %ebx

	/* load 64-bit factor into %eax (low), %edx (high) */
	rdtsc		/* time in %edx,%eax */

	/* save high half */
	movl	%edx, %ecx

	/* multiply low half by conversion factor */
	mull	%ebx

	/* save result */
	movl	%eax, %esi	/* low half -> %esi */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* high half -> %ecx */

	/* multiply high half by conversion factor */
	mull	%ebx

	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	xorl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%ebx, %edx

	/* shift the result left 16 bit */
	shl		$16, %edx
	movl	%eax, %ebx
	shr		$16, %ebx
	orw		%bx, %dx
	shl		$16, %eax

	/* add the high 16 bit of the low half of the low product */
	shr		$16, %esi
	orw		%si, %ax

	popl	%esi
	popl	%ecx
	popl	%ebx
	ret
FUNCTION_END(system_time_nsecs)
#endif
