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
#define CPSR_MODE_SVC		0x13
#define CPSR_MODE_SYS		0x1f

#define CPSR_T				0x20
#define CPSR_F				0x40
#define CPSR_I				0x80

#define FSR_WNR				0x800

#endif	/* _SYSTEM_ARCH_ARM_DEFS_H */
