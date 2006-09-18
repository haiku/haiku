/*
** Copyright 2006, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the MIT License.
*/

// as -o pxe_stage1.o pxe_stage1.S
// ld --oformat binary --Ttext 0x7C00 -o pxe_stage1.bin pxe_stage1.o
// cp pxe_stage1.bin /tftpboot/pxehaiku
// objdump -mi8086 -d pxe_stage1.o
// objdump -mi8086 -bbinary -D -z pxe_stage1.bin

// system state according to PXE specification:
//   CS:IP	0000:7C00
//   ES:BX 	address of the PXENV+ structure
//   SS:[SP+4] 	address of the !PXE structure.
//   SS:SP	at least 1.5KB of free stack


.equ PXENV_GET_CACHED_INFO,		0x71
.equ PXENV_TFTP_OPEN,			0x20
.equ PXENV_TFTP_CLOSE,			0x21
.equ PXENV_TFTP_READ,			0x22
.equ PXENV_TFTP_GET_FSIZE,		0x25

.equ PXENV_PACKET_TYPE_DHCP_DISCOVER,	1
.equ PXENV_PACKET_TYPE_DHCP_ACK,		2
.equ PXENV_PACKET_TYPE_CACHED_REPLY,	3

.equ BOOTP_OPCODE_REQ,	1
.equ BOOTP_OPCODE_REP,	2


.code16
.text
.globl _start

_start:
			// save es segment to dx
			movw	%es, %dx

			// setup segments
			xorw	%ax, %ax
			movw	%ax, %ds
			movw	%ax, %es
			movw	%ax, %ss
			movw	$0x7c00, %sp

			// print start banner	
			movw	$startmsg, %si
			call	puts

			// print PXE struct address
			movw	$pxenvmsg, %si
			call	puts
			movw	%dx, %ax
			call	puthex16
			movw	$colon, %si
			call	puts
			movw	%bx, %ax
			call	puthex16
			movw	$crlf, %si
			call	puts

			// switch to unreal mode
			cli	
			call	enable_a20
			call	go_unreal
			sti
			movw	$unrealmsg, %si
			call	puts

			// store PXE struct address as linear 32 bit pointer in edx
			andl	$0xffff, %edx
			shll	$4, %edx
			andl	$0xffff, %ebx
			addl	%ebx, %edx

			// API version
			movw	$apivermsg, %si
			call	puts
			movw	0x6(%edx), %ax
			call	puthex16
			movw	$crlf, %si
			call	puts

			// store API entry as seg:ofs in pxenv_api
			movl	0xa(%edx), %eax
			movl	%eax, pxenv_api

			// print API entry address
			movw	$apimsg, %si
			call	puts
			movl	pxenv_api, %eax
			shrl	$16, %eax
			call	puthex16
			movw	$colon, %si
			call	puts
			movl	pxenv_api, %eax
			call	puthex16
			movw	$crlf, %si
			call	puts

			movw	$PXENV_PACKET_TYPE_CACHED_REPLY, %ax
//			movw	$PXENV_PACKET_TYPE_DHCP_ACK, %ax
			movw	%ax, s_PXENV_GET_CACHED_INFO_PacketType
			movw	$300, %ax
			movw	%ax, s_PXENV_GET_CACHED_INFO_BufferSize
			movl	$cached_info, %eax
			movl 	%eax, s_PXENV_GET_CACHED_INFO_Buffer

			movw	$PXENV_GET_CACHED_INFO, %bx
			movw	$s_PXENV_GET_CACHED_INFO, %di
			lcall *	pxenv_api
			test 	%ax, %ax

			andl	$0xffff, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			movw	s_PXENV_GET_CACHED_INFO_Status, %ax
			andl	$0xffff, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			movl	cached_info_cip, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			movl	cached_info_yip, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			movl	cached_info_sip, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			movl	cached_info_gip, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts


loop:
			jmp		loop


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

puthex32:
			pushl	%eax
			shrl	$16, %eax
			call	puthex16
			popl	%eax
			call	puthex16
			ret

puthex16:	pushal
			movw	%ax, %cx
			rolw	$4, %cx
			call	puthexc
			rolw	$4, %cx
			call	puthexc
			rolw	$4, %cx
			call	puthexc
			rolw	$4, %cx
			call	puthexc
			popal
			ret
puthexc:	movb	%cl, %al
			andb	$0xf, %al
			cmpb	$0xa, %al
			jb		_puthexc2
			addb	$0x27, %al
_puthexc2:	addb	$0x30, %al
			movb	$0xe, %ah
			movw	$0x7, %bx
			int		$0x10
			ret


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
			lgdt	gdt_descriptor
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


startmsg:	.asciz	"\r\nHaiku PXE bootloader version 0.1\r\n\r\n"
unrealmsg:	.asciz	"Switch to unreal mode done\r\n"

pxenvmsg:	.asciz	"PXENV+ data structure at "
apimsg:		.asciz	"PXENV+ API entry point at "
apivermsg:	.asciz	"API version is "
colon:		.asciz	":"
dot:		.asciz	"."
crlf:		.asciz	"\r\n"

.balign 8
gdt:
			.long	0
			.long	0
			.long	0x0000ffff
			.long	0x00cf9200
gdt_descriptor:
			.word	0x10
			.long	gdt

.balign 8

pxenv_api:	.long	0



s_PXENV_GET_CACHED_INFO:
s_PXENV_GET_CACHED_INFO_Status:			.word 0
s_PXENV_GET_CACHED_INFO_PacketType:		.word 0
s_PXENV_GET_CACHED_INFO_BufferSize:		.word 0
s_PXENV_GET_CACHED_INFO_Buffer:			.long 0
s_PXENV_GET_CACHED_INFO_BufferLimit:	.word 0

cached_info:
cached_info_opcode:						.byte 0
cached_info_hardware:					.byte 0
cached_info_hardlen:					.byte 0
cached_info_gatehops:					.byte 0
cached_info_ident:						.long 0
cached_info_seconds:					.word 0
cached_info_flags:						.word 0
cached_info_cip:						.long 0
cached_info_yip:						.long 0
cached_info_sip:						.long 0
cached_info_gip:						.long 0
cached_info_mac_addr:					.fill 16
cached_info_server_name:				.fill 64
cached_info_bootfile:					.fill 128
cached_info_vendorspec:					.fill 64


end:
