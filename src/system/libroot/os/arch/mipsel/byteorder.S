/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the OpenBeOS License.
 */

#define FUNCTION(x) .global x; .type x,@function; x

#warning MIPSEL: fixme

.text

/* uint16 __swap_int16(uint16 value)
 *                     r3
 */
FUNCTION(__swap_int16):
		jr	$ra


/* uint32 __swap_int32(uint32 value)
 *                     r3
 */
FUNCTION(__swap_int32):
		jr	$ra


/* uint64 __swap_int64(uint64 value)
 *                     r3/r4
 */
FUNCTION(__swap_int64):
		jr	$ra


/* TODO: The following functions can surely be optimized. A simple optimization
 * would be to define macros with the contents of the __swap_int{32,64}
 * functions and use those instead of calling the functions.
 */

/* float __swap_float(float value)
 *					  f1
 */
FUNCTION(__swap_float):
		jr	$ra

/* double __swap_double(double value)
 *						f1
 */
FUNCTION(__swap_double):
		jr	$ra

