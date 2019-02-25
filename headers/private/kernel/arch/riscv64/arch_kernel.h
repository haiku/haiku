/*
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_RISCV64_KERNEL_H
#define _KERNEL_ARCH_RISCV64_KERNEL_H

#include <arch/cpu.h>

#warning Review arch_kernel.h

// memory layout
#define KERNEL_LOAD_BASE        0x80000000
#define KERNEL_LOAD_BASE_64_BIT 0xffffffff80000000ll


#if defined(__riscv64__)

// Base of the kernel address space.
#define KERNEL_BASE             0xffffff0000000000
#define KERNEL_SIZE             0x10000000000
#define KERNEL_TOP              (KERNEL_BASE + (KERNEL_SIZE - 1))
#define KERNEL_LOAD_BASE        0xffffffff80000000

// Kernel physical memory map area.
#define KERNEL_PMAP_BASE        0xffffff0000000000
#define KERNEL_PMAP_SIZE        0x8000000000

// Userspace address space layout.
// There is a 2MB hole just before the end of the bottom half of the address
// space. This means that if userland passes in a buffer that crosses into the
// uncanonical address region, it will be caught through a page fault.
#define USER_BASE               0x100000
#define USER_BASE_ANY           USER_BASE
#define USER_SIZE               (0x800000000000 - (0x200000 + 0x100000))
#define USER_TOP                (USER_BASE + (USER_SIZE - 1))

#define KERNEL_USER_DATA_BASE   0x7f0000000000
#define USER_STACK_REGION       0x7f0000000000
#define USER_STACK_REGION_SIZE  ((USER_TOP - USER_STACK_REGION) + 1)

#else /* ! __riscv64__ */
	#warning Unknown RISC-V Architecture!
#endif

#endif	/* _KERNEL_ARCH_RISCV64_KERNEL_H */
