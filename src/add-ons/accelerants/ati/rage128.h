/*
	Haiku ATI video driver adapted from the X.org ATI driver which has the
	following copyright:

	Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
						  Precision Insight, Inc., Cedar Park, Texas, and
						  VA Linux Systems Inc., Fremont, California.

	Copyright 2009, 2011 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/


#ifndef __RAGE128_H__
#define __RAGE128_H__


#define CURSOR_BYTES	1024	// bytes used for cursor image in video memory

#define R128_TIMEOUT	2000000	// Fall out of wait loops after this count


// Register MMIO addresses.
//-------------------------

#define R128_AUX_SC_CNTL				  0x1660

#define R128_BUS_CNTL					  0x0030
#		define R128_BUS_MASTER_DIS		  (1 << 6)
#		define R128_BUS_RD_DISCARD_EN	  (1 << 24)
#		define R128_BUS_RD_ABORT_EN 	  (1 << 25)
#		define R128_BUS_MSTR_DISCONNECT_EN (1 << 28)
#		define R128_BUS_WRT_BURST		  (1 << 29)
#		define R128_BUS_READ_BURST		  (1 << 30)

#define R128_CAP0_TRIG_CNTL 			  0x0950	// ?
#define R128_CAP1_TRIG_CNTL 			  0x09c0	// ?
#define R128_CLOCK_CNTL_DATA			  0x000c
#define R128_CLOCK_CNTL_INDEX			  0x0008
#		define R128_PLL_WR_EN			  (1 << 7)
#		define R128_PLL_DIV_SEL 		  (3 << 8)
#define R128_CONFIG_MEMSIZE 			  0x00f8
#define R128_CRTC_EXT_CNTL				  0x0054
#		define R128_CRTC_VGA_XOVERSCAN	  (1 <<  0)
#		define R128_VGA_ATI_LINEAR		  (1 <<  3)
#		define R128_XCRT_CNT_EN 		  (1 <<  6)
#		define R128_CRTC_HSYNC_DIS		  (1 <<  8)
#		define R128_CRTC_VSYNC_DIS		  (1 <<  9)
#		define R128_CRTC_DISPLAY_DIS	  (1 << 10)
#		define R128_CRTC_CRT_ON 		  (1 << 15)
#		define R128_FP_OUT_EN			  (1 << 22)
#		define R128_FP_ACTIVE			  (1 << 23)
#define R128_CRTC_GEN_CNTL				  0x0050
#		define R128_CRTC_CUR_EN 		  (1 << 16)
#		define R128_CRTC_EXT_DISP_EN	  (1 << 24)
#		define R128_CRTC_EN 			  (1 << 25)
#define R128_CRTC_H_SYNC_STRT_WID		  0x0204
#		define R128_CRTC_H_SYNC_POL		  (1 << 23)
#define R128_CRTC_H_TOTAL_DISP			  0x0200
#define R128_CRTC_OFFSET				  0x0224
#define R128_CRTC_OFFSET_CNTL			  0x0228
#define R128_CRTC_PITCH					  0x022c
#define R128_CRTC_V_SYNC_STRT_WID		  0x020c
#		define R128_CRTC_V_SYNC_POL		  (1 << 23)
#define R128_CRTC_V_TOTAL_DISP			  0x0208
#define R128_CUR_CLR0					  0x026c
#define R128_CUR_CLR1					  0x0270
#define R128_CUR_HORZ_VERT_OFF			  0x0268
#define R128_CUR_HORZ_VERT_POSN 		  0x0264
#define R128_CUR_OFFSET 				  0x0260
#		define R128_CUR_LOCK			  (1 << 31)

#define R128_DAC_CNTL					  0x0058
#		define R128_DAC_RANGE_CNTL		  (3 <<  0)
#		define R128_DAC_BLANKING		  (1 <<  2)
#		define R128_DAC_CRT_SEL_CRTC2	  (1 <<  4)
#		define R128_DAC_PALETTE_ACC_CTL   (1 <<  5)
#		define R128_DAC_8BIT_EN 		  (1 <<  8)
#		define R128_DAC_VGA_ADR_EN		  (1 << 13)
#		define R128_DAC_MASK_ALL		  (0xff << 24)
#define R128_DDA_CONFIG 				  0x02e0
#define R128_DDA_ON_OFF 				  0x02e4
#define R128_DEFAULT_OFFSET 			  0x16e0
#define R128_DEFAULT_PITCH				  0x16e4
#define R128_DEFAULT_SC_BOTTOM_RIGHT	  0x16e8
#		define R128_DEFAULT_SC_RIGHT_MAX  (0x1fff <<  0)
#		define R128_DEFAULT_SC_BOTTOM_MAX (0x1fff << 16)
#define R128_DP_BRUSH_BKGD_CLR			  0x1478
#define R128_DP_BRUSH_FRGD_CLR			  0x147c
#define R128_DP_CNTL					  0x16c0
#		define R128_DST_X_LEFT_TO_RIGHT   (1 <<  0)
#		define R128_DST_Y_TOP_TO_BOTTOM   (1 <<  1)
#define R128_DP_DATATYPE				  0x16c4
#		define R128_HOST_BIG_ENDIAN_EN	  (1 << 29)
#define R128_DP_GUI_MASTER_CNTL 		  0x146c
#		define R128_DP_SRC_SOURCE_MEMORY	(2  << 24)
#		define R128_GMC_AUX_CLIP_DIS		(1  << 29)
#		define R128_GMC_BRUSH_NONE			(15 <<  4)
#		define R128_GMC_BRUSH_SOLID_COLOR	(13 <<  4)
#		define R128_GMC_CLR_CMP_CNTL_DIS	(1  << 28)
#		define R128_GMC_DST_DATATYPE_SHIFT	8
#		define R128_GMC_SRC_DATATYPE_COLOR	(3 << 12)
#		define R128_ROP3_Dn 			  0x00550000
#		define R128_ROP3_P				  0x00f00000
#		define R128_ROP3_S				  0x00cc0000
#define R128_DP_SRC_BKGD_CLR			  0x15dc
#define R128_DP_SRC_FRGD_CLR			  0x15d8
#define R128_DP_WRITE_MASK				  0x16cc
#define R128_DST_BRES_DEC				  0x1630
#define R128_DST_BRES_ERR				  0x1628
#define R128_DST_BRES_INC				  0x162c
#define R128_DST_HEIGHT_WIDTH			  0x143c
#define R128_DST_WIDTH_HEIGHT			  0x1598
#define R128_DST_Y_X					  0x1438

#define R128_FP_CRTC_H_TOTAL_DISP         0x0250
#define R128_FP_CRTC_V_TOTAL_DISP         0x0254
#define R128_FP_GEN_CNTL				  0x0284
#		define R128_FP_FPON 				 (1 << 0)
#		define R128_FP_BLANK_DIS			 (1 << 1)
#		define R128_FP_TDMS_EN				 (1 <<	2)
#		define R128_FP_DETECT_SENSE 		 (1 <<	8)
#		define R128_FP_SEL_CRTC2			 (1 << 13)
#		define R128_FP_CRTC_DONT_SHADOW_VPAR (1 << 16)
#		define R128_FP_CRTC_DONT_SHADOW_HEND (1 << 17)
#		define R128_FP_CRTC_USE_SHADOW_VEND  (1 << 18)
#		define R128_FP_CRTC_USE_SHADOW_ROWCUR (1 << 19)
#		define R128_FP_CRTC_HORZ_DIV2_EN	 (1 << 20)
#		define R128_FP_CRTC_HOR_CRT_DIV2_DIS (1 << 21)
#		define R128_FP_CRT_SYNC_SEL 		 (1 << 23)
#		define R128_FP_USE_SHADOW_EN		 (1 << 24)
#define R128_FP_H_SYNC_STRT_WID           0x02c4
#define R128_FP_HORZ_STRETCH              0x028c
#       define R128_HORZ_STRETCH_RATIO_MASK  0xfff
#       define R128_HORZ_STRETCH_RATIO_SHIFT 0
#       define R128_HORZ_STRETCH_RATIO_MAX   4096
#       define R128_HORZ_PANEL_SIZE          (0xff << 16)
#       define R128_HORZ_PANEL_SHIFT         16
#       define R128_AUTO_HORZ_RATIO          (0 << 24)
#       define R128_HORZ_STRETCH_PIXREP      (0 << 25)
#       define R128_HORZ_STRETCH_BLEND       (1 << 25)
#       define R128_HORZ_STRETCH_ENABLE      (1 << 26)
#       define R128_HORZ_FP_LOOP_STRETCH     (0x7 << 27)
#       define R128_HORZ_STRETCH_RESERVED    (1 << 30)
#       define R128_HORZ_AUTO_RATIO_FIX_EN   (1 << 31)

#define R128_FP_PANEL_CNTL                0x0288
#       define R128_FP_DIGON              (1 << 0)
#       define R128_FP_BLON               (1 << 1)
#define R128_FP_V_SYNC_STRT_WID           0x02c8
#define R128_FP_VERT_STRETCH              0x0290
#       define R128_VERT_PANEL_SIZE          (0x7ff <<  0)
#       define R128_VERT_PANEL_SHIFT         0
#       define R128_VERT_STRETCH_RATIO_MASK  0x3ff
#       define R128_VERT_STRETCH_RATIO_SHIFT 11
#       define R128_VERT_STRETCH_RATIO_MAX   1024
#       define R128_VERT_STRETCH_ENABLE      (1 << 24)
#       define R128_VERT_STRETCH_LINEREP     (0 << 25)
#       define R128_VERT_STRETCH_BLEND       (1 << 25)
#       define R128_VERT_AUTO_RATIO_EN       (1 << 26)
#       define R128_VERT_STRETCH_RESERVED    0xf8e00000

#define R128_GEN_INT_CNTL				  0x0040
#define R128_GEN_RESET_CNTL 			  0x00f0
#		define R128_SOFT_RESET_GUI		  (1 <<  0)
#define R128_GPIO_MONID 				  0x0068
#		define R128_GPIO_MONID_A_0		  (1 <<  0)
#		define R128_GPIO_MONID_A_3		  (1 <<  3)
#		define R128_GPIO_MONID_Y_0		  (1 <<  8)
#		define R128_GPIO_MONID_Y_3		  (1 << 11)
#		define R128_GPIO_MONID_EN_0 	  (1 << 16)
#		define R128_GPIO_MONID_EN_3 	  (1 << 19)
#		define R128_GPIO_MONID_MASK_0	  (1 << 24)
#		define R128_GPIO_MONID_MASK_3	  (1 << 27)
#define R128_GUI_PROBE					  0x16bc
#define R128_GUI_STAT					  0x1740
#		define R128_GUI_FIFOCNT_MASK	  0x0fff
#		define R128_GUI_ACTIVE			  (1 << 31)

#define R128_HTOTAL_CNTL				  0x0009	// PLL

#define R128_I2C_CNTL_1 				  0x0094	// ?

#define R128_LVDS_GEN_CNTL				  0x02d0
#		define R128_LVDS_ON 			  (1   <<  0)
#		define R128_LVDS_DISPLAY_DIS	  (1   <<  1)
#		define R128_LVDS_EN 			  (1   <<  7)
#		define R128_LVDS_DIGON			  (1   << 18)
#		define R128_LVDS_BLON			  (1   << 19)
#		define R128_LVDS_SEL_CRTC2		  (1   << 23)
#		define R128_HSYNC_DELAY_SHIFT	  28
#		define R128_HSYNC_DELAY_MASK	  (0xf << 28)

#define R128_MCLK_CNTL					  0x000f /* PLL */
#		define R128_FORCE_GCP			  (1 << 16)
#		define R128_FORCE_PIPE3D_CP 	  (1 << 17)
#define R128_MEM_CNTL					  0x0140
#define R128_MPP_TB_CONFIG				  0x01c0	// ?
#define R128_MPP_GP_CONFIG				  0x01c8	// ?

