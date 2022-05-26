/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the NewOS License.
 */

// Relocatable. Before calling, smp_trampoline_args should point
// to a struct trampoline_args (see smp.cpp). This pointer should
// be 16-byte aligned, below 1MB and identity-mapped.
.globl smp_trampoline
.globl smp_trampoline_end
.globl smp_trampoline_args

#include <asm_defs.h>

#include <arch/x86/descriptors.h>
#include "mmu.h"

.code16
smp_trampoline:
	cli

//  movl 0xdeadbeef, esi
	.byte 0x66
	.byte 0xbe
smp_trampoline_args:
	.long 0xdeadbeef

	// Set up the stack pointer for 16-bit mode
	movl %esi, %eax
	shrl $4, %eax
	movw %ax, %ss
	xorw %sp, %sp

	popl %ebx		// trampoline

	// Initialize GDT
	popl %edx		// gdt
	lgdt (%edx)

	// Switch to protected mode
	movl   %cr0, %eax
	orl    $0x01, %eax
	movl   %eax, %cr0

	pushl  $KERNEL_CODE_SELECTOR
	leal  (trampoline_32 - smp_trampoline)(%ebx), %eax
	pushl  %eax
.code32
	.byte 0x66
	lret

trampoline_32:
	// Set data segments.
	movw   $KERNEL_DATA_SELECTOR, %ax
	movw   %ax, %ds
	movw   %ax, %es
	movw   %ax, %fs
	movw   %ax, %gs
	movw   %ax, %ss

	// Set up the stack pointer for 32-bit mode
	movl	%esi, %esp
	addl	$8, %esp

	// Point CR3 to the kernel's page dir.
	popl	%eax		// page_dir
	movl	%eax, %cr3

	popl	%ebx		// kernel_entry
	popl	%ecx		// kernel_args
	popl	%edx		// current_cpu

	// Set the stack pointer, write to the sentinel and clear the stack frame
	popl	%ebp		// stack_top
	movl	$0, (%esp)	// sentinel
	movl	%ebp, %esp
	xorl	%ebp, %ebp

	movl	$0x80010021, %eax
	movl	%eax, %cr0

	cli
	cld
	fninit

	pushl	%edx		// currentCpu
	pushl	%ecx		// kernelArgs
	pushl	$0x0		// fake return address
	pushl	$KERNEL_CODE_SELECTOR
	pushl	%ebx		// kernelEntry
	lret
smp_trampoline_end:
