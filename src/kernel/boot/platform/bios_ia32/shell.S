/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


/**	This file contains the boot floppy and BFS boot block entry points for
 *	the stage 2 boot loader.
 *	The floppy entry point is at offset 0. It's loaded at 0x07c0:0x000. It
 *	will load the rest of the loader to 0x1000:0x0200 and execute it.
 *	The BFS boot block will load the whole stage 2 loader to 0x1000:0x0000
 *	and will then jump to 0x1000:0x0200 as its entry point.
 *	This code will then switch to protected mode and will directly call
 *	the entry function of the embedded ELF part of the loader.
 */

#define GLOBAL(x) .globl x ; x

.text
.code16

/** This is the entry point when we were written directly to a floppy disk */

floppy_start:
	ljmp	$0x07c0, $0x0005	// make sure we're on the right segment

	jmp		floppy_start
		// ToDo: implement the rest of the floppy loader!


floppy_end:
	.org	0x01fe-floppy_end
	.word	0xaa55
		// this bumps the "start" label to offset 0x0200 as
		// expected by the BFS boot loader, and also marks
		// this block as valid boot block for the BIOS


//--------------------------------------------------------------

/**	This is the entry point of the stage2 bootloader when it has
 *	been loaded from the stage1 loader from a BFS disk.
 */

bfs_start:
	cld						// set the data, and extra segment to our code start
	pushw	$0x1000
	pop		%ds
	push	%ds
	pop		%es

	.code32					// save knowledge from the BFS boot block for later use
	.byte	0x67
	movb	%dl, gBootDriveID - 0x10000
	.byte	0x67
	movl	%eax, gBootPartitionOffset - 0x10000
	.code16

	cli						// no interrupts please
	call	enable_a20		// enable a20 gate

	.code32					// This forces a 32 bit relocation entry
	.byte	0x66			// that allows linking with others
	.byte	0x67
	lgdt	gdt_descriptor - 0x10000
		// load global descriptor table; we're still in real mode segment
		// 0x1000 so we have to manually correct the address

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

/** Enables the a20 gate. It will first try to enable it through
 *	the BIOS, and, if that fails, will use the old style AT mechanism
 *	using the keyboard port.
 */

enable_a20:
	movw	$0x2402, %ax		// first, query the a20 status
	int		$0x15
	jc		_a20_old_method		// if that fails, use the old AT method
	test	$0x1, %al
	jnz		_a20_done			// Is a20 gate already enabled?
	movw	$0x2401, %ax
	int		$0x15
	jnc		_a20_done
_a20_old_method:
	call	_a20_loop1			// empty the keyboard buffer
	jnz		_a20_done
	movb	$0xd1, %al
	outb	%al, $0x64
	call	_a20_loop1			// empty the keyboard buffer
	jnz		_a20_done
	movb	$0xdf, %al
	outb	%al, $0x60
_a20_loop1:
	movl	$0x20000, %ecx
_a20_loop2:
	inb		$0x64, %al
	test	$0x2, %al
	loopne	_a20_loop2
_a20_done:
	ret

//--------------------------------------------------------------

.org 896
	// since we don't need the above space when the boot loader is
	// running, it is used as a real mode scratch buffer (as our
	// boot loader spans over the whole real mode 0x1000 segment)

/* global data table */

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

GLOBAL(gBootDriveID):
	.byte	0

GLOBAL(gBootPartitionOffset):
	.long	0

.org 1024
