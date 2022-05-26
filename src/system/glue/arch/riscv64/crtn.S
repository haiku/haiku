/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/**	This file contains the final part of the ".init" and ".fini" sections in
 *	the ELF executable. It is tightly connected to crti.S.
 *	Have a look at crti.S to find a description of what happens here.
 */

.section .init
	ld   ra, 8(sp)
	addi sp, sp, 16
	ret


.section .fini
	ld   ra, 8(sp)
	addi sp, sp, 16
	ret
