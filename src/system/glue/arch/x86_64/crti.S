/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


/**	This file contains the first part of the ".init" and ".fini" sections in
 *	the ELF executable.
 *	The functions defined here will be called during initialization/termination
 *	of the loaded executable/library. The ".init" and ".fini" sections are
 *	stacked together like this:
 *
 *	crti.S		entry point
 *	crtbegin.S	GCC specific: constructors/destructors are called, ...
 *	crtend.S
 *	crtn.S		call to _init_after/_term_after
 *				exit
 */


.section .init
FUNCTION(_init):
	push	%rbp
	movq	%rsp, %rbp

	// Preserve image ID.
	push	%rdi
	sub		$0x8, %rsp

	// crtbegin.o stuff comes here

.section .fini
FUNCTION(_fini):
	push	%rbp
	movq	%rsp, %rbp

	push	%rdi
	sub		$0x8, %rsp

	// crtend.o stuff comes here
