/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */

/* Copyright for portions of this file (Xorg radeonhd registers)
 *
 * Copyright 2007, 2008  Luc Verhaegen <libv@exsuse.de>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
 * Copyright 2007, 2008  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef RADEON_HD_H
#define RADEON_HD_H


#include "lock.h"

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>


#define VENDOR_ID_ATI			0x1002

// TODO : Remove masks as they don't apply to radeon
#define RADEON_TYPE_FAMILY_MASK	0xf000
#define RADEON_TYPE_GROUP_MASK	0xfff0
#define RADEON_TYPE_MODEL_MASK	0xffff

#define RADEON_R600	0x0600
#define RADEON_R700	0x0700
#define RADEON_R800	0x0800

#define DEVICE_NAME				"radeon_hd"
#define RADEON_ACCELERANT_NAME	"radeon_hd.accelerant"


struct DeviceType {
	uint32			type;

	DeviceType(int t)
	{
		type = t;
	}

	DeviceType& operator=(int t)
	{
		type = t;
		return *this;
	}

	bool InFamily(uint32 family) const
	{
		return (type & RADEON_TYPE_FAMILY_MASK) == family;
	}

	bool InGroup(uint32 group) const
	{
		return (type & RADEON_TYPE_GROUP_MASK) == group;
	}

	bool IsModel(uint32 model) const
	{
		return (type & RADEON_TYPE_MODEL_MASK) == model;
	}
};


// info about PLL on graphics card
struct pll_info {
	uint32			reference_frequency;
	uint32			max_frequency;
	uint32			min_frequency;
	uint32			divisor_register;
};


struct ring_buffer {
	struct lock		lock;
	uint32			register_base;
	uint32			offset;
	uint32			size;
	uint32			position;
	uint32			space_left;
	uint8*			base;
};


struct overlay_registers;


struct radeon_shared_info {
	area_id			mode_list_area;		// area containing display mode list
	uint32			mode_count;

	display_mode	current_mode;
	uint32			bytes_per_row;
	uint32			bits_per_pixel;
	uint32			dpms_mode;

	area_id			registers_area;			// area of memory mapped registers
	uint8*			status_page;
	addr_t			physical_status_page;
	uint8*			graphics_memory;
	addr_t			physical_graphics_memory;
	uint32			graphics_memory_size;

	addr_t			frame_buffer;
	uint32			frame_buffer_offset;

	struct lock		accelerant_lock;
	struct lock		engine_lock;

	ring_buffer		primary_ring_buffer;

	int32			overlay_channel_used;
	bool			overlay_active;
	uint32			overlay_token;
	addr_t			physical_overlay_registers;
	uint32			overlay_offset;

	bool			hardware_cursor_enabled;
	sem_id			vblank_sem;

	uint8*			cursor_memory;
	addr_t			physical_cursor_memory;
	uint32			cursor_buffer_offset;
	uint32			cursor_format;
	bool			cursor_visible;
	uint16			cursor_hot_x;
	uint16			cursor_hot_y;

	DeviceType		device_type;
	char			device_identifier[32];
	struct pll_info	pll_info;
};

//----------------- ioctl() interface ----------------

// magic code for ioctls
#define RADEON_PRIVATE_DATA_MAGIC		'rdhd'

// list ioctls
enum {
	RADEON_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,

	RADEON_GET_DEVICE_NAME,
	RADEON_ALLOCATE_GRAPHICS_MEMORY,
	RADEON_FREE_GRAPHICS_MEMORY
};

// retrieve the area_id of the kernel/accelerant shared info
struct radeon_get_private_data {
	uint32	magic;				// magic number
	area_id	shared_info_area;
};

// allocate graphics memory
struct radeon_allocate_graphics_memory {
	uint32	magic;
	uint32	size;
	uint32	alignment;
	uint32	flags;
	uint32	buffer_base;
};

// free graphics memory
struct radeon_free_graphics_memory {
	uint32 	magic;
	uint32	buffer_base;
};

// ----------------------------------------------------------
// Register definitions, taken from X driver

// Generic Radeon registers
enum {
	CLOCK_CNTL_INDEX	= 0x8,  /* (RW) */
	CLOCK_CNTL_DATA		= 0xC,  /* (RW) */
	BUS_CNTL			= 0x4C, /* (RW) */
	MC_IND_INDEX		= 0x70, /* (RW) */
	MC_IND_DATA			= 0x74, /* (RW) */
	RS600_MC_INDEX		= 0x70,
	RS600_MC_DATA		= 0x74,
	RS690_MC_INDEX		= 0x78,
	RS690_MC_DATA		= 0x7c,
	RS780_MC_INDEX		= 0x28f8,
	RS780_MC_DATA		= 0x28fc,

	RS60_MC_NB_MC_INDEX	= 0x78,
	RS60_MC_NB_MC_DATA	= 0x7C,
	CONFIG_CNTL			= 0xE0,
	PCIE_RS69_MC_INDEX	= 0xE8,
	PCIE_RS69_MC_DATA	= 0xEC,
	R5XX_CONFIG_MEMSIZE	= 0x00F8,
	
	HDP_FB_LOCATION		= 0x0134,
	
	SEPROM_CNTL1		= 0x1C0,  /* (RW) */
	
	AGP_BASE			= 0x0170,
	
	GPIOPAD_MASK		= 0x198,  /* (RW) */
	GPIOPAD_A			= 0x19C,  /* (RW) */
	GPIOPAD_EN			= 0x1A0,  /* (RW) */
	VIPH_CONTROL		= 0xC40,  /* (RW) */
	
	ROM_CNTL			= 0x1600,
	GENERAL_PWRMGT		= 0x0618,
	LOW_VID_LOWER_GPIO_CNTL = 0x0724,
	MEDIUM_VID_LOWER_GPIO_CNTL = 0x0720,
	HIGH_VID_LOWER_GPIO_CNTL = 0x071C,
	CTXSW_VID_LOWER_GPIO_CNTL = 0x0718,
	LOWER_GPIO_ENABLE	= 0x0710,

	/* VGA registers */
	VGA_RENDER_CONTROL		= 0x0300,
	VGA_MODE_CONTROL		= 0x0308,
	VGA_MEMORY_BASE_ADDRESS	= 0x0310,
	VGA_HDP_CONTROL			= 0x0328,
	D1VGA_CONTROL			= 0x0330,
	D2VGA_CONTROL			= 0x0338,
	
	EXT1_PPLL_REF_DIV_SRC	= 0x0400,
	EXT1_PPLL_REF_DIV		= 0x0404,
	EXT1_PPLL_UPDATE_LOCK	= 0x0408,
	EXT1_PPLL_UPDATE_CNTL	= 0x040C,
	EXT2_PPLL_REF_DIV_SRC	= 0x0410,
	EXT2_PPLL_REF_DIV		= 0x0414,
	EXT2_PPLL_UPDATE_LOCK	= 0x0418,
	EXT2_PPLL_UPDATE_CNTL	= 0x041C,
	
	EXT1_PPLL_FB_DIV		= 0x0430,
	EXT2_PPLL_FB_DIV		= 0x0434,
	EXT1_PPLL_POST_DIV_SRC	= 0x0438,
	EXT1_PPLL_POST_DIV		= 0x043C,
	EXT2_PPLL_POST_DIV_SRC	= 0x0440,
	EXT2_PPLL_POST_DIV		= 0x0444,
	EXT1_PPLL_CNTL			= 0x0448,
	EXT2_PPLL_CNTL			= 0x044C,
	P1PLL_CNTL				= 0x0450,
	P2PLL_CNTL				= 0x0454,
	P1PLL_INT_SS_CNTL		= 0x0458,
	P2PLL_INT_SS_CNTL		= 0x045C,
	
	P1PLL_DISP_CLK_CNTL		= 0x0468, /* rv620+ */
	P2PLL_DISP_CLK_CNTL		= 0x046C, /* rv620+ */
	EXT1_SYM_PPLL_POST_DIV	= 0x0470, /* rv620+ */
	EXT2_SYM_PPLL_POST_DIV	= 0x0474, /* rv620+ */
	
	PCLK_CRTC1_CNTL			= 0x0480,
	PCLK_CRTC2_CNTL			= 0x0484,

	// TODO : xorg reverse engineered registers
};


// ATI r600 specific
enum _r6xxRegs {
	/* MCLK */
	R6_MCLK_PWRMGT_CNTL			= 0x620,
	/* I2C */
	R6_DC_I2C_CONTROL			= 0x7D30,  /* (RW) */
	R6_DC_I2C_ARBITRATION		= 0x7D34,  /* (RW) */
	R6_DC_I2C_INTERRUPT_CONTROL	= 0x7D38,  /* (RW) */
	R6_DC_I2C_SW_STATUS			= 0x7d3c,  /* (RW) */
	R6_DC_I2C_DDC1_SPEED		= 0x7D4C,  /* (RW) */
	R6_DC_I2C_DDC1_SETUP		= 0x7D50,  /* (RW) */
	R6_DC_I2C_DDC2_SPEED		= 0x7D54,  /* (RW) */
	R6_DC_I2C_DDC2_SETUP		= 0x7D58,  /* (RW) */
	R6_DC_I2C_DDC3_SPEED		= 0x7D5C,  /* (RW) */
	R6_DC_I2C_DDC3_SETUP		= 0x7D60,  /* (RW) */
	R6_DC_I2C_TRANSACTION0		= 0x7D64,  /* (RW) */
	R6_DC_I2C_TRANSACTION1		= 0x7D68,  /* (RW) */
	R6_DC_I2C_DATA				= 0x7D74,  /* (RW) */
	R6_DC_I2C_DDC4_SPEED		= 0x7DB4,  /* (RW) */
	R6_DC_I2C_DDC4_SETUP		= 0x7DBC,  /* (RW) */
	R6_DC_GPIO_DDC4_MASK		= 0x7E00,  /* (RW) */
	R6_DC_GPIO_DDC4_A			= 0x7E04,  /* (RW) */
	R6_DC_GPIO_DDC4_EN			= 0x7E08,  /* (RW) */
	R6_DC_GPIO_DDC1_MASK		= 0x7E40,  /* (RW) */
	R6_DC_GPIO_DDC1_A			= 0x7E44,  /* (RW) */
	R6_DC_GPIO_DDC1_EN			= 0x7E48,  /* (RW) */
	R6_DC_GPIO_DDC1_Y			= 0x7E4C,  /* (RW) */
	R6_DC_GPIO_DDC2_MASK		= 0x7E50,  /* (RW) */
	R6_DC_GPIO_DDC2_A			= 0x7E54,  /* (RW) */
	R6_DC_GPIO_DDC2_EN			= 0x7E58,  /* (RW) */
	R6_DC_GPIO_DDC2_Y			= 0x7E5C,  /* (RW) */
	R6_DC_GPIO_DDC3_MASK		= 0x7E60,  /* (RW) */
	R6_DC_GPIO_DDC3_A			= 0x7E64,  /* (RW) */
	R6_DC_GPIO_DDC3_EN			= 0x7E68,  /* (RW) */
	R6_DC_GPIO_DDC3_Y			= 0x7E6C   /* (RW) */
};


