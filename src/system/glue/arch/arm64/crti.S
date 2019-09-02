/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
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

#define FUNCTION(x) .globl x; .type _init, %function; x

.section .init, "ax", @progbits
.p2align 16
FUNCTION(_init):
	sub		sp, sp, #16
	str		lr, [sp]

.section .fini, "ax", @progbits
.p2align 16
FUNCTION(_fini):
	sub		sp, sp, #16
	str		lr, [sp]
