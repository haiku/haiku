/*
   Copyright 2019, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under the terms of the MIT License.
 */


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
.global _init
.type _init, #function
	// crtbegin.o stuff comes here

.section .fini
.global _fini
.type _fini, #function
	// crtend.o stuff comes here
