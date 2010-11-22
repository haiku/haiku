/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jonas Sundström, jonas@kirilla.se
 */

/**	This file contains the final part of the ".init" and ".fini" sections in
 *	the ELF executable. It is tightly connected to crti.S.
 *	Have a look at crti.S to find a description of what happens here.
 */

#warning MIPSEL: fixme

.section .init
	// the image ID and program args are still on the stack
	.set	noreorder
	jal	__haiku_init_after
	nop
	.set	reorder

.section .fini
	// the image ID and program args are still on the stack
	.set	noreorder
	jal	__haiku_term_after
	nop
	.set	reorder
