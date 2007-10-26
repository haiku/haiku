/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#warning M68K: optimize memcpy
#if 0
#define FUNCTION(x) .global x; .type x,@function; x

/* that should be enough for now */

.align 4
FUNCTION(memcpy):
/*	void *memcpy(void *dest, const void *src, size_t count) */
	move.l	(4,%a7),%a1
	move.l	(8,%a7),%a0
	move.l	(12,%a7),%d0
	addi.l	#-1,%d0
	blt	_memcpy_out
_memcpy_loop:
	move.b	(%a0)+,(%a1)+
	dbf	%d0,_memcpy_loop
_memcpy_out:
	move.l	(4,%a7),%a0
	move.l	%a0,%d0
	rts
#endif
