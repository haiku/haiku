/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_KERNEL_H_
#define _KERNEL_ARCH_ARM64_ARCH_KERNEL_H_


#include <kernel/arch/cpu.h>


// memory layout
#define KERNEL_BASE		0xffff000000000000
#define KERNEL_SIZE		0x8000000000
#define KERNEL_TOP		(KERNEL_BASE + (KERNEL_SIZE - 1))

#define USER_BASE		0x1000
#define USER_BASE_ANY	USER_BASE
#define USER_SIZE		(0x0001000000000000UL - USER_BASE)
#define USER_TOP		(0x0001000000000000UL - 1)

#define KERNEL_USER_DATA_BASE	0x60000000
#define USER_STACK_REGION		0x70000000
#define USER_STACK_REGION_SIZE	((USER_TOP - USER_STACK_REGION) + 1)


#endif /* _KERNEL_ARCH_ARM64_ARCH_KERNEL_H_ */
