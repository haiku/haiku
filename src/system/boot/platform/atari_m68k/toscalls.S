/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

#define FUNCTION(x) .global x; .type x,@function; x

/* non reentrant but that's enough
 * idea taken from
 * http://www.fortunecity.com/skyscraper/apple/308/html/chap4.htm
 */

_trap_ret_addr:
	.long	0

/*	uint32 bios(uint16 nr, ...); */
FUNCTION(bios):
	move.l	(%a7)+,_trap_ret_addr
	trap	#
	move.l	_trap_ret_addr,-(%a7)
	rts

/*	uint32 xbios(uint16 nr, ...); */
FUNCTION(xbios):
	move.l	(%a7)+,_trap_ret_addr
	trap	#14
	move.l	_trap_ret_addr,-(%a7)
	rts

/*	uint32 gemdos(uint16 nr, ...); */
FUNCTION(gemdos):
	move.l	(%a7)+,_trap_ret_addr
	trap	#1
	move.l	_trap_ret_addr,-(%a7)
	rts

/*	uint32 vdi(uint16 nr, ...); */
FUNCTION(vdi):
	move.l	(%a7)+,_trap_ret_addr
	trap	#14
	move.l	_trap_ret_addr,-(%a7)
	rts