// PLL Clock Controls
enum {
    /* CLOCK_CNTL_INDEX */
    PLL_ADDR		= (0x3f << 0),
    PLL_WR_EN		= (0x1 << 7),
    PPLL_DIV_SEL	= (0x3 << 8),

    /* SPLL_FUNC_CNTL */
    SPLL_CHG_STATUS	= (0x1 << 29),
    SPLL_BYPASS_EN	= (0x1 << 25),

    /* MC_IND_INDEX */
    MC_IND_ADDR		= (0xffff << 0),
    MC_IND_SEQ_RBS_0 = (0x1 << 16),
    MC_IND_SEQ_RBS_1 = (0x1 << 17),
    MC_IND_SEQ_RBS_2 = (0x1 << 18),
    MC_IND_SEQ_RBS_3 = (0x1 << 19),
    MC_IND_AIC_RBS   = (0x1 << 20),
    MC_IND_CITF_ARB0 = (0x1 << 21),
    MC_IND_CITF_ARB1 = (0x1 << 22),
    MC_IND_WR_EN     = (0x1 << 23),
    MC_IND_RD_INV    = (0x1 << 24)
};


/* CLOCK_CNTL_DATA */
#define PLL_DATA 0xffffffff
/* MC_IND_DATA */
#define MC_IND_ALL (MC_IND_SEQ_RBS_0 | MC_IND_SEQ_RBS_1 \
	| MC_IND_SEQ_RBS_2 | MC_IND_SEQ_RBS_3 \
	| MC_IND_AIC_RBS | MC_IND_CITF_ARB0 \
	| MC_IND_CITF_ARB1)
