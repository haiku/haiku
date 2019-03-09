/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

.text

// These function use the unused parameters slots on the stack (reserved for
// arguments that are in fact passed in registers) for temporary storage.
// %o0 is corrupt, but it is a scratch register so this should be fine.

/* float __swap_float(float value)
 */
FUNCTION(__swap_float):
	add %sp, 2175, %o0
	st %d0, [%o0]
	lda [%o0] 0x88, %d0
	return
FUNCTION_END(__swap_float)


/* double __swap_double(double value)
 */
FUNCTION(__swap_double):
	add %sp, 2175, %o0
	std %d0, [%o0]
	ldda [%o0] 0x88, %d0
	return
FUNCTION_END(__swap_double)

