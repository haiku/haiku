/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the OpenBeOS License.
 */

#define FUNCTION(x) .global x; .type x,@function; x

#warning MIPSEL: fixme

.text

/* int64 __mipsel_get_time_base(void)
 * r3/r4
 */
FUNCTION(__mipsel_get_time_base):
		/* get TB (time base) register */
carry:		jr	$ra

