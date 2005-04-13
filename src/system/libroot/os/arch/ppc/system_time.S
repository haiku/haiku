/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

// ToDo: note, this function absolutely doesn't do what it should do.
//	It only reads out the time base register, but it doesn't compute
//	the real system time from its value.
//	To do so, the kernel has to provide the appropriate system dependent
//	conversion factors.

/* uint64 system_time(void)
 * r3/r4
 */
FUNCTION(system_time):
carry:	mftbu	%r3
		mftb	%r4
		mftbu	%r5			// read the upper half again
		cmpw	%r3, %r5	// and check if the values have changed
		bne		carry		// try again, if they had
		blr

