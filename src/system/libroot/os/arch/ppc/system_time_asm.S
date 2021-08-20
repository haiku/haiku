/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* int64 __ppc_get_time_base(void)
 * r3/r4
 */
FUNCTION(__ppc_get_time_base):
		/* get TB (time base) register */
carry:	mftbu	%r3
		mftb	%r4
		mftbu	%r5			// read the upper half again
		cmpw	%r3, %r5	// and check if the values have changed
		bne		carry		// try again, if they had
		blr

