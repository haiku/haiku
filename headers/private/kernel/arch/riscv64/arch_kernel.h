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
#define KERNEL_LOAD_BASE_64_BIT 0xffffffc000000000


#if (defined(__riscv) && __riscv_xlen == 64)

// Base of the kernel address space.
#define KERNEL_BASE               0xffffffc000000000
#define KERNEL_SIZE               0x0000004000000000
#define KERNEL_TOP                (KERNEL_BASE + (KERNEL_SIZE - 1))

// Userspace address space layout.
#define USER_BASE            (0x0000000000000000 + 0x1000)
#define USER_BASE_ANY         USER_BASE
#define USER_TOP             (0x0000004000000000 - 1)
#define USER_SIZE            (USER_TOP - USER_BASE + 1)

#define KERNEL_USER_DATA_BASE   (USER_BASE + 0x3000000000)
#define USER_STACK_REGION       (USER_BASE + 0x3000000000)
#define USER_STACK_REGION_SIZE  ((USER_TOP - USER_STACK_REGION) + 1)

#else /* ! riscv64 */
	#warning Unknown RISC-V Architecture!
#endif

#endif	/* _KERNEL_ARCH_RISCV64_KERNEL_H */
