/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
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

.section .init
FUNCTION(_init):
	pushl	%ebp
	movl	%esp, %ebp
	sub     $4,%esp		// Keep stack aligned
	pushl	8(%ebp)		// put image ID on the stack again
	// crtbegin.o stuff comes here

.section .fini
FUNCTION(_fini):
	pushl	%ebp
	movl	%esp, %ebp
	sub     $4,%esp		// Keep stack aligned
	pushl	8(%ebp)
	// crtend.o stuff comes here