#define MC_IND_DATA_BIT 0xffffffff


// cursor
#define RADEON_CURSOR_CONTROL			0x70080
#define RADEON_CURSOR_BASE				0x70084
#define RADEON_CURSOR_POSITION			0x70088
#define RADEON_CURSOR_PALETTE			0x70090 // (- 0x7009f)
#define RADEON_CURSOR_SIZE				0x700a0
#define CURSOR_ENABLED					(1UL << 31)
#define CURSOR_FORMAT_2_COLORS			(0UL << 24)
#define CURSOR_FORMAT_3_COLORS			(1UL << 24)
#define CURSOR_FORMAT_4_COLORS			(2UL << 24)
#define CURSOR_FORMAT_ARGB				(4UL << 24)
#define CURSOR_FORMAT_XRGB				(5UL << 24)
#define CURSOR_POSITION_NEGATIVE		0x8000
#define CURSOR_POSITION_MASK			0x3fff

// overlay flip
#define COMMAND_OVERLAY_FLIP			(0x11 << 23)
#define COMMAND_OVERLAY_CONTINUE		(0 << 21)
#define COMMAND_OVERLAY_ON				(1 << 21)
#define COMMAND_OVERLAY_OFF				(2 << 21)
#define OVERLAY_UPDATE_COEFFICIENTS		0x1

// 2D acceleration
#define XY_COMMAND_SOURCE_BLIT			0x54c00006
#define XY_COMMAND_COLOR_BLIT			0x54000004
#define XY_COMMAND_SETUP_MONO_PATTERN	0x44400007
#define XY_COMMAND_SCANLINE_BLIT		0x49400001
#define COMMAND_COLOR_BLIT				0x50000003
#define COMMAND_BLIT_RGBA				0x00300000

#define COMMAND_MODE_SOLID_PATTERN		0x80
#define COMMAND_MODE_CMAP8				0x00
#define COMMAND_MODE_RGB15				0x02
#define COMMAND_MODE_RGB16				0x01
#define COMMAND_MODE_RGB32				0x03

// display

#define DISPLAY_CONTROL_ENABLED			(1UL << 31)
#define DISPLAY_CONTROL_GAMMA			(1UL << 30)
#define DISPLAY_CONTROL_COLOR_MASK		(0x0fUL << 26)
#define DISPLAY_CONTROL_CMAP8			(2UL << 26)
#define DISPLAY_CONTROL_RGB15			(4UL << 26)
#define DISPLAY_CONTROL_RGB16			(5UL << 26)
#define DISPLAY_CONTROL_RGB32			(6UL << 26)

// PCI bridge memory management

// overlay

