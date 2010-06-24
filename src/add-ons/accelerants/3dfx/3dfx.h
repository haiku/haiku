/*
	Copyright 2010 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2010
*/

#ifndef __3DFX_H__
#define __3DFX_H__



#define BIT(n)	(1UL<<(n))

#define ROP_COPY	0xcc
#define ROP_INVERT	0x55
#define ROP_XOR		0x66

#define MEM_TYPE_SGRAM	0
#define MEM_TYPE_SDRAM	1


// Base register offsets in MMIO area.
#define STATUS				0x0
#define MISC_INIT1			0x14
#define DRAM_INIT0			0x18
#define DRAM_INIT1			0x1C
#define VGA_INIT0			0x28
#define VGA_INIT1			0x2c
#define PLL_CTRL0			0x40
#define DAC_MODE			0x4c
#define DAC_ADDR			0x50
#define DAC_DATA			0x54
#define RGB_MAX_DELTA		0x58
#define VIDEO_PROC_CONFIG	0x5c
#define HW_CURSOR_PAT_ADDR	0x60
#define HW_CURSOR_LOC		0x64
#define HW_CURSOR_COLOR0	0x68
#define HW_CURSOR_COLOR1	0x6c
#define VIDEO_SERIAL_PARALLEL_PORT	0x78
#define VIDEO_CHROMA_MIN	0x8c
#define VIDEO_CHROMA_MAX	0x90
#define VIDEO_SCREEN_SIZE	0x98
#define VIDEO_OVERLAY_START_COORDS	0x9c
#define VIDEO_OVERLAY_END_COORDS	0xa0
#define VIDEO_OVERLAY_DUDX			0xa4
#define VIDEO_OVERLAY_DUDX_OFFSET_SRC_WIDTH 0xa8
#define VIDEO_OVERLAY_DVDY			0xac
#define VIDEO_OVERLAY_DVDY_OFFSET	0xe0
#define VIDEO_DESKTOP_START_ADDR	0xe4
#define VIDEO_DESKTOP_OVERLAY_STRIDE 0xe8
#define VIDEO_IN_ADDR0				0xec

// Offset in MMIO area of registers used for drawing.
#define CLIP0_MIN		(0x100000 + 0x8)
#define CLIP0_MAX		(0x100000 + 0xC)
#define DST_BASE_ADDR	(0x100000 + 0x10)
#define DST_FORMAT		(0x100000 + 0x14)
#define SRC_BASE_ADDR	(0x100000 + 0x34)
#define CLIP1_MIN		(0x100000 + 0x4C)
#define CLIP1_MAX		(0x100000 + 0x50)
#define SRC_FORMAT		(0x100000 + 0x54)
#define SRC_XY			(0x100000 + 0x5C)
#define COLOR_BACK		(0x100000 + 0x60)
#define COLOR_FORE		(0x100000 + 0x64)
#define DST_SIZE		(0x100000 + 0x68)
#define DST_XY			(0x100000 + 0x6C)
#define CMD_2D			(0x100000 + 0x70)

// Offset in MMIO area of 3D registers.
#define CMD_3D 			(0x200000 + 0x120)

// Flags and register values.
#define CMD_2D_GO				BIT(8)
#define CMD_3D_NOP				0
#define SGRAM_TYPE				BIT(27)
#define SGRAM_NUM_CHIPSETS		BIT(26)
#define DISABLE_2D_BLOCK_WRITE	BIT(15)
#define MCTL_TYPE_SDRAM 		BIT(30)
#define DAC_MODE_2X				BIT(0)
#define VIDEO_2X_MODE_ENABLE	BIT(26)
#define CLUT_SELECT_8BIT		BIT(2)
#define VGA0_EXTENSIONS 		BIT(6)
#define WAKEUP_3C3				BIT(8)
#define VGA0_LEGACY_DECODE		BIT(9)
#define ENABLE_ALT_READBACK 	BIT(10)
#define EXT_SHIFT_OUT			BIT(12)
#define VIDEO_PROCESSOR_ENABLE	BIT(0)
#define DESKTOP_ENABLE			BIT(7)
#define DESKTOP_PIXEL_FORMAT_SHIFT	18
#define DESKTOP_CLUT_BYPASS 	BIT(10)
#define OVERLAY_CLUT_BYPASS		BIT(11)
#define CURSOR_ENABLE			BIT(27)
#define STATUS_BUSY				BIT(9)
#define X_RIGHT_TO_LEFT			BIT(14)
#define Y_BOTTOM_TO_TOP			BIT(15)

// 2D Commands
#define SCRN_TO_SCRN_BLIT	1
#define RECTANGLE_FILL		5

// Definitions for I2C bus when fetching  EDID info.
#define VSP_SDA0_IN		0x00400000
#define VSP_SCL0_IN		0x00200000
#define VSP_SDA0_OUT	0x00100000
#define VSP_SCL0_OUT	0x00080000
#define VSP_ENABLE_IIC0	0x00040000	// set bit to 1 to enable I2C bus 0

// Definitions for overlays.
#define VIDEO_PROC_CONFIG_MASK	0xa2e3eb6c
#define VIDCFG_OVL_FMT_RGB565	(1 << 21)
#define VIDCFG_OVL_FMT_YUYV422	(5 << 21)


#endif	// __3DFX_H__
