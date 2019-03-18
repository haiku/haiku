/*
 * Copyright 2009, Johannes Wischert. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

.text

/*
 * These aren't needed on gcc4+
 *  uint16 __swap_int16(uint16 value)
 *  uint32 __swap_int32(uint32 value)
 *  uint64 __swap_int64(uint64 value)
 */

/* TODO: The following functions can surely be optimized. A simple optimization
 * would be to define macros with the contents of the __swap_int{32,64}
 * functions and use those instead of calling the functions.
 */

/* float __swap_float(float value)
 */
FUNCTION(__swap_float):
		// Assumes single precision
		b		__swap_int32
		//rts
FUNCTION_END(__swap_float)


/* double __swap_double(double value)
 */
FUNCTION(__swap_double):
		// Assumes double is int64 on RV64
		b		__swap_int64
		//rts
FUNCTION_END(__swap_double)

