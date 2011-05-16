/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SAFEMODE_DEFS_H
#define _SYSTEM_SAFEMODE_DEFS_H


#define B_SAFEMODE_DISABLE_USER_ADD_ONS		"disable_user_addons"
#define B_SAFEMODE_DISABLE_IDE_DMA			"disable_ide_dma"
#define B_SAFEMODE_DISABLE_IOAPIC			"disable_ioapic"
#define B_SAFEMODE_DISABLE_ACPI				"disable_acpi"
#define B_SAFEMODE_DISABLE_APIC				"disable_apic"
#define B_SAFEMODE_DISABLE_APM				"disable_apm"
#define B_SAFEMODE_DISABLE_SMP				"disable_smp"
#define B_SAFEMODE_DISABLE_HYPER_THREADING	"disable_hyperthreading"
#define B_SAFEMODE_FAIL_SAFE_VIDEO_MODE		"fail_safe_video_mode"
#define B_SAFEMODE_4_GB_MEMORY_LIMIT		"4gb_memory_limit"

#if DEBUG_SPINLOCK_LATENCIES
#	define B_SAFEMODE_DISABLE_LATENCY_CHECK	"disable_latency_check"
#endif

#endif	/* _SYSTEM_SAFEMODE_DEFS_H */