#define RADEON_OVERLAY_UPDATE			0x30000
#define RADEON_OVERLAY_TEST				0x30004
#define RADEON_OVERLAY_STATUS			0x30008
#define RADEON_OVERLAY_EXTENDED_STATUS	0x3000c
#define RADEON_OVERLAY_GAMMA_5			0x30010
#define RADEON_OVERLAY_GAMMA_4			0x30014
#define RADEON_OVERLAY_GAMMA_3			0x30018
#define RADEON_OVERLAY_GAMMA_2			0x3001c
#define RADEON_OVERLAY_GAMMA_1			0x30020
#define RADEON_OVERLAY_GAMMA_0			0x30024

struct overlay_scale {
	uint32 _reserved0 : 3;
	uint32 horizontal_scale_fraction : 12;
	uint32 _reserved1 : 1;
	uint32 horizontal_downscale_factor : 3;
	uint32 _reserved2 : 1;
	uint32 vertical_scale_fraction : 12;
};

#define OVERLAY_FORMAT_RGB15			0x2
#define OVERLAY_FORMAT_RGB16			0x3
#define OVERLAY_FORMAT_RGB32			0x1
#define OVERLAY_FORMAT_YCbCr422			0x8
#define OVERLAY_FORMAT_YCbCr411			0x9
#define OVERLAY_FORMAT_YCbCr420			0xc

#define OVERLAY_MIRROR_NORMAL			0x0
#define OVERLAY_MIRROR_HORIZONTAL		0x1
#define OVERLAY_MIRROR_VERTICAL			0x2

// The real overlay registers are written to using an update buffer

struct overlay_registers {
	uint32 buffer_rgb0;
	uint32 buffer_rgb1;
	uint32 buffer_u0;
	uint32 buffer_v0;
	uint32 buffer_u1;
	uint32 buffer_v1;
	// (0x18) OSTRIDE - overlay stride
	uint16 stride_rgb;
	uint16 stride_uv;
	// (0x1c) YRGB_VPH - Y/RGB vertical phase
	uint16 vertical_phase0_rgb;
	uint16 vertical_phase1_rgb;
	// (0x20) UV_VPH - UV vertical phase
	uint16 vertical_phase0_uv;
	uint16 vertical_phase1_uv;
	// (0x24) HORZ_PH - horizontal phase
	uint16 horizontal_phase_rgb;
	uint16 horizontal_phase_uv;
	// (0x28) INIT_PHS - initial phase shift
	uint32 initial_vertical_phase0_shift_rgb0 : 4;
	uint32 initial_vertical_phase1_shift_rgb0 : 4;
	uint32 initial_horizontal_phase_shift_rgb0 : 4;
	uint32 initial_vertical_phase0_shift_uv : 4;
	uint32 initial_vertical_phase1_shift_uv : 4;
	uint32 initial_horizontal_phase_shift_uv : 4;
	uint32 _reserved0 : 8;
	// (0x2c) DWINPOS - destination window position
	uint16 window_left;
	uint16 window_top;
	// (0x30) DWINSZ - destination window size
	uint16 window_width;
	uint16 window_height;
	// (0x34) SWIDTH - source width
	uint16 source_width_rgb;
	uint16 source_width_uv;
	// (0x38) SWITDHSW - source width in 8 byte steps
	uint16 source_bytes_per_row_rgb;
	uint16 source_bytes_per_row_uv;
	uint16 source_height_rgb;
	uint16 source_height_uv;
	overlay_scale scale_rgb;
	overlay_scale scale_uv;
	// (0x48) OCLRC0 - overlay color correction 0
	uint32 brightness_correction : 8;		// signed, -128 to 127
	uint32 _reserved1 : 10;
	uint32 contrast_correction : 9;			// fixed point: 3.6 bits
	uint32 _reserved2 : 5;
	// (0x4c) OCLRC1 - overlay color correction 1
	uint32 saturation_cos_correction : 10;	// fixed point: 3.7 bits
	uint32 _reserved3 : 6;
	uint32 saturation_sin_correction : 11;	// signed fixed point: 3.7 bits
	uint32 _reserved4 : 5;
	// (0x50) DCLRKV - destination color key value
	uint32 color_key_blue : 8;
	uint32 color_key_green : 8;
	uint32 color_key_red : 8;
	uint32 _reserved5 : 8;
	// (0x54) DCLRKM - destination color key mask
	uint32 color_key_mask_blue : 8;
	uint32 color_key_mask_green : 8;
	uint32 color_key_mask_red : 8;
	uint32 _reserved6 : 7;
	uint32 color_key_enabled : 1;
	// (0x58) SCHRKVH - source chroma key high value
	uint32 source_chroma_key_high_red : 8;
	uint32 source_chroma_key_high_blue : 8;
	uint32 source_chroma_key_high_green : 8;
	uint32 _reserved7 : 8;
	// (0x5c) SCHRKVL - source chroma key low value
	uint32 source_chroma_key_low_red : 8;
	uint32 source_chroma_key_low_blue : 8;
	uint32 source_chroma_key_low_green : 8;
	uint32 _reserved8 : 8;
	// (0x60) SCHRKEN - source chroma key enable
	uint32 _reserved9 : 24;
	uint32 source_chroma_key_red_enabled : 1;
	uint32 source_chroma_key_blue_enabled : 1;
	uint32 source_chroma_key_green_enabled : 1;
	uint32 _reserved10 : 5;
	// (0x64) OCONFIG - overlay configuration
	uint32 _reserved11 : 3;
	uint32 color_control_output_mode : 1;
	uint32 yuv_to_rgb_bypass : 1;
	uint32 _reserved12 : 11;
	uint32 gamma2_enabled : 1;
	uint32 _reserved13 : 1;
	uint32 select_pipe : 1;
	uint32 slot_time : 8;
	uint32 _reserved14 : 5;
	// (0x68) OCOMD - overlay command
	uint32 overlay_enabled : 1;
	uint32 active_field : 1;
	uint32 active_buffer : 2;
	uint32 test_mode : 1;
	uint32 buffer_field_mode : 1;
	uint32 _reserved15 : 1;
	uint32 tv_flip_field_enabled : 1;
	uint32 _reserved16 : 1;
	uint32 tv_flip_field_parity : 1;
	uint32 source_format : 4;
	uint32 ycbcr422_order : 2;
	uint32 _reserved18 : 1;
	uint32 mirroring_mode : 2;
	uint32 _reserved19 : 13;

