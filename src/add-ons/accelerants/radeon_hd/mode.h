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


status_t create_mode_list(void);
status_t mode_sanity_check(display_mode *mode);


enum {
	/* CRTC1 registers */
	D1CRTC_H_TOTAL                 = 0x6000,
	D1CRTC_H_BLANK_START_END       = 0x6004,
	D1CRTC_H_SYNC_A                = 0x6008,
	D1CRTC_H_SYNC_A_CNTL           = 0x600C,
	D1CRTC_H_SYNC_B                = 0x6010,
	D1CRTC_H_SYNC_B_CNTL           = 0x6014,

	D1CRTC_V_TOTAL                 = 0x6020,
	D1CRTC_V_BLANK_START_END       = 0x6024,
	D1CRTC_V_SYNC_A                = 0x6028,
	D1CRTC_V_SYNC_A_CNTL           = 0x602C,
	D1CRTC_V_SYNC_B                = 0x6030,
	D1CRTC_V_SYNC_B_CNTL           = 0x6034,

	D1CRTC_CONTROL                 = 0x6080,
	D1CRTC_BLANK_CONTROL           = 0x6084,
	D1CRTC_INTERLACE_CONTROL       = 0x6088,
	D1CRTC_BLACK_COLOR             = 0x6098,
	D1CRTC_STATUS                  = 0x609C,
	D1CRTC_COUNT_CONTROL           = 0x60B4,

	/* D1GRPH registers */
	D1GRPH_ENABLE                  = 0x6100,
	D1GRPH_CONTROL                 = 0x6104,
	D1GRPH_LUT_SEL                 = 0x6108,
	D1GRPH_SWAP_CNTL               = 0x610C,
	D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x6110,
	D1GRPH_SECONDARY_SURFACE_ADDRESS = 0x6118,
	D1GRPH_PITCH                   = 0x6120,
	D1GRPH_SURFACE_OFFSET_X        = 0x6124,
	D1GRPH_SURFACE_OFFSET_Y        = 0x6128,
	D1GRPH_X_START                 = 0x612C,
	D1GRPH_Y_START                 = 0x6130,
	D1GRPH_X_END                   = 0x6134,
	D1GRPH_Y_END                   = 0x6138,
	D1GRPH_UPDATE                  = 0x6144,

	/* D1MODE */
	D1MODE_DESKTOP_HEIGHT          = 0x652C,
	D1MODE_VLINE_START_END         = 0x6538,
	D1MODE_VLINE_STATUS            = 0x653C,
	D1MODE_VIEWPORT_START          = 0x6580,
	D1MODE_VIEWPORT_SIZE           = 0x6584,
	D1MODE_EXT_OVERSCAN_LEFT_RIGHT = 0x6588,
	D1MODE_EXT_OVERSCAN_TOP_BOTTOM = 0x658C,
	D1MODE_DATA_FORMAT             = 0x6528,

	/* D1SCL */
	D1SCL_ENABLE                   = 0x6590,
	D1SCL_TAP_CONTROL              = 0x6594,
	D1MODE_CENTER                  = 0x659C, /* guess */
	D1SCL_HVSCALE                  = 0x65A4, /* guess */
	D1SCL_HFILTER                  = 0x65B0, /* guess */
	D1SCL_VFILTER                  = 0x65C0, /* guess */
	D1SCL_UPDATE                   = 0x65CC,
	D1SCL_DITHER                   = 0x65D4, /* guess */
	D1SCL_FLIP_CONTROL             = 0x65D8 /* guess */
};


#endif /*RADEON_HD_MODE_H*/
