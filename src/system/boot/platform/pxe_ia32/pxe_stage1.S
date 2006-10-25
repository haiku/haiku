/*
** Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
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

.equ PXENV_PACKET_TYPE_DHCP_ACK,		2
.equ PXENV_PACKET_TYPE_CACHED_REPLY,	3


// memory map:
// 0x00000 - 0x07bff		real mode IDT, stack
// 0x07C00 - 0x08FFF		pxehaiku (stage 1, this)
// 0x09000 - 0x0FFFF		scratchbuffer
// 0x10000 - 0x8AFFF		pxe-haiku-loader (stage 2)
// 0x8B000 - 0x8CFFF		used by stage2 trampoline code
// 0x8D000 - 0x9F7FF		PXE and UNDI code and data segments
// 0x9F800 - 0xA0000		extended BIOS data area

.equ scratchbuffer_addr,		0x9000
.equ scratchbuffer_addr_seg,	(scratchbuffer_addr / 16)
.equ scratchbuffer_addr_ofs,	(scratchbuffer_addr % 16)
.equ scratchbuffer_size,		0x7000

.equ load_addr,					0x10000
.equ load_size_max,				(0x8B000 - load_addr)

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

			cld

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

			// determine client and server ips
			movw	$PXENV_GET_CACHED_INFO, %bx
			movw	$s_PXENV_GET_CACHED_INFO_3, %di
			lcall *	pxenv_api
			test 	%ax, %ax
			jnz		err_ip

			// convert pointer to linear 32 bit address
			movl	$s_PXENV_GET_CACHED_INFO_3_Buffer, %edx
			movzwl	2(%edx), %eax
			movzwl	0(%edx), %ebx
			shll	$4, %eax
			addl	%ebx, %eax
			movl	%eax, s_PXENV_GET_CACHED_INFO_3_Buffer

			// print environment data pointer and size
			movw	$envdata, %si
			call	puts
			movl	s_PXENV_GET_CACHED_INFO_3_Buffer, %eax
			call	puthex32
			movw	$length, %si
			call	puts
			movw	s_PXENV_GET_CACHED_INFO_3_BufferSize, %ax
			call	puthex16
			movw	$crlf, %si
			call	puts

			// determine client and server IPs
			movl	s_PXENV_GET_CACHED_INFO_3_Buffer, %ebx
			movl	16(%ebx), %eax
			movl	%eax, client_ip
			movl	20(%ebx), %eax
			movl	%eax, server_ip

			// print client and server IPs
			movw	$ipmsg_1, %si
			call	puts
			movl	client_ip, %eax
			call	puthex32
			movw	$ipmsg_2, %si
			call	puts
			movl	server_ip, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			// handle DHCP options.....		
			movw	$PXENV_GET_CACHED_INFO, %bx
			movw	$s_PXENV_GET_CACHED_INFO_2, %di
			lcall *	pxenv_api
			test 	%ax, %ax
			jnz		err_dhcp

			// convert pointer to linear 32 bit address
			movl	$s_PXENV_GET_CACHED_INFO_2_Buffer, %edx
			movzwl	2(%edx), %eax
			movzwl	0(%edx), %ebx
			shll	$4, %eax
			addl	%ebx, %eax
			movl	%eax, s_PXENV_GET_CACHED_INFO_2_Buffer

			// print dhcp data pointer and size
			movw	$dhcpdata, %si
			call	puts
			movl	s_PXENV_GET_CACHED_INFO_2_Buffer, %eax
			call	puthex32
			movw	$length, %si
			call	puts
			movw	s_PXENV_GET_CACHED_INFO_2_BufferSize, %ax
			call	puthex16
			movw	$crlf, %si
			call	puts

			// we could process the DHCP ack here, but we don't need options yet...


			// Print loading file
			movw	$loading, %si
			call	puts
			movw	$s_PXENV_TFTP_OPEN_FileName, %si
			call	puts
			movw	$crlf, %si
			call	puts
		
			// get file size
			movl	server_ip, %eax
			movl	%eax, s_PXENV_TFTP_GET_FSIZE_ServerIPAddress
			movw	$PXENV_TFTP_GET_FSIZE, %bx
			movw	$s_PXENV_TFTP_GET_FSIZE, %di
			lcall *	pxenv_api
			test 	%ax, %ax
			jnz		err_fsize
			movw	s_PXENV_TFTP_GET_FSIZE_Status, %ax
			test 	%ax, %ax
			jnz		err_fsize

			// print file size
			movw	$fsizeis, %si
			call	puts
			movl	s_PXENV_TFTP_GET_FSIZE_FileSize, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			// open TFTP connection
			movl	server_ip, %eax
			movl	%eax, s_PXENV_TFTP_OPEN_ServerIPAddress
			movw	$PXENV_TFTP_OPEN, %bx
			movw	$s_PXENV_TFTP_OPEN, %di
			lcall *	pxenv_api
			test 	%ax, %ax
			jnz		err_load
			movw	s_PXENV_TFTP_OPEN_Status, %ax
			test 	%ax, %ax
			jnz		err_load

			// print packet size
			movw	$psizeis, %si
			call	puts
			movw	s_PXENV_TFTP_OPEN_PacketSize, %ax
			call	puthex16
			movw	$crlf, %si
			call	puts

			// calculate max file size
			xorl	%eax, %eax
			movw	s_PXENV_TFTP_OPEN_PacketSize, %ax
			imull	$65535, %eax
			movl	%eax, max_fsize
			
			// print max TFTP file size
			movw	$maxfsize, %si
			call	puts
			movl	max_fsize, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			// print max stage 2 file size
			movw	$maxs2size, %si
			call	puts
			movl	$load_size_max, %eax
			call	puthex32
			movw	$crlf, %si
			call	puts

			// check if file is small enough (TFTP limit)
			movl	s_PXENV_TFTP_GET_FSIZE_FileSize, %eax
			movl	max_fsize, %ebx
			cmpl	%ebx, %eax
			ja		err_etoobig

			// check if file is small enough (address range limit)
			movl	s_PXENV_TFTP_GET_FSIZE_FileSize, %eax
			movl	$load_size_max, %ebx
			cmpl	%ebx, %eax
			ja		err_etoobig

			// load data
			movl	$load_addr, %edi
read_loop:
			pushl	%edi
			movw	$PXENV_TFTP_READ, %bx
			movw	$s_PXENV_TFTP_READ, %di
			lcall *	pxenv_api
			popl	%edi
			test 	%ax, %ax
			jnz		err_load
			movw	s_PXENV_TFTP_READ_Status, %ax
			test 	%ax, %ax
			jnz		err_load

			// copy to upper mem
			movw	s_PXENV_TFTP_READ_BufferSize, %cx
			movw	%cx, %dx
			shrw	$2, %cx
			andw	$3, %dx
			movl	$scratchbuffer_addr, %esi
			testw	%cx, %cx
			jz _copy_b
_copy_l:	movl	(%esi), %eax
			movl	%eax, (%edi)
			addl	$4, %esi
			addl	$4, %edi
			decw	%cx
			jnz		_copy_l
			testw	%dx, %dx
			jz _copy_done
_copy_b:	movb	(%esi), %al
			movb	%al, (%edi)
			incl	%esi
			incl	%edi
			decw	%dx
			jnz		_copy_b
_copy_done:

			// print a dot every 20 packets
			movw	pos, %ax
			incw	%ax
			movw	%ax, pos
			cmpw	$20, %ax
			jb		_no_dot
			xorw	%ax, %ax
			movw	%ax, pos
			movw	$dot, %si
			call	puts
_no_dot:

			// check if copy is done
			movw	s_PXENV_TFTP_OPEN_PacketSize, %ax
			movw	s_PXENV_TFTP_READ_BufferSize, %bx
			cmpw	%ax, %bx
			je		read_loop

			// close TFTP connection, ignore result
			movw	$PXENV_TFTP_CLOSE, %bx
			movw	$s_PXENV_TFTP_OPEN, %di
			lcall *	pxenv_api

			// execute
			movw	$executing, %si
			call	puts

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

			ljmp	$0x8, $load_addr

.code16
stop:		hlt
			jmp		stop

err_dhcp:
			movw	$dhcpfailed, %si
			call	puts
			jmp		stop

err_ip:
			movw	$ipfailed, %si
			call	puts
			jmp		stop

err_fsize:
			movw	$sizefailed, %si
			call	puts
			jmp		stop
		
err_load:
			movw	$loadfailed, %si
			call	puts
			jmp		stop

err_etoobig:
			movw	$etoobig, %si
			call	puts
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
			lgdt	unreal_gdt_descriptor
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
envdata:	.asciz	"Boot environment data at "
dhcpdata:	.asciz	"DHCP options data at "
ipfailed:	.asciz	"Error, can't determine IP address\r\n"
dhcpfailed:	.asciz	"Error, can't determine DHCP options\r\n"
apivermsg:	.asciz	"API version is "
ipmsg_1:	.asciz	"My IP address is "
ipmsg_2:	.asciz	", server IP address is "
length:		.asciz	", length "
loading:	.asciz	"Loading image file "
fsizeis:	.asciz	"Image file size is "
psizeis:	.asciz	"Packet size is "
maxfsize:	.asciz	"Maximum TFTP file size is "
maxs2size:	.asciz	"Maximum stage 2 file size is "
etoobig: 	.asciz	"haiku-pxe-loader file is too large, loading aborted\r\n"
sizefailed: .asciz	"\r\nCouldn't get haiku-pxe-loader  file size, loading failed\r\n"
loadfailed: .asciz	"\r\nLoading haiku-pxe-loader (stage 2) failed\r\n"
executing:	.asciz	"\r\nExecuting haiku-pxe-loader (stage 2)\r\n"
colon:		.asciz	":"
dot:		.asciz	"."
crlf:		.asciz	"\r\n"

.balign 8
unreal_gdt:
			.long	0
			.long	0
			.long	0x0000ffff
			.long	0x00cf9200
unreal_gdt_descriptor:
			.word	0x10
			.long	unreal_gdt

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


.balign 8
pxenv_api:	.long	0

client_ip:	.long 0
server_ip:	.long 0

max_fsize:	.long 0

pos:		.word 0

s_PXENV_GET_CACHED_INFO_2:
s_PXENV_GET_CACHED_INFO_2_Status:		.word 0
s_PXENV_GET_CACHED_INFO_2_PacketType:	.word 2
s_PXENV_GET_CACHED_INFO_2_BufferSize:	.word scratchbuffer_size
s_PXENV_GET_CACHED_INFO_2_Buffer:		.word scratchbuffer_addr_ofs
										.word scratchbuffer_addr_seg
s_PXENV_GET_CACHED_INFO_2_BufferLimit:	.word 0

s_PXENV_GET_CACHED_INFO_3:
s_PXENV_GET_CACHED_INFO_3_Status:		.word 0
s_PXENV_GET_CACHED_INFO_3_PacketType:	.word 3
s_PXENV_GET_CACHED_INFO_3_BufferSize:	.word scratchbuffer_size
s_PXENV_GET_CACHED_INFO_3_Buffer:		.word scratchbuffer_addr_ofs
										.word scratchbuffer_addr_seg
s_PXENV_GET_CACHED_INFO_3_BufferLimit:	.word 0

s_PXENV_TFTP_OPEN:
s_PXENV_TFTP_OPEN_Status:				.word 0
s_PXENV_TFTP_OPEN_ServerIPAddress:		.long 0
s_PXENV_TFTP_OPEN_GatewayIPAddress:		.long 0
s_PXENV_TFTP_OPEN_FileName:				.asciz "haiku-pxe-loader"
										.fill 111
s_PXENV_TFTP_OPEN_TFTPPort:				.word (69 << 8)
s_PXENV_TFTP_OPEN_PacketSize:			.word 1456

s_PXENV_TFTP_GET_FSIZE:
s_PXENV_TFTP_GET_FSIZE_Status:			.word 0
s_PXENV_TFTP_GET_FSIZE_ServerIPAddress:	.long 0
s_PXENV_TFTP_GET_FSIZE_GatewayIPAddress:.long 0
s_PXENV_TFTP_GET_FSIZE_FileName:		.asciz "haiku-pxe-loader"
										.fill 111
s_PXENV_TFTP_GET_FSIZE_FileSize:		.long 0

s_PXENV_TFTP_READ:
s_PXENV_TFTP_READ_Status:				.word 0
s_PXENV_TFTP_READ_PacketNumber:			.word 0
s_PXENV_TFTP_READ_BufferSize:			.word 0
s_PXENV_TFTP_READ_Buffer:				.word scratchbuffer_addr_ofs
										.word scratchbuffer_addr_seg
end:
