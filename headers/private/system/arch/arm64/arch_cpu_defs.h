/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_ARM64_DEFS_H
#define _SYSTEM_ARCH_ARM64_DEFS_H


#define SPINLOCK_PAUSE()	__asm__ __volatile__("yield")


#endif	/* _SYSTEM_ARCH_ARM64_DEFS_H */
