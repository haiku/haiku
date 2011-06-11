/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
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

#include "multiboot.h"

#define GLOBAL(x) .globl x ; x

#define OUR_MB_FLAGS (MULTIBOOT_PAGE_ALIGN \
	| MULTIBOOT_MEMORY_INFO \
	/*| MULTIBOOT_VIDEO_MODE*/ \
	| MULTIBOOT_AOUT_KLUDGE)

// load address
#define LOAD_SEGMENT 0x1000
#define LOAD_ADDRESS 0x10000

// MultiBoot load address
#define MB_LOAD_ADDRESS 0x100000
//#define MB_LOAD_ADDRESS LOAD_ADDRESS
#define MB_LOAD_OFFSET (MB_LOAD_ADDRESS - LOAD_ADDRESS)

// this saves us some trouble with relocation (I didn't manage GAS to
// create 32 bit references to labels)
#define FAILURE_STRING 0x1d0
#define DOT_STRING 0x1fc

#define DRIVE_RETRIES 3
	// when the drive reading fails for some reason, it will
	// retry this many times until it will report a failure

.text
.code16

/** This is the entry point when we were written directly to a floppy disk */

	jmp		floppy_start

sNumSectors:
	// this location will contain the length of the boot loader as
	// written by the "makeflop" command in 512 byte blocks
	// 0x180 is the allowed maximum, as the zipped TAR with the
	// kernel and the boot module might start at offset 192 kB
	.word BOOT_ARCHIVE_IMAGE_OFFSET*2

floppy_start:
	cli
	cld

	// set up the stack to 0x0000:0x9000
	xor		%ax, %ax
	mov		%ax, %ss
	mov		$0x9000, %sp

	push	$0x07c0
	pop		%ds
	push	$0x1000
	pop		%es

	// load the rest of the boot loader to 0x1000:0x0200
	.code32					// we need to create a 32-bit relocation entry for the linker...
	.byte	0x67
	movw	sNumSectors - 0x10000, %di
		// the loader symbols are located at offset 0x10000
	.code16
	xor		%dh, %dh		// head 0, don't change BIOS boot device
	mov		$0x2, %cx		// sector 2
	mov		$0x200, %bx		// to 0x1000:0x0200
	call	load_sectors

	// ToDo: this seems to be problematic, at least under Bochs (reboot will fail)
#if 0
	or		%dl, %dl		// if it's a floppy, turn off its motor
	jnz		start_loader
	call	disable_floppy_motor
#endif

start_loader:
	// indicate that we were booted from CD/floppy/whatever
	.code32
	.byte	0x67
	movb	$1, gBootedFromImage - 0x7c00
		// %ds is 0x7c0 right now, but the symbol were loaded
		// to offset 0x10000
	.code16

	// set our environment and jump to the standard BFS boot block entry point
	xor		%dx, %dx		// boot device ID and partition offset to 0
	xor		%eax, %eax
	ljmp	$0x1000, $0x0200


/**	Loads %di sectors from floppy disk, starting at head %dh, sector %cx.
 *	The data is loaded to %es:%bx. On exit, %es:%bx will point immediately
 *	behind the loaded data, so that you can continue to read in data.
 *	%ax, %cx, %dx, %bp, %di and %si will be clobbered.
 */

load_sectors:
	// first, get information about the drive as we intend to read whole tracks
	push	%bx
	push	%cx
	push	%dx
	push	%di
	push	%es

	movb	$8, %ah			// get drive parameters - changes a lot of registers
	int		$0x13

	pop		%es
	pop		%di
		// ToDo: store the number of heads somewhere (it's in %dh)
	pop		%dx
	and		$63, %cx		// mask out max. sector number (bit 0-5)
	mov		%cx, %si		// and remember it
	pop		%cx
	pop		%bx

load_track:
	mov		%di, %ax		// limit the sector count to track boundaries
	add		%cl, %al
	dec		%ax
	cmp		%si, %ax
	jbe		matches_track_boundary
	mov		%si, %ax
matches_track_boundary:
	inc		%ax				// take the current sector offset into account
	sub		%cl, %al

	// make sure we don't cross a 64kB address boundary or else the read will fail
	// (this small piece of knowledge took me some time to accept :))
	shl		$9, %ax
	mov		%ax, %bp
	add		%bx, %bp
	jnc		respects_boundary
	xor		%ax, %ax		// only read up to the 64kB boundary
	sub		%bx, %ax
respects_boundary:
	shr		$9, %ax
	mov		DRIVE_RETRIES, %bp

try_to_read:
	pusha
	movb	$2, %ah			// load sectors from drive
	int		$0x13
	jnc		read_succeeded

	xor		%ax, %ax
	int		$0x13			// reset drive
	popa

	dec		%bp
	jz		load_failed		// if already retried often enough, bail out
	jmp		try_to_read