#define R128_OVR_CLR					  0x0230
#define R128_OVR_WID_LEFT_RIGHT 		  0x0234
#define R128_OVR_WID_TOP_BOTTOM 		  0x0238

#define R128_PALETTE_DATA				  0x00b4
#define R128_PALETTE_INDEX				  0x00b0
#define R128_PC_NGUI_CTLSTAT			  0x0184
#		define R128_PC_FLUSH_ALL		  0x00ff
#		define R128_PC_BUSY 			  (1 << 31)
#define R128_PPLL_CNTL					  0x0002	// PLL
#		define R128_PPLL_RESET				  (1 <<  0)
#		define R128_PPLL_SLEEP				  (1 <<  1)
#		define R128_PPLL_ATOMIC_UPDATE_EN	  (1 << 16)
#		define R128_PPLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#define R128_PPLL_DIV_3 				  0x0007	// PLL
#		define R128_PPLL_FB3_DIV_MASK	  0x07ff
#		define R128_PPLL_POST3_DIV_MASK   0x00070000
#define R128_PPLL_REF_DIV				  0x0003	// PLL
#		define R128_PPLL_REF_DIV_MASK	  0x03ff
#		define R128_PPLL_ATOMIC_UPDATE_R  (1 << 15)	// same as _W
#		define R128_PPLL_ATOMIC_UPDATE_W  (1 << 15)	// same as _R

