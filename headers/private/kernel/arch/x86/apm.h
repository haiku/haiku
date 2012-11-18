/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_APM_H
#define KERNEL_APM_H

#include <SupportDefs.h>

#include <arch/x86/apm_defs.h>


struct kernel_args;


// int 0x15 APM definitions
#define BIOS_APM_CHECK				0x5300
#define BIOS_APM_CONNECT_32_BIT		0x5303
#define BIOS_APM_DISCONNECT			0x5304
#define BIOS_APM_CPU_IDLE			0x5305
#define BIOS_APM_CPU_BUSY			0x5306
#define BIOS_APM_SET_STATE			0x5307
#define BIOS_APM_ENABLE				0x5308
#define BIOS_APM_GET_POWER_STATUS	0x530a
#define BIOS_APM_GET_EVENT			0x530b
#define BIOS_APM_GET_STATE			0x530c
#define BIOS_APM_VERSION			0x530e
#define BIOS_APM_ENGAGE				0x530f

// APM devices
#define APM_ALL_DEVICES				0x0001

// APM power states
#define APM_POWER_STATE_ENABLED		0x0000
#define APM_POWER_STATE_STANDBY		0x0001
#define APM_POWER_STATE_SUSPEND		0x0002
#define APM_POWER_STATE_OFF			0x0003

typedef struct apm_info {
	uint16	version;
	uint16	flags;
	uint16	code32_segment_base;
	uint32	code32_segment_offset;
	uint16	code32_segment_length;
	uint16	code16_segment_base;
	uint16	code16_segment_length;
	uint16	data_segment_base;
	uint16	data_segment_length;
} apm_info;


#ifndef _BOOT_MODE
#ifndef __x86_64__
#ifdef __cplusplus
extern "C" {
#endif

status_t apm_shutdown(void);
status_t apm_init(struct kernel_args *args);

#ifdef __cplusplus
}
#endif
#endif	// !__x86_64__
#endif	// !_BOOT_MODE

#endif	/* KERNEL_APM_H */
