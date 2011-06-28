/*
	Haiku ATI video driver adapted from the X.org ATI driver which has the
	following copyright:

	Copyright 1992,1993,1994,1995,1996,1997 by Kevin E. Martin, Chapel Hill, North Carolina.

	Copyright 2009, 2011 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/

#ifndef __MACH64_H__
#define __MACH64_H__


#define CURSOR_BYTES	1024		// bytes used for cursor image in video memory

// Memory types for Mach64 chips.

enum Mach64_MemoryType
{
    MEM_NONE,
    MEM_DRAM,
    MEM_EDO,
    MEM_PSEUDO_EDO,
    MEM_SDRAM,
    MEM_SGRAM,
    MEM_SGRAM32,
    MEM_TYPE_7
};


// Macros to get/set a contiguous bit field.  Arguments should not be
// self-modifying.

#define UnitOf(___Value) (((((___Value) ^ ((___Value) - 1)) + 1) >> 1) \
						| ((((___Value) ^ ((___Value) - 1)) >> 1) + 1))

#define GetBits(__Value, _Mask) (((__Value) & (_Mask)) / UnitOf(_Mask))
#define SetBits(__Value, _Mask) (((__Value) * UnitOf(_Mask)) & (_Mask))


// Register MMIO addresses - expressed in BYTE offsets.
//-----------------------------------------------------

#define CRTC_H_TOTAL_DISP		(0x0c00 + 0x0000)	// Dword offset 00
#define CRTC_H_SYNC_STRT_WID	(0x0c00 + 0x0004)	// Dword offset 01
#define CRTC_V_TOTAL_DISP		(0x0c00 + 0x0008)	// Dword offset 02
#define CRTC_V_SYNC_STRT_WID	(0x0c00 + 0x000C)	// Dword offset 03
#define CRTC_VLINE_CRNT_VLINE	(0x0c00 + 0x0010)	// Dword offset 04
#define CRTC_OFF_PITCH			(0x0c00 + 0x0014)	// Dword offset 05
#define CRTC_INT_CNTL			(0x0c00 + 0x0018)	// Dword offset 06
#define CRTC_GEN_CNTL			(0x0c00 + 0x001C)	// Dword offset 07

#define DSP_CONFIG				(0x0c00 + 0x0020)	// Dword offset 08
#define DSP_ON_OFF				(0x0c00 + 0x0024)	// Dword offset 09

#define SHARED_CNTL 			(0x0c00 + 0x0038)	// Dword offset 0E

#define OVR_CLR 				(0x0c00 + 0x0040)	// Dword offset 10
#define OVR_WID_LEFT_RIGHT		(0x0c00 + 0x0044)	// Dword offset 11
#define OVR_WID_TOP_BOTTOM		(0x0c00 + 0x0048)	// Dword offset 12

#define CUR_CLR0				(0x0c00 + 0x0060)	// Dword offset 18
#define CUR_CLR1				(0x0c00 + 0x0064)	// Dword offset 19
#define CUR_OFFSET				(0x0c00 + 0x0068)	// Dword offset 1A
#define CUR_HORZ_VERT_POSN		(0x0c00 + 0x006C)	// Dword offset 1B
#define CUR_HORZ_VERT_OFF		(0x0c00 + 0x0070)	// Dword offset 1C

#define HW_DEBUG				(0x0c00 + 0x007C)	// Dword offset 1F

#define SCRATCH_REG0			(0x0c00 + 0x0080)	// Dword offset 20
#define SCRATCH_REG1			(0x0c00 + 0x0084)	// Dword offset 21

#define CLOCK_CNTL				(0x0c00 + 0x0090)	// Dword offset 24

#define BUS_CNTL				(0x0c00 + 0x00A0)	// Dword offset 28

#define LCD_INDEX				(0x0c00 + 0x00A4)	// Dword offset 29
#define LCD_DATA				(0x0c00 + 0x00A8)	// Dword offset 2A

#define MEM_CNTL				(0x0c00 + 0x00B0)	// Dword offset 2C

#define MEM_VGA_WP_SEL			(0x0c00 + 0x00B4)	// Dword offset 2D
#define MEM_VGA_RP_SEL			(0x0c00 + 0x00B8)	// Dword offset 2E

#define DAC_REGS				(0x0c00 + 0x00C0)	// Dword offset 30
#define DAC_W_INDEX				(DAC_REGS + 0)
#define DAC_DATA				(DAC_REGS + 1)
#define DAC_MASK				(DAC_REGS + 2)
#define DAC_R_INDEX				(DAC_REGS + 3)
#define DAC_CNTL				(0x0c00 + 0x00C4)	// Dword offset 31

#define GEN_TEST_CNTL			(0x0c00 + 0x00D0)	// Dword offset 34

#define CONFIG_CNTL				(0x0c00 + 0x00DC	// Dword offset 37 (CT, ET, VT)
#define CONFIG_CHIP_ID			(0x0c00 + 0x00E0)	// Dword offset 38
#define CONFIG_STAT0			(0x0c00 + 0x00E4)	// Dword offset 39
#define CONFIG_STAT1			(0x0c00 + 0x00E8)	// Dword offset 3A

#define DST_OFF_PITCH			(0x0c00 + 0x0100)	// Dword offset 40
#define DST_X					(0x0c00 + 0x0104)	// Dword offset 41
#define DST_Y					(0x0c00 + 0x0108)	// Dword offset 42
#define DST_Y_X 				(0x0c00 + 0x010C)	// Dword offset 43
#define DST_WIDTH				(0x0c00 + 0x0110)	// Dword offset 44
#define DST_HEIGHT				(0x0c00 + 0x0114)	// Dword offset 45
#define DST_HEIGHT_WIDTH		(0x0c00 + 0x0118)	// Dword offset 46
#define DST_X_WIDTH 			(0x0c00 + 0x011C)	// Dword offset 47
#define DST_BRES_LNTH			(0x0c00 + 0x0120)	// Dword offset 48
#define DST_BRES_ERR			(0x0c00 + 0x0124)	// Dword offset 49
#define DST_BRES_INC			(0x0c00 + 0x0128)	// Dword offset 4A
#define DST_BRES_DEC			(0x0c00 + 0x012C)	// Dword offset 4B
#define DST_CNTL				(0x0c00 + 0x0130)	// Dword offset 4C

#define SRC_OFF_PITCH			(0x0c00 + 0x0180)	// Dword offset 60
#define SRC_X					(0x0c00 + 0x0184)	// Dword offset 61
#define SRC_Y					(0x0c00 + 0x0188)	// Dword offset 62
#define SRC_Y_X 				(0x0c00 + 0x018C)	// Dword offset 63
#define SRC_WIDTH1				(0x0c00 + 0x0190)	// Dword offset 64
#define SRC_HEIGHT1 			(0x0c00 + 0x0194)	// Dword offset 65
#define SRC_HEIGHT1_WIDTH1		(0x0c00 + 0x0198)	// Dword offset 66
#define SRC_X_START 			(0x0c00 + 0x019C)	// Dword offset 67
#define SRC_Y_START 			(0x0c00 + 0x01A0)	// Dword offset 68
#define SRC_Y_X_START			(0x0c00 + 0x01A4)	// Dword offset 69
#define SRC_WIDTH2				(0x0c00 + 0x01A8)	// Dword offset 6A
#define SRC_HEIGHT2 			(0x0c00 + 0x01AC)	// Dword offset 6B
#define SRC_HEIGHT2_WIDTH2		(0x0c00 + 0x01B0)	// Dword offset 6C
#define SRC_CNTL				(0x0c00 + 0x01B4)	// Dword offset 6D

#define HOST_DATA0				(0x0c00 + 0x0200)	// Dword offset 80
#define HOST_DATA1				(0x0c00 + 0x0204)	// Dword offset 81
#define HOST_DATA2				(0x0c00 + 0x0208)	// Dword offset 82
#define HOST_DATA3				(0x0c00 + 0x020C)	// Dword offset 83
#define HOST_DATA4				(0x0c00 + 0x0210)	// Dword offset 84
#define HOST_DATA5				(0x0c00 + 0x0214)	// Dword offset 85
#define HOST_DATA6				(0x0c00 + 0x0218)	// Dword offset 86
#define HOST_DATA7				(0x0c00 + 0x021C)	// Dword offset 87
#define HOST_DATA8				(0x0c00 + 0x0220)	// Dword offset 88
#define HOST_DATA9				(0x0c00 + 0x0224)	// Dword offset 89
#define HOST_DATAA				(0x0c00 + 0x0228)	// Dword offset 8A
#define HOST_DATAB				(0x0c00 + 0x022C)	// Dword offset 8B
#define HOST_DATAC				(0x0c00 + 0x0230)	// Dword offset 8C
#define HOST_DATAD				(0x0c00 + 0x0234)	// Dword offset 8D
#define HOST_DATAE				(0x0c00 + 0x0238)	// Dword offset 8E
#define HOST_DATAF				(0x0c00 + 0x023C)	// Dword offset 8F
#define HOST_CNTL				(0x0c00 + 0x0240)	// Dword offset 90

#define PAT_REG0				(0x0c00 + 0x0280)	// Dword offset A0
#define PAT_REG1				(0x0c00 + 0x0284)	// Dword offset A1
#define PAT_CNTL				(0x0c00 + 0x0288)	// Dword offset A2

#define SC_LEFT 				(0x0c00 + 0x02A0)	// Dword offset A8
#define SC_RIGHT				(0x0c00 + 0x02A4)	// Dword offset A9
#define SC_LEFT_RIGHT			(0x0c00 + 0x02A8)	// Dword offset AA
#define SC_TOP					(0x0c00 + 0x02AC)	// Dword offset AB
#define SC_BOTTOM				(0x0c00 + 0x02B0)	// Dword offset AC
#define SC_TOP_BOTTOM			(0x0c00 + 0x02B4)	// Dword offset AD

#define DP_BKGD_CLR 			(0x0c00 + 0x02C0)	// Dword offset B0
#define DP_FRGD_CLR 			(0x0c00 + 0x02C4)	// Dword offset B1
#define DP_WRITE_MASK			(0x0c00 + 0x02C8)	// Dword offset B2
#define DP_CHAIN_MASK			(0x0c00 + 0x02CC)	// Dword offset B3
#define DP_PIX_WIDTH			(0x0c00 + 0x02D0)	// Dword offset B4
#define DP_MIX					(0x0c00 + 0x02D4)	// Dword offset B5
#define DP_SRC					(0x0c00 + 0x02D8)	// Dword offset B6

#define CLR_CMP_CLR 			(0x0c00 + 0x0300)	// Dword offset C0
#define CLR_CMP_MASK			(0x0c00 + 0x0304)	// Dword offset C1
#define CLR_CMP_CNTL			(0x0c00 + 0x0308)	// Dword offset C2

#define FIFO_STAT				(0x0c00 + 0x0310)	// Dword offset C4

#define CONTEXT_MASK			(0x0c00 + 0x0320)	// Dword offset C8
#define CONTEXT_LOAD_CNTL		(0x0c00 + 0x032C)	// Dword offset CB

#define GUI_TRAJ_CNTL			(0x0c00 + 0x0330)	// Dword offset CC
#define GUI_STAT				(0x0c00 + 0x0338)	// Dword offset CE


// Definitions used for overlays.
//===============================

#define REG_BLOCK_1	0x0800		// offset of register block 1 in register area

#define OVERLAY_Y_X_START		(REG_BLOCK_1 + 0x0000)	// Dword offset 00
#define OVERLAY_LOCK_START		0x80000000ul
#define OVERLAY_Y_X_END			(REG_BLOCK_1 + 0x0004)	// Dword offset 01
#define OVERLAY_GRAPHICS_KEY_CLR (REG_BLOCK_1 + 0x0010)	// Dword offset 04
#define OVERLAY_GRAPHICS_KEY_MSK (REG_BLOCK_1 + 0x0014)	// Dword offset 05
#define OVERLAY_KEY_CNTL		(REG_BLOCK_1 + 0x0018)	// Dword offset 06
#define OVERLAY_MIX_FALSE		0x00
#define OVERLAY_MIX_EQUAL		0x50
#define OVERLAY_SCALE_INC		(REG_BLOCK_1 + 0x0020)	// Dword offset 08
#define OVERLAY_SCALE_CNTL		(REG_BLOCK_1 + 0x0024)	// Dword offset 09
#define SCALE_PIX_EXPAND		0x00000001
#define OVERLAY_EN				0x40000000
#define SCALE_EN				0x80000000
#define SCALER_HEIGHT_WIDTH		(REG_BLOCK_1 + 0x0028)	// Dword offset 0a
#define SCALER_BUF0_OFFSET		(REG_BLOCK_1 + 0x0034)	// Dword offset 0d
#define SCALER_BUF0_PITCH		(REG_BLOCK_1 + 0x003c)	// Dword offset 0f
#define VIDEO_FORMAT			(REG_BLOCK_1 + 0x0048)	// Dword offset 12
#define SCALE_IN_VYUY422		0x000b0000
#define BUF0_OFFSET				(REG_BLOCK_1 + 0x0080)	// Dword offset 20
#define BUF0_PITCH				(REG_BLOCK_1 + 0x008c)	// Dword offset 23
#define SCALER_COLOUR_CNTL		(REG_BLOCK_1 + 0x0150)	// Dword offset 54
#define SCALER_H_COEFF0			(REG_BLOCK_1 + 0x0154)	// Dword offset 55
#define SCALER_H_COEFF1			(REG_BLOCK_1 + 0x0158)	// Dword offset 56
#define SCALER_H_COEFF2			(REG_BLOCK_1 + 0x015c)	// Dword offset 57
#define SCALER_H_COEFF3			(REG_BLOCK_1 + 0x0160)	// Dword offset 58
#define SCALER_H_COEFF4			(REG_BLOCK_1 + 0x0164)	// Dword offset 59


// CRTC control values.

#define CRTC_H_SYNC_NEG			0x00200000
#define CRTC_V_SYNC_NEG			0x00200000

#define CRTC_DBL_SCAN_EN		0x00000001
#define CRTC_INTERLACE_EN		0x00000002
#define CRTC_HSYNC_DIS			0x00000004
#define CRTC_VSYNC_DIS			0x00000008
#define CRTC_CSYNC_EN			0x00000010
#define CRTC_PIX_BY_2_EN		0x00000020
#define CRTC_DISPLAY_DIS		0x00000040
#define CRTC_VGA_XOVERSCAN		0x00000080

#define CRTC_PIX_WIDTH			0x00000700
#define CRTC_PIX_WIDTH_4BPP		0x00000100
#define CRTC_PIX_WIDTH_8BPP		0x00000200
#define CRTC_PIX_WIDTH_15BPP	0x00000300
#define CRTC_PIX_WIDTH_16BPP	0x00000400
#define CRTC_PIX_WIDTH_24BPP	0x00000500
#define CRTC_PIX_WIDTH_32BPP	0x00000600

#define CRTC_BYTE_PIX_ORDER		0x00000800
#define CRTC_PIX_ORDER_MSN_LSN	0x00000000
#define CRTC_PIX_ORDER_LSN_MSN	0x00000800

#define CRTC_FIFO_LWM			0x000f0000
#define CRTC2_PIX_WIDTH			0x000e0000
#define CRTC_VGA_128KAP_PAGING	0x00100000
#define CRTC_VFC_SYNC_TRISTATE	0x00200000
#define CRTC2_EN				0x00200000
#define CRTC_LOCK_REGS			0x00400000
#define CRTC_SYNC_TRISTATE		0x00800000
#define CRTC_EXT_DISP_EN		0x01000000
#define CRTC_EN					0x02000000
#define CRTC_DISP_REQ_EN		0x04000000
#define CRTC_VGA_LINEAR			0x08000000
#define CRTC_VGA_TEXT_132		0x20000000
#define CRTC_CNT_EN				0x40000000
#define CRTC_CUR_B_TEST			0x80000000

#define CRTC_CRNT_VLINE			0x07f00000
#define CRTC_VBLANK				0x00000001

#define CRTC_PITCH				0xffc00000

// DAC control values.
#define DAC_8BIT_EN				0x00000100


// Mix control values.
#define MIX_NOT_DST				0x0000
#define MIX_0 					0x0001
#define MIX_1 					0x0002
#define MIX_DST					0x0003
#define MIX_NOT_SRC				0x0004
#define MIX_XOR					0x0005
#define MIX_XNOR				0x0006
#define MIX_SRC					0x0007
#define MIX_NAND				0x0008
#define MIX_NOT_SRC_OR_DST		0x0009
#define MIX_SRC_OR_NOT_DST		0x000a
#define MIX_OR					0x000b
#define MIX_AND					0x000c
#define MIX_SRC_AND_NOT_DST		0x000d
#define MIX_NOT_SRC_AND_DST		0x000e
#define MIX_NOR					0x000f

// BUS_CNTL register constants.
#define BUS_FIFO_ERR_ACK		0x00200000
#define BUS_HOST_ERR_ACK		0x00800000
#define BUS_EXT_REG_EN			0x08000000

// GEN_TEST_CNTL register constants.
#define GEN_OVR_OUTPUT_EN		0x20
#define HWCURSOR_ENABLE 		0x80
#define GUI_ENGINE_ENABLE		0x100

// DSP_CONFIG register constants.
#define DSP_XCLKS_PER_QW		0x00003fff
#define DSP_LOOP_LATENCY		0x000f0000
#define DSP_PRECISION			0x00700000

// DSP_ON_OFF register constants.
#define DSP_OFF 				0x000007ff
#define DSP_ON					0x07ff0000

// CLOCK_CNTL register constants.
#define CLOCK_SEL		0x0f
#define CLOCK_DIV		0x30
#define CLOCK_DIV1		0x00
#define CLOCK_DIV2		0x10
#define CLOCK_DIV4		0x20
#define CLOCK_STROBE	0x40
#define PLL_WR_EN		0x02
#define PLL_ADDR		0xfc

// PLL registers.
#define PLL_MACRO_CNTL		0x01
#define PLL_REF_DIV			0x02
#define PLL_GEN_CNTL		0x03
#define PLL_MCLK_FB_DIV		0x04
#define PLL_VCLK_CNTL		0x05
#define PLL_VCLK_POST_DIV	0x06
#define PLL_VCLK0_FB_DIV	0x07
#define PLL_VCLK1_FB_DIV	0x08
#define PLL_VCLK2_FB_DIV	0x09
#define PLL_VCLK3_FB_DIV	0x0A
#define PLL_XCLK_CNTL		0x0B
#define PLL_TEST_CTRL		0x0E
#define PLL_TEST_COUNT		0x0F

// Fields in PLL registers.
#define PLL_PC_GAIN			0x07
#define PLL_VC_GAIN			0x18
#define PLL_DUTY_CYC		0xE0
#define PLL_MFB_TIMES_4_2B	0x08
#define PLL_VCLK0_XDIV		0x10
#define PLL_OVERRIDE		0x01
#define PLL_MCLK_RST		0x02
#define OSC_EN				0x04
#define EXT_CLK_EN			0x08
#define MCLK_SRC_SEL		0x70
#define EXT_CLK_CNTL		0x80
#define PLL_VCLK_SRC_SEL	0x03
#define PLL_VCLK_RESET		0x04
#define VCLK_INVERT			0x08
#define VCLK0_POST			0x03
#define VCLK1_POST			0x0C
#define VCLK2_POST			0x30
#define VCLK3_POST			0xC0

// MEM_CNTL register constants.
#define CTL_MEM_TRP			0x00000300
#define CTL_MEM_TRCD		0x00000C00
#define CTL_MEM_TCRD		0x00001000
#define CTL_MEM_TRAS		0x00070000

// DST_CNTL register constants.
#define DST_X_LEFT_TO_RIGHT		1
#define DST_Y_TOP_TO_BOTTOM		2
#define DST_LAST_PEL				0x20

// SRC_CNTL register constants.
#define SRC_LINE_X_LEFT_TO_RIGHT	0x10

// HOST_CNTL register constants.
#define HOST_BYTE_ALIGN			1

// DP_CHAIN_MASK register constants.
#define DP_CHAIN_4BPP			0x8888
#define DP_CHAIN_7BPP			0xD2D2
#define DP_CHAIN_8BPP			0x8080
#define DP_CHAIN_8BPP_RGB		0x9292
#define DP_CHAIN_15BPP		0x4210
#define DP_CHAIN_16BPP		0x8410
#define DP_CHAIN_24BPP		0x8080
#define DP_CHAIN_32BPP		0x8080

// DP_PIX_WIDTH register constants.
#define DST_1BPP			0
#define DST_4BPP			1
#define DST_8BPP			2
#define DST_15BPP 			3
#define DST_16BPP 			4
#define DST_32BPP 			6
#define SRC_1BPP			0
#define SRC_4BPP			0x100
#define SRC_8BPP			0x200
#define SRC_15BPP 			0x300
#define SRC_16BPP 			0x400
#define SRC_32BPP 			0x600
#define HOST_1BPP 			0
#define HOST_4BPP 			0x10000
#define HOST_8BPP 			0x20000
#define HOST_15BPP			0x30000
#define HOST_16BPP			0x40000
#define HOST_32BPP			0x60000
#define BYTE_ORDER_MSB_TO_LSB 0
#define BYTE_ORDER_LSB_TO_MSB 0x1000000

// DP_SRC register constants.
#define BKGD_SRC_BKGD_CLR 	0
#define FRGD_SRC_FRGD_CLR 	0x100
#define FRGD_SRC_BLIT 		0x300
#define MONO_SRC_ONE		0

// GUI_STAT register constants.
#define ENGINE_BUSY 		1

// LCD Index register constants.
#define LCD_REG_INDEX		0x0000003F
#define LCD_DISPLAY_DIS		0x00000100
#define LCD_SRC_SEL			0x00000200
#define CRTC2_DISPLAY_DIS	0x00000400

// LCD register indices.
#define LCD_CONFIG_PANEL		0x00
#define LCD_GEN_CNTL			0x01
#define LCD_DSTN_CONTROL		0x02
#define LCD_HFB_PITCH_ADDR		0x03
#define LCD_HORZ_STRETCHING		0x04
#define LCD_VERT_STRETCHING		0x05
#define LCD_EXT_VERT_STRETCH	0x06
#define LCD_LT_GIO				0x07
#define LCD_POWER_MANAGEMENT	0x08

// LCD_CONFIG_PANEL register constants.
#define DONT_SHADOW_HEND	0x00004000

// LCD_GEN_CNTL register constants.
#define CRT_ON				0x00000001
#define LCD_ON				0x00000002
#define HORZ_DIVBY2_EN		0x00000004
#define LOCK_8DOT			0x00000010
#define DONT_SHADOW_VPAR	0x00000040
#define DIS_HOR_CRT_DIVBY2	0x00000400
#define MCLK_PM_EN			0x00010000
#define VCLK_DAC_PM_EN		0x00020000
#define CRTC_RW_SELECT		0x08000000
#define USE_SHADOWED_VEND	0x10000000
#define USE_SHADOWED_ROWCUR	0x20000000
#define SHADOW_EN			0x40000000
#define SHADOW_RW_EN		0x80000000

// LCD_HORZ_STRETCHING register constants.
#define HORZ_STRETCH_BLEND	0x00000fff
#define HORZ_STRETCH_RATIO	0x0000ffff
#define HORZ_STRETCH_LOOP	0x00070000
#define HORZ_PANEL_SIZE		0x0ff00000
#define AUTO_HORZ_RATIO		0x20000000
#define HORZ_STRETCH_MODE	0x40000000
#define HORZ_STRETCH_EN		0x80000000

// LCD_VERT_STRETCHING register constants.
#define VERT_STRETCH_RATIO0	0x000003ff
#define VERT_STRETCH_RATIO1	0x000ffc00
#define VERT_STRETCH_RATIO2	0x3ff00000
#define VERT_STRETCH_USE0	0x40000000
#define VERT_STRETCH_EN		0x80000000

// LCD_EXT_VERT_STRETCH register constants.
#define VERT_STRETCH_RATIO3	0x000003ff
#define FORCE_DAC_DATA		0x000000ff
#define FORCE_DAC_DATA_SEL	0x00000300
#define VERT_STRETCH_MODE	0x00000400
#define VERT_PANEL_SIZE		0x003ff800
#define AUTO_VERT_RATIO		0x00400000
#define USE_AUTO_FP_POS		0x00800000
#define USE_AUTO_LCD_VSYNC	0x01000000

// LCD_POWER_MANAGEMENT register constants.
#define AUTO_POWER_UP		0x00000008
#define POWER_BLON			0x02000000
#define STANDBY_NOW			0x10000000
#define SUSPEND_NOW			0x20000000



// Functions to get/set PLL registers.
//=======================================

static inline uint8
Mach64_GetPLLReg(uint8 index)
{
	OUTREG8(CLOCK_CNTL + 1, (index << 2) & PLL_ADDR);
	return INREG8(CLOCK_CNTL + 2);
}


static inline void
Mach64_SetPLLReg(uint8 index, uint8 value)
{
	OUTREG8(CLOCK_CNTL + 1, ((index << 2) & PLL_ADDR) | PLL_WR_EN);
	OUTREG8(CLOCK_CNTL + 2, value);
}


static inline uint32
Mach64_GetLCDReg(int index)
{
	OUTREG8(LCD_INDEX, index & LCD_REG_INDEX);
	return INREG(LCD_DATA);
}


static inline void
Mach64_PutLCDReg(int index, uint32 value)
{
	OUTREG8(LCD_INDEX, index & LCD_REG_INDEX);
	OUTREG(LCD_DATA, value);
}


#endif	// __MACH64_H__
