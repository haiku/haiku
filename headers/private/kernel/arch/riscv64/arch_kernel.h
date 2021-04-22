/*
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_RISCV64_KERNEL_H
#define _KERNEL_ARCH_RISCV64_KERNEL_H


#ifndef _ASSEMBLER
#ifdef __cplusplus
#	include <arch/cpu.h>
#endif
#endif


// memory layout
#define KERNEL_LOAD_BASE        0x80000000
#define KERNEL_LOAD_BASE_64_BIT 0xffffffff80000000ll


#if defined(__riscv64__)

// Base of the kernel address space.
#define KERNEL_BASE            (0x0000000000000000 + 0x1000)
#define KERNEL_TOP             (0x0000004000000000 - 1)
#define KERNEL_SIZE            ((KERNEL_TOP - KERNEL_BASE) + 1)

// Kernel physical memory map area.
#define KERNEL_PMAP_BASE        0x0000003000000000
#define KERNEL_PMAP_SIZE        0x1000000000

// Userspace address space layout.
#define USER_BASE               0xffffffc000000000
#define USER_BASE_ANY           USER_BASE
#define USER_SIZE               0x0000004000000000
#define USER_TOP                (USER_BASE + (USER_SIZE - 1))

#define KERNEL_USER_DATA_BASE   (USER_BASE + 0x3000000000)
#define USER_STACK_REGION       (USER_BASE + 0x3000000000)
#define USER_STACK_REGION_SIZE  ((USER_TOP - USER_STACK_REGION) + 1)

#else /* ! __riscv64__ */
	#warning Unknown RISC-V Architecture!
#endif

#endif	/* _KERNEL_ARCH_RISCV64_KERNEL_H */
