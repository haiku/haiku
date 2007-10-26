/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* int64 __m68k_get_time_base(void)
 */
FUNCTION(__m68k_get_time_base):
#warning M68K: implement __m68k_get_time_base()
		rts