	uint32 _reserved20;

	uint32 start_0y;
	uint32 start_1y;
	uint32 start_0u;
	uint32 start_0v;
	uint32 start_1u;
	uint32 start_1v;
	uint32 _reserved21[6];
#if 0
	// (0x70) AWINPOS - alpha blend window position
	uint32 awinpos;
	// (0x74) AWINSZ - alpha blend window size
	uint32 awinsz;

	uint32 _reserved21[10];
#endif

	// (0xa0) FASTHSCALE - fast horizontal downscale (strangely enough,
	// the next two registers switch the usual Y/RGB vs. UV order)
	uint16 horizontal_scale_uv;
	uint16 horizontal_scale_rgb;
	// (0xa4) UVSCALEV - vertical downscale
	uint16 vertical_scale_uv;
	uint16 vertical_scale_rgb;

	uint32 _reserved22[86];

	// (0x200) polyphase filter coefficients
	uint16 vertical_coefficients_rgb[128];
	uint16 horizontal_coefficients_rgb[128];

	uint32	_reserved23[64];

	// (0x500)
	uint16 vertical_coefficients_uv[128];
	uint16 horizontal_coefficients_uv[128];
};


struct hardware_status {
	uint32	interrupt_status_register;
	uint32	_reserved0[3];
	void*	primary_ring_head_storage;
	uint32	_reserved1[3];
	void*	secondary_ring_0_head_storage;
	void*	secondary_ring_1_head_storage;
	uint32	_reserved2[2];
	void*	binning_head_storage;
	uint32	_reserved3[3];
	uint32	store[1008];
};

#endif	/* RADEON_HD_H */
