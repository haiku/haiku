/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#if defined(ARCH_x86)
#	include "arch/x86/syscalls.inc"
#elif defined(ARCH_x86_64)
#	include "arch/x86_64/syscalls.inc"
#elif defined(ARCH_alpha)
#	include "arch/alpha/syscalls.inc"
#elif defined(ARCH_sh4)
#	include "arch/sh4/syscalls.inc"
#elif defined(ARCH_sparc)
#	include "arch/sparc/syscalls.inc"
#elif defined(ARCH_sparc64)
#	include "arch/sparc64/syscalls.inc"
#elif defined(ARCH_ppc)
#	include "arch/ppc/syscalls.inc"
#elif defined(ARCH_arm)
#	include "arch/arm/syscalls.inc"
#elif defined(ARCH_arm64)
#	include "arch/arm64/syscalls.inc"
#elif defined(ARCH_m68k)
#	include "arch/m68k/syscalls.inc"
#elif defined(ARCH_riscv64)
#	include "arch/riscv64/syscalls.inc"
#else
#error	No syscalls macros defined for this architecture!
#endif

/* include the auto-generated file syscalls file */
#include "syscalls.S.inc"
