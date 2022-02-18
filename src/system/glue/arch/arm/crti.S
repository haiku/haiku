/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
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

#define FUNCTION(x) .global x; .type x,%function; x

.section .init
FUNCTION(_init):
	sub		sp, sp, #8
	str		lr, [sp]

.section .fini
FUNCTION(_fini):
	sub		sp, sp, #8
	str		lr, [sp]
