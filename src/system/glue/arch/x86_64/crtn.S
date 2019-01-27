/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


/**	This file contains the final part of the ".init" and ".fini" sections in
 *	the ELF executable. It is tightly connected to crti.S.
 *	Have a look at crti.S to find a description of what happens here.
 */


.section .init
	// The image ID is preserved on the stack.
	add		$0x8, %rsp
	pop		%rdi

	movq	%rbp, %rsp
	pop		%rbp
	ret

.section .fini
	add		$0x8, %rsp
	pop		%rdi

	movq	%rbp, %rsp
	pop		%rbp
	ret
