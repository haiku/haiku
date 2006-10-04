/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2006, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


/**	This file contains the stage 2 boot loader for the PXE (preboot execution 
 *  environment) image.
 *  It gets loaded by the PXE stage1 loader to 0xffff:0010 (0x100000), and is
 *  starts execution in 16 bit real mode. Entry point is at offset 0.
 *	This code will then switch to protected mode and will directly call
 *	the entry function of the embedded ELF part of the loader.
 */

#define GLOBAL(x) .globl x ; x

.text
.code16
.org 0

pxe_start:
	cli
	cld

	.code32					// This forces a 32 bit relocation entry
	.byte	0x66			// that allows linking with others
	.byte	0x67
	lgdt	gdt_descriptor

	.code16
	movl	%cr0, %eax		// set the PE bit of cr0 to switch to protected mode
	orb		$0x1, %al
	movl	%eax, %cr0

	.code32
	.byte	0x66
	ljmp	$0x8, $_protected_code_segment
_protected_code_segment:
	mov		$0x10, %ax		// load descriptor 2 in the data and stack segment selectors
	mov		%ax, %ds
	mov		%ax, %es
	mov		%ax, %fs
	mov		%ax, %gs
	mov		%ax, %ss

	mov		$0x10000, %ebp	// setup new stack
	mov		%ebp, %esp

	call	_start

//--------------------------------------------------------------


/* global data table */
.balign 8

gdt:
	// null descriptor
	.long	0
	.long	0

	// kernel code segment
	.long	0x0000ffff		// base: 0, limit: 4 GB
	.long	0x00cf9e00		// type: 32 bit, exec-only conforming, privilege 0
	// kernel data and stack segment
	.long	0x0000ffff		// base: 0, limit: 4 GB
	.long	0x00cf9200		// type: 32 bit, data read/write, privilege 0

	// real mode 16 bit code segment
	.long	0x0000ffff		// base: 0x10000, limit: 64 kB
	.long	0x00009e01
	// real mode 16 bit data and stack segment
	.long	0x0000ffff		// base: 0x10000, limit: 64 kB
	.long	0x00009201
	// real mode 16 bit stack segment
	.long	0x0000ffff		// base: 0, limit: 64 kB
	.long	0x00009200

gdt_descriptor:
	.word	0x2f			// 6 entries in the GDT (8 bytes each)
	.long	gdt

GLOBAL(gBootedFromImage):
	.byte	1

GLOBAL(gBootDriveID):
	.byte	0xff

GLOBAL(gBootPartitionOffset):
	.long	0xffffffff