#define R128_SC_BOTTOM_RIGHT			  0x16f0
#define R128_SC_TOP_LEFT				  0x16ec
#define R128_SCALE_3D_CNTL				  0x1a00
#define R128_SRC_Y_X					  0x1434
#define R128_SUBPIC_CNTL				  0x0540	// ?

#define R128_TMDS_CRC                     0x02a0

#define R128_VCLK_ECP_CNTL				 0x0008	// PLL
#		define R128_VCLK_SRC_SEL_MASK	 0x03
#		define R128_VCLK_SRC_SEL_CPUCLK  0x00
#		define R128_VCLK_SRC_SEL_PPLLCLK 0x03
#		define R128_ECP_DIV_MASK		 (3 << 8)
#define R128_VIPH_CONTROL				  0x01D0	// ?


// Definitions used for overlays.
//===============================

#define R128_OV0_Y_X_START				0x0400
#define R128_OV0_Y_X_END				0x0404
#define R128_OV0_EXCLUSIVE_HORZ 		0x0408
#define R128_OV0_REG_LOAD_CNTL			0x0410
#define R128_OV0_SCALE_CNTL 			0x0420
#define R128_OV0_V_INC					0x0424
#define R128_OV0_P1_V_ACCUM_INIT		0x0428
#define R128_OV0_P23_V_ACCUM_INIT		0x042C
#define R128_OV0_P1_BLANK_LINES_AT_TOP	0x0430
#define R128_OV0_VID_BUF0_BASE_ADRS 	0x0440
#define R128_OV0_VID_BUF_PITCH0_VALUE	0x0460
#define R128_OV0_AUTO_FLIP_CNTL 		0x0470
#define R128_OV0_H_INC					0x0480
#define R128_OV0_STEP_BY				0x0484
#define R128_OV0_P1_H_ACCUM_INIT		0x0488
#define R128_OV0_P23_H_ACCUM_INIT		0x048C
#define R128_OV0_P1_X_START_END 		0x0494
#define R128_OV0_P2_X_START_END 		0x0498
#define R128_OV0_P3_X_START_END 		0x049C
#define R128_OV0_FILTER_CNTL			0x04A0
#define R128_OV0_COLOUR_CNTL			0x04E0
#define R128_OV0_GRAPHICS_KEY_CLR		0x04EC
#define R128_OV0_GRAPHICS_KEY_MSK		0x04F0
#define R128_OV0_KEY_CNTL				0x04F4
#		define R128_GRAPHIC_KEY_FN_EQ	0x00000040L
#		define R128_GRAPHIC_KEY_FN_NE	0x00000050L
#define R128_OV0_TEST					0x04F8



// Functions to get/set PLL registers.
//=======================================

static inline uint32
GetPLLReg(uint8 index)
{
	OUTREG8(R128_CLOCK_CNTL_INDEX, index & 0x3f);
	return INREG(R128_CLOCK_CNTL_DATA);
}


static inline void
SetPLLReg(uint8 index, uint32 value)
{
	OUTREG8(R128_CLOCK_CNTL_INDEX, ((index) & 0x3f) | R128_PLL_WR_EN);
	OUTREG(R128_CLOCK_CNTL_DATA, value);
}


static inline void
SetPLLReg(uint8 index, uint32 value, uint32 mask)
{
	// Write a value to a PLL reg using a mask.  The mask selects the
	// bits to be modified.

	SetPLLReg(index, (GetPLLReg(index) & ~mask) | (value & mask));
}


#endif // __RAGE128_H__
