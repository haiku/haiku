/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2006, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


/**	This file contains the stage 2 boot loader for the PXE (preboot execution 
 *  environment) image.
 *  It gets loaded by the PXE stage1 loader to 0xffff:0010 (0x100000), and it
 *  starts execution in 32 bit protected mode. Entry point is at offset 0.
 *	This code will directly call the entry function of the embedded ELF part 
 *  of the loader.
 */

#define GLOBAL(x) .globl x ; x

.text
.code32
.org 0

pxe_start:

	mov		$0x10000, %ebp	// setup new stack
	mov		%ebp, %esp

// stop:	hlt
// jmp		stop

	call	_start

//--------------------------------------------------------------


/* global data table */
.balign 8

GLOBAL(gBootPartitionOffset):
	.long	0xffffffff

GLOBAL(gBootedFromImage):
	.byte	1

GLOBAL(gBootDriveID):
	.byte	0xff

.balign 8
