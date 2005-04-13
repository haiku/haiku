/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/*
** syscall interface works as such:
** eax has syscall #
** ecx has number of args (0-16)
** edx has pointer to buffer containing args from first to last
** each is verified to make sure someone doesnt try to clobber it
*/

#define SYSCALL0(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall0

#define SYSCALL1(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall1

#define SYSCALL2(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall2

#define SYSCALL3(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall3

#define SYSCALL4(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall4

#define SYSCALL5(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall5

#define SYSCALL6(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall6


#define SYSCALL7(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall7

#define SYSCALL8(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall8

#define SYSCALL9(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall9

#define SYSCALL10(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall10

#define SYSCALL11(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall11

#define SYSCALL12(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall12

#define SYSCALL13(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall13

#define SYSCALL14(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall14

#define SYSCALL15(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall15

#define SYSCALL16(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp		syscall16



/* we will optimize by just passing a ptr to the place where the caller
** would have dumped the args */

syscall0:
	movl	$0, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall1:
	movl	$1, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall2:
	movl	$2, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall3:
	movl	$3, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall4:
	movl	$4, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall5:
	movl	$5, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall6:
	movl	$6, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall7:
	movl	$7, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall8:
	movl	$8, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall9:
	movl	$9, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall10:
	movl	$10, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall11:
	movl	$11, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall12:
	movl	$12, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall13:
	movl	$13, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall14:
	movl	$14, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall15:
	movl	$15, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

syscall16:
	movl	$16, %ecx
	lea		4(%esp), %edx
	int		$99
	ret

