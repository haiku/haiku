/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jonas Sundström, jonas@kirilla.se
 */

/**	This file contains the first part of the ".init" and ".fini" sections in
 *	the ELF executable.
 *	The functions defined here will be called during initialization/termination
 *	of the loaded executable/library. The ".init" and ".fini" sections are
 *	stacked together like this:
 *
 *	crti.S		entry point
 *				call to _init_before/_term_before
 *	crtbegin.S	GCC specific: constructors/destructors are called, ...
 *	crtend.S
 *	crtn.S		call to _init_after/_term_after
 *				exit
 */

#define FUNCTION(x) .global x; .type x,@function; x

#warning MIPSEL: fixme

.section .init
FUNCTION(_init):
	.set	noreorder
	jal	__haiku_init_before
	nop
	.set	reorder
	// crtbegin.o stuff comes here

.section .fini
FUNCTION(_fini):
	.set	noreorder
	jal	__haiku_term_before
	nop
	.set	reorder
	// crtbegin.o stuff comes here
