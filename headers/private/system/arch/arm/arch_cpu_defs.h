/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_ARM_DEFS_H
#define _SYSTEM_ARCH_ARM_DEFS_H


#define SPINLOCK_PAUSE()	do {} while (false)

#define CPSR_MODE_MASK		0x1f
#define CPSR_MODE_USR		0x10

#define FSR_WNR				0x800

#endif	/* _SYSTEM_ARCH_ARM_DEFS_H */
