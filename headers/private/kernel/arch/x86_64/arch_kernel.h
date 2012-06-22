/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_KERNEL_H
#define _KERNEL_ARCH_X86_64_KERNEL_H


#ifndef _ASSEMBLER
#	include <arch/cpu.h>
#endif


// Base of the kernel address space.
// When compiling the bootloader, KERNEL_BASE is set to the x86 base address,
// the correct 64-bit addresses are calculated differently.
// For the kernel, this is the base of the kernel address space. This is NOT
// the address where the kernel is loaded to: the kernel is loaded in the top
// 2GB of the virtual address space as required by GCC's kernel code model.
// The whole kernel address space is the top 512GB of the address space.
#ifdef _BOOT_MODE
# define KERNEL_BASE	0x80000000
#else
# define KERNEL_BASE	0xFFFFFF8000000000
#endif

#define KERNEL_SIZE		0x8000000000
#define KERNEL_TOP  	(KERNEL_BASE + (KERNEL_SIZE - 1))


// Userspace address space layout.
#define USER_BASE		0x0
#define USER_BASE_ANY	0x100000
#define USER_SIZE		0x800000000000
#define USER_TOP		(USER_BASE + USER_SIZE)

#define KERNEL_USER_DATA_BASE	0x7FFFEFFF0000
#define USER_STACK_REGION		0x7FFFF0000000
#define USER_STACK_REGION_SIZE	(USER_TOP - USER_STACK_REGION)


#endif	/* _KERNEL_ARCH_X86_64_KERNEL_H */
