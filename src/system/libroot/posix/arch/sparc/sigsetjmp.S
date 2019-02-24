/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
FUNCTION(_setjmp):
	ret
FUNCTION_END(setjmp)
