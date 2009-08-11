/*
** Copyright 2003, Axel D�fler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <asm_defs.h>

.text

/* uint16 __swap_int16(uint16 value)
 */
FUNCTION(__swap_int16):
#if __ARM_ARCH__ >= 6
		rev16 r0,r0;
#else
#warning IMPLEMENT_ME
#endif
FUNCTION_END(__swap_int16)

/* uint32 __swap_int32(uint32 value)
 */
FUNCTION(__swap_int32):
#if __ARM_ARCH__ >= 6
		rev r0,r0;
#else
#warning IMPLEMENT_ME
#endif
FUNCTION_END(__swap_int32)

/* uint64 __swap_int64(uint64 value)
 */
FUNCTION(__swap_int64):
#if __ARM_ARCH__ >= 6
		rev r0,r0;
		rev r1,r1;
		mov r12,r0;
		mov r0,r1;
		mov r1,r12;
#else
#warning IMPLEMENT_ME
#endif
FUNCTION_END(__swap_int64)

/* TODO: The following functions can surely be optimized. A simple optimization
 * would be to define macros with the contents of the __swap_int{32,64}
 * functions and use those instead of calling the functions.
 */

/* float __swap_float(float value)
 */
FUNCTION(__swap_float):
		b		__swap_int32
		//rts
FUNCTION_END(__swap_float)


/* double __swap_double(double value)
 */
FUNCTION(__swap_double):
		b		__swap_int32
		//rts
#warning M68K: XXX:check sizeof(double)
FUNCTION_END(__swap_double)

