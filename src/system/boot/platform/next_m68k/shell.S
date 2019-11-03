/*
 * Copyright 2020, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */

/**	This file contains the entry point for the NeXT loader.
 */

/*
 * references :
 * NetBSD sys/arch/next68k/stand/boot/srt0.s
 */

 #include <asm_defs.h>


// 1 enable verbose output
//#define DEBUG 1

#define GLOBAL(x) .globl x ; x
//#define FUNCTION(x) .global x; .type x,@function; x


.text


SYMBOL(_start):
	// find the monitor vector and push it
	movec	%vbr,%a3
	movel	%a3@(4),-(%sp)
	// push the arg (bootarg)
	movel	%sp@(8),-(%sp)
	jsr	start_next
	addql	#8,%sp
	rts
SYMBOL_END(_start)