read_succeeded:
	mov		$DOT_STRING, %si
	call	print_string
	popa

	xor		%ah, %ah
	add		%ax, %cx		// next sector start
	sub		%ax, %di		// update sectors left to be read

	shl		$9, %ax			// get byte offset
	add		%ax, %bx		// update target address
	jnz		check_sector_start

	mov		%es, %ax		// overflow to the next 64kB, %bx is already zero
	add		$0x1000, %ax
	mov		%ax, %es

check_sector_start:
	mov		%si, %ax		// compare the sectors, not the cylinders
	cmp		%al, %cl
	jbe		continue_reading

	sub		%si, %cx
	inc		%dh				// next head
	cmp		$1, %dh
		// ToDo: check max. number of heads!
	jbe		check_sector_start

	xor		%dh, %dh		// next cylinder
	inc		%ch
	jmp		check_sector_start

continue_reading:
	or		%di, %di
	jnz		load_track
	ret

load_failed:
	mov		$FAILURE_STRING, %si
	call	print_string

	xor		%ax, %ax
	int		$0x16			// wait for key
	int		$0x19			// and reboot

disable_floppy_motor:
	xor		%al, %al
	mov		$0x3f2, %dx
	out		%al, %dx
	ret

print_string:
	movb	$0x0e, %ah
	xor		%bx, %bx
print_char:
	lodsb
	orb		%al, %al		// are there still characters left?
	jz		no_more_chars
	int		$0x10
	jmp		print_char
no_more_chars:
	ret

floppy_end:
	.org	FAILURE_STRING
	.string " Loading failed! Press key to reboot.\r\n"
	.org	DOT_STRING
	.string	"."

	.org	0x01fe
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
	.byte	0x66
	movl	%eax, gBootPartitionOffset - 0x10000
	.code16

	xor		%ax, %ax		// set up stack at 0x0000:0x9000
	mov		%ax, %ss
	mov		$0x9000, %sp

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

	movl	$0x10000, %esp	// setup new stack
	pushl	$0				// terminate stack frame chain (next frame and
	pushl	$0				// return address)
	mov		%esp, %ebp

	call	_start

//--------------------------------------------------------------

/** MultiBoot entry point
 */

multiboot_start:
	//subl		$MULTIBOOT_MAGIC2, %eax
	//jnz		load_failed		// rts to grub ?
	movl		%ebx, gMultiBootInfo + MB_LOAD_OFFSET
	// load the GDT
	lgdt		gdt_descriptor + MB_LOAD_OFFSET

#if MB_LOAD_ADDRESS != LOAD_ADDRESS
	// QEMU does not like the real load address...
	// copy ourselves to the expected location
	cld
	mov		$(_end - LOAD_ADDRESS), %ecx
	add		$3, %ecx
	shr		$2, %ecx
	mov		$LOAD_ADDRESS, %edi
	mov		$MB_LOAD_ADDRESS, %esi
	rep movsl

	// reload the GDT just in case
	lgdt		gdt_descriptor
#endif

relocated_mb_start:
	ljmp		$0x8, $_protected_code_segment

//--------------------------------------------------------------

/** Enables the a20 gate. It will first try to enable it through
 *	the BIOS, and, if that fails, will use the old style AT mechanism
 *	using the keyboard port.
 *	ToDo: it no longer does this! Now, it just uses the "fast A20"
 *		mechanism using port 0x92. This does work on all systems
 *		I have access to.
 */

enable_a20:
	inb		$0x92, %al
	testb	$0x02, %al
	jnz		_a20_out
	orb		$0x02, %al
	andb	$0xfe, %al
	outb	%al, $0x92
_a20_out:
	ret

// ToDo: the code below didn't seem to work properly on all machines
/*	movw	$0x2402, %ax		// first, query the a20 status
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
*/

//--------------------------------------------------------------

.org 856
	// since we don't need the above space when the boot loader is
	// running, it is used as a real mode scratch buffer (as our
	// boot loader spans over the whole real mode 0x1000 segment)

.align 4
multiboot_header:
	.long	MULTIBOOT_MAGIC
	.long	OUR_MB_FLAGS
	.long	(0 - MULTIBOOT_MAGIC - OUR_MB_FLAGS)		// checksum (8 bytes)
	.long	multiboot_header + MB_LOAD_OFFSET
	.long	.text + MB_LOAD_OFFSET
	.long	.bss + (MB_LOAD_OFFSET - 24)
	.long	_end + (MB_LOAD_OFFSET - 24)
	.long	multiboot_start + MB_LOAD_OFFSET
#if (OUR_MB_FLAGS & MULTIBOOT_VIDEO_MODE)
	.long	0	// non text mode
	.long	1024
	.long	786
	.long	24
#endif

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

GLOBAL(gBootedFromImage):
	.byte	0

GLOBAL(gBootDriveID):
	.byte	0

GLOBAL(gBootPartitionOffset):
	.long	0

GLOBAL(gMultiBootInfo):
	.long	0

.org 1024
