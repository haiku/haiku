/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Michael Noisternig. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <arch/x86/descriptors.h>

#include "syscall_numbers.h"

#define FUNCTION(x) .global x; .type x,@function; x

.text

#define TRAP_ERRC(name, vector) \
.globl name; \
.align 8; \
name: \
	pushl	$vector; \
	pushl	$-1; \
	pushl	$-1; \
	jmp		int_bottom

#define TRAP(name, vector) \
.globl name; \
.align 8; \
name: \
	pushl	$0; \
	pushl	$vector; \
	pushl	%edx; \
	pushl	%eax; \
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
TRAP(trap19, 19)

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

.align 16
.globl int_bottom
int_bottom:
	pusha
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$KERNEL_DATA_SEG,%eax
	cld
	movl	%eax,%ds
	movl	%eax,%es
	movl	%ss,%ebx
	movl	%esp,%esi
	cmpl	%eax,%ebx  // check if we changed the stack
	jne		custom_stack
  kernel_stack:
	call	i386_handle_trap
	pop		%gs
	pop		%fs
	pop		%es
	pop		%ds
	popa
	addl	$16,%esp
	iret

	// custom stack -> copy registers to kernel stack and switch there
  custom_stack:
	movl	%dr3,%edx  // get_current_thread
	movl	%eax,%es
	addl	_interrupt_stack_offset,%edx
	lss		(%edx),%esp
	movl	%ebx,%ds
	subl	$92,%esp
	movl	%esp,%edi
	movl	$21,%ecx
	rep		movsl
	movl	%eax,%ds
	subl	$84,%esi
	movl	%esi,(%edi)  // save custom stack address
	movl	%ebx,4(%edi)
	call	i386_handle_trap
	lss		84(%esp),%esp  // reload custom stack address
	pop		%gs
	pop		%fs
	pop		%es
	pop		%ds
	popa
	addl	$16,%esp
	iret

_interrupt_stack_offset:
.long	0

// void	i386_stack_init(struct farcall *interrupt_stack_offset)
	/* setup in arch_thread.c: arch_thread_init_thread_struct() */
FUNCTION(i386_stack_init):
	movl	4(%esp),%eax
	movl	%eax,_interrupt_stack_offset
	ret

// void i386_stack_switch(struct farcall new_stack)
FUNCTION(i386_stack_switch):
	movl	%dr3,%eax  // get_current_thread
	movl	(%esp),%edx
	pushf
	popl	%ecx
	addl	_interrupt_stack_offset,%eax
	cli
	pushl	%ss
	cmpl	$KERNEL_DATA_SEG,(%esp)
	je		kernel_stack2
	popl	%eax
	jmp		switch
  kernel_stack2:
	popl	4(%eax)
	movl	%esp,(%eax)
  switch:
	lss		4(%esp),%esp
	pushl	%ecx
	popf
	jmp		*%edx

FUNCTION(i386_return_from_signal):
	addl	$12, %esp	// Flushes the 3 arguments to sa_handler
	movl	$SYSCALL_RESTORE_SIGNAL_FRAME, %eax	// This syscall will restore the cpu context to the
	movl	$0, %ecx	// one existing before calling the signal handler
	lea		4(%esp), %edx
	int		$99
	ret
FUNCTION(i386_end_return_from_signal):
	
