/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
.text

#define TRAP_ERRC(name, vector) \
.globl name; \
.align 8; \
name: \
	pushl	$vector; \
	jmp		int_bottom
	
#define TRAP(name, vector) \
.globl name; \
.align 8; \
name: \
	pushl	$0; \
	pushl	$vector; \
	jmp		int_bottom

TRAP(trap0, 0)
TRAP(trap1, 1)
TRAP(trap2, 2)
TRAP(trap3, 3)
TRAP(trap4, 4)
TRAP(trap5, 5)
TRAP(trap6, 6)
TRAP(trap7, 7)
TRAP_ERRC(trap8, 8)
TRAP(trap9, 9)
TRAP_ERRC(trap10, 10)
TRAP_ERRC(trap11, 11)
TRAP_ERRC(trap12, 12)
TRAP_ERRC(trap13, 13)
TRAP_ERRC(trap14, 14)
/*TRAP(trap15, 15)*/
TRAP(trap16, 16)
TRAP_ERRC(trap17, 17)
TRAP(trap18, 18)

TRAP(trap32, 32)
TRAP(trap33, 33)
TRAP(trap34, 34)
TRAP(trap35, 35)
TRAP(trap36, 36)
TRAP(trap37, 37)
TRAP(trap38, 38)
TRAP(trap39, 39)
TRAP(trap40, 40)
TRAP(trap41, 41)
TRAP(trap42, 42)
TRAP(trap43, 43)
TRAP(trap44, 44)
TRAP(trap45, 45)
TRAP(trap46, 46)
TRAP(trap47, 47)

TRAP(trap99, 99)

TRAP(trap251, 251)
TRAP(trap252, 252)
TRAP(trap253, 253)
TRAP(trap254, 254)
TRAP(trap255, 255)

.align 8
.globl int_bottom
int_bottom:
	pusha
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movw	$0x10,%ax
	movw	%ax,%ds
	cld
	call	i386_handle_trap
	pop		%gs
	pop		%fs
	pop		%es
	pop		%ds
	popa
	addl	$8,%esp
	iret
