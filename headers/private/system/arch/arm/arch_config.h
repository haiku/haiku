/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM_CONFIG_H
#define _KERNEL_ARCH_ARM_CONFIG_H

#include "arch_arm_version.h"

#define FUNCTION_CALL_PARAMETER_ALIGNMENT_TYPE	unsigned int

#define STACK_GROWS_DOWNWARDS

// If we're building on ARMv5 or older, all our atomics need to be syscalls... :(
#if __ARM_ARCH__ <= 5
#define ATOMIC_FUNCS_ARE_SYSCALLS
#endif

// If we're building on ARMv6 or older, 64-bit atomics need to be syscalls...
#if __ARM_ARCH__ <= 6
#define ATOMIC64_FUNCS_ARE_SYSCALLS
#endif

#endif	/* _KERNEL_ARCH_ARM_CONFIG_H */
