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
#define CPSR_MODE_FIQ		0x11
#define CPSR_MODE_IRQ		0x12
#define CPSR_MODE_SVC		0x13
#define CPSR_MODE_ABT		0x17
#define CPSR_MODE_UND		0x1b
#define CPSR_MODE_SYS		0x1f

#define CPSR_T				0x20
#define CPSR_F				0x40
#define CPSR_I				0x80

#define SCTLR_HIGH_VECTORS	0x00002000

#define FSR_WNR				0x800
#define FSR_LPAE			0x200

#define FSR_FS_ALIGNMENT_FAULT		0x01
#define FSR_FS_ACCESS_FLAG_FAULT	0x06
#define FSR_FS_PERMISSION_FAULT_L1	0x0d
#define FSR_FS_PERMISSION_FAULT_L2	0x0f

#endif	/* _SYSTEM_ARCH_ARM_DEFS_H */
