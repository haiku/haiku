/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/**	This file contains the final part of the ".init" and ".fini" sections in
 *	the ELF executable. It is tightly connected to crti.S.
 *	Have a look at crti.S to find a description of what happens here.
 */

.section .init
	// the image ID and program args are still on the stack
	jsr	__haiku_init_after
	rts

.section .fini
	// the image ID and program args are still on the stack
	jsr	__haiku_term_after
	rts
