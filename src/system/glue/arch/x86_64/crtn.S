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
	pop		%rdi
	call	__haiku_init_after
	movq	%rbp, %rsp
	pop		%rbp
	ret

.section .fini
	pop		%rdi
	call	__haiku_term_after
	movq	%rbp, %rsp
	pop		%rbp
	ret
