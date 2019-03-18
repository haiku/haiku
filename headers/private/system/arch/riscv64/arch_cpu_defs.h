/*
 * Copyright 2018-2019, Haiku, Inc. All rights reserved
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 * 	Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _SYSTEM_ARCH_RISCV64_DEFS_H
#define _SYSTEM_ARCH_RISCV64_DEFS_H


#define SPINLOCK_PAUSE()	do {} while (false)

// Status register flags
#define ARCH_SR_SIE		0x00000002 // Supervisor Interrupt Enable
#define ARCH_SR_SPIE		0x00000020 // Previous Supervisor Interrupt En
#define ARCH_SR_SPP		0x00000100 // Previously Supervisor
#define ARCH_SR_SUM		0x00040000 // Supervisor may access user memory

#define ARCH_SR_FS		0x00006000 // Floating Point Status
#define ARCH_SR_FS_OFF		0x00000000
#define ARCH_SR_FS_INITIAL	0x00002000
#define ARCH_SR_FS_CLEAN	0x00004000
#define ARCH_SR_FS_DIRTY	0x00006000

#define ARCH_SR_XS		0x00018000 // Extension Status
#define ARCH_SR_XS_OFF		0x00000000
#define ARCH_SR_XS_INITIAL	0x00008000
#define ARCH_SR_XS_CLEAN	0x00010000
#define ARCH_SR_XS_DIRTY	0x00018000

#define ARCH_SR_SD		0x8000000000000000 // FS/XS dirty

// Interrupt Enable and Interrupt Pending
#define ARCH_SIE_SSIE		0x00000002 // Software Interrupt Enable
#define ARCH_SIE_STIE		0x00000020 // Timer Interrupt Enable
#define ARCH_SIE_SEIE		0x00000200 // External Interrupt Enable


#endif	/* _SYSTEM_ARCH_RISCV64_DEFS_H */

