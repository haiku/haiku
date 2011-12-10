/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_MODE_H
#define RADEON_HD_MODE_H


#include <create_display_modes.h>
#include <ddc.h>
#include <edid.h>

#include "gpu.h"


#define T_POSITIVE_SYNC (B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)

#define D1_REG_OFFSET 0x0000
#define D2_REG_OFFSET 0x0800
#define FMT1_REG_OFFSET 0x0000
#define FMT2_REG_OFFSET 0x800

#define OVERSCAN 0
	// TODO: Overscan and scaling support


status_t create_mode_list(void);
bool is_mode_supported(display_mode* mode);
status_t is_mode_sane(display_mode* mode);
uint32 radeon_dpms_capabilities(void);
uint32 radeon_dpms_mode(void);
void radeon_dpms_set(int mode);


#endif /*RADEON_HD_MODE_H*/
