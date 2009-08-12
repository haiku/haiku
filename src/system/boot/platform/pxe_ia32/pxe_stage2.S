/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2006, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


// system state according to PXE specification:
//   CS:IP	0000:7C00
//   ES:BX 	address of the PXENV+ structure
//   SS:[SP+4] 	address of the !PXE structure.
//   SS:SP	at least 1.5KB of free stack

// memory map:
// 0x00000 - 0x07bff		real mode IDT, stack
// 0x07C00 - 0x08FFF		original boot loader location
// 0x10000 - 0x8AFFF		relocated boot loader
// 0x8B000 - 0x8CFFF		used by stage2 trampoline code
// 0x8D000 - 0x9F7FF		PXE and UNDI code and data segments
// 0x9F800 - 0xA0000		extended BIOS data area

#define GLOBAL(x) .globl x ; x

.equ LOAD_ADDRESS,							0x10000
.equ INITIAL_LOAD_ADDRESS,					0x07C00
.equ INITIAL_LOAD_OFFSET,					INITIAL_LOAD_ADDRESS - LOAD_ADDRESS

.text
.code16

pxe_start:
			// setup segments
			xorw	%ax, %ax
			movw	%ax, %ds
			movw	%ax, %es
			movw	%ax, %ss
			movw	$0x7c00, %sp

			cld

			// print start banner
			mov		$kStartMessage + INITIAL_LOAD_OFFSET, %esi
			call	puts

			// switch to unreal mode
			cli
			call	enable_a20
			call	go_unreal
			sti
			movl	$kUnrealMessage + INITIAL_LOAD_OFFSET, %esi
			call	puts

			// relocate boot loader code to expected start address
			movl	$_end, %ecx			// desired end address
			addl	$3, %ecx			// long word align
			andb	$0xfc, %cl			// (should be page aligned anyway, though)
			movl	%ecx, %edi
			subl	$LOAD_ADDRESS, %ecx	// number of bytes to copy
			movl	$INITIAL_LOAD_ADDRESS, %esi
			addl	%ecx, %esi			// current end address
			shrl	$2, %ecx			// number of long words
_copy:		subl	$4, %esi
			subl	$4, %edi
			movl	(%esi), %eax
			movl	%eax, (%edi)
			decl	%ecx
			jnz		_copy

			// jump into the relocated boot loader
.code32
			.byte	0x66
			ljmp	$0x1000, $relocated_start - LOAD_ADDRESS
.code16

relocated_start:
			cli

			// switch to PM
			.code32
			.byte	0x66
			.byte	0x67
			lgdt	pm_gdt_descriptor
			.code16

			movl	%cr0, %eax
			orb		$0x1, %al
			movl	%eax, %cr0

.code32
			.byte	0x66
			ljmp	$0x8, $pm_start
pm_start:
			mov		$0x10, %ax
			mov		%ax, %ds
			mov		%ax, %es
			mov		%ax, %fs
			mov		%ax, %gs
			mov		%ax, %ss

			call	_start


.code16
stop:		hlt
			jmp		stop



puts:		pushal
_puts2:		lodsb
			testb	%al, %al
			jnz		putc
			popal
			ret
putc:		movw	$0x7, %bx
			movb	$0xe, %ah
			int		$0x10
			jmp		_puts2


enable_a20:	inb		$0x92, %al
			testb	$0x02, %al
			jnz		_a20_out
			orb		$0x02, %al
			andb	$0xfe, %al
			outb	%al, $0x92
_a20_out:	ret


go_unreal:	pushw	%ds
			pushw	%es
			pushw	%bx
			.code32
			.byte	0x66
			.byte	0x67
			lgdt	unreal_gdt_descriptor + INITIAL_LOAD_OFFSET
			.code16
			movl	%cr0, %eax
			orb		$1, %al
			movl	%eax, %cr0
			movw	$8, %bx
			movw	%bx, %ds
			movw	%bx, %es
			decb	%al
			movl	%eax, %cr0
			popw	%bx
			popw	%es
			popw	%ds
			ret


kStartMessage:			.asciz	"\r\nHaiku PXE bootloader version 1.0\r\n\r\n"
kUnrealMessage:			.asciz	"Switch to unreal mode done\r\n"


.balign 8
unreal_gdt:
			.long	0
			.long	0
			.long	0x0000ffff
			.long	0x00cf9200

unreal_gdt_descriptor:
			.word	0x10
			.long	unreal_gdt + INITIAL_LOAD_OFFSET

.balign 8
pm_gdt:
	// null descriptor
			.long	0
			.long	0

	// kernel code segment
			.long	0x0000ffff
			.long	0x00cf9e00
	// kernel data and stack segment
			.long	0x0000ffff
			.long	0x00cf9200

	// real mode 16 bit code segment
			.long	0x0000ffff
			.long	0x00009e01
	// real mode 16 bit data and stack segment
			.long	0x0000ffff
			.long	0x00009201
	// real mode 16 bit stack segment
			.long	0x0000ffff
			.long	0x00009200

pm_gdt_descriptor:
			.word	0x2f
			.long	pm_gdt



//--------------------------------------------------------------


/* global data table */
.balign 8

GLOBAL(gMultiBootInfo):
	.long	0

GLOBAL(gBootPartitionOffset):
	.long	0xffffffff

GLOBAL(gBootedFromImage):
	.byte	1

GLOBAL(gBootDriveID):
	.byte	0xff

.balign 8
