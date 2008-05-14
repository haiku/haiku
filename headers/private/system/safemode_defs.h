/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SAFEMODE_DEFS_H
#define _SYSTEM_SAFEMODE_DEFS_H


// these are BeOS compatible additions to the safemode
// constants in the driver_settings.h header

#define B_SAFEMODE_DISABLE_USER_ADD_ONS		"disableuseraddons"
#define B_SAFEMODE_DISABLE_IDE_DMA			"disableidedma"
#define B_SAFEMODE_DISABLE_ACPI				"disable_acpi"
#define B_SAFEMODE_DISABLE_APM				"disable_apm"
#define B_SAFEMODE_DISABLE_SMP				"disable_smp"
#define B_SAFEMODE_DISABLE_HYPER_THREADING	"disable_hyperthreading"
#define B_SAFEMODE_FAIL_SAFE_VIDEO_MODE		"fail_safe_video_mode"


#endif	/* _SYSTEM_SAFEMODE_DEFS_H */
