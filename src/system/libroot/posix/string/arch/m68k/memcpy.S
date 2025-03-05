/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <asm_defs.h>

#warning M68K: optimize memcpy
#if 1

/* that should be enough for now */

.align 4
FUNCTION(memcpy):
/*	void *memcpy(void *dest, const void *src, size_t count) */
	move.l	(4,%a7),%a1
	move.l	(8,%a7),%a0
	move.l	(12,%a7),%d0
_memcpy_loop:
	addi.l	#-1,%d0
	blt	_memcpy_out
	move.b	(%a0)+,(%a1)+
	bra	_memcpy_loop
_memcpy_out:
	move.l	(4,%a7),%a0
	move.l	%a0,%d0
	rts
FUNCTION_END(memcpy)
#endif
