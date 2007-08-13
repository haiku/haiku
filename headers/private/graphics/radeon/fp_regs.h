/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Flat Panel controller registers
*/

#ifndef _FP_REGS_H
#define _FP_REGS_H

#define RADEON_FP_CRTC_H_TOTAL_DISP         0x0250
#define RADEON_FP_CRTC_V_TOTAL_DISP         0x0254
#define RADEON_FP_GEN_CNTL                  0x0284
#       define RADEON_FP_FPON                  (1 <<  0)
#       define RADEON_FP_TMDS_EN               (1 <<  2)
#       define RADEON_FP_PANEL_FORMAT          (1 <<  3)
#       define RADEON_FP_EN_TMDS               (1 <<  7)
#       define RADEON_FP_DETECT_SENSE          (1 <<  8)
#       define RADEON_FP_SEL_CRTC2             (1 << 13)
#       define RADEON_FP_CRTC_DONT_SHADOW_HPAR (1 << 15)
#       define RADEON_FP_CRTC_DONT_SHADOW_VPAR (1 << 16)
#       define RADEON_FP_CRTC_DONT_SHADOW_HEND (1 << 17)
#       define RADEON_FP_CRTC_USE_SHADOW_VEND  (1 << 18)
#       define RADEON_FP_RMX_HVSYNC_CONTROL_EN (1 << 20)
#       define RADEON_FP_DFP_SYNC_SEL          (1 << 21)
#       define RADEON_FP_CRTC_LOCK_8DOT        (1 << 22)
#       define RADEON_FP_CRT_SYNC_SEL          (1 << 23)
#       define RADEON_FP_USE_SHADOW_EN         (1 << 24)
#       define RADEON_FP_CRT_SYNC_ALT          (1 << 26)
#define RADEON_FP2_GEN_CNTL                 0x0288
#       define RADEON_FP2_BLANK_EN              (1 <<  1)
#       define RADEON_FP2_FPON                  (1 <<  2)
#       define RADEON_FP2_PANEL_FORMAT          (1 <<  3)
#       define RADEON_FP2_SOURCE_SEL_MASK       (3 << 10)
#       define RADEON_FP2_SOURCE_SEL_CRTC2      (1 << 10)
#       define RADEON_FP2_SOURCE_SEL_RMX        (2 << 10) 
#       define RADEON_FP2_SRC_SEL_MASK          (3 << 13)
#       define RADEON_FP2_SRC_SEL_CRTC2         (1 << 13)
#       define RADEON_FP2_FP_POL                (1 << 16)
#       define RADEON_FP2_LP_POL                (1 << 17)
#       define RADEON_FP2_SCK_POL               (1 << 18)
#       define RADEON_FP2_LCD_CNTL_MASK         (7 << 19)
#       define RADEON_FP2_PAD_FLOP_EN           (1 << 22)
#       define RADEON_FP2_CRC_EN                (1 << 23)
#       define RADEON_FP2_CRC_READ_EN           (1 << 24)
#       define RADEON_FP2_DV0_EN                (1 << 25)
#       define RADEON_FP2_DV0_RATE_SEL_SDR      (1 << 26)
#define RADEON_FP_HORZ_STRETCH              0x028c
#       define RADEON_HORZ_STRETCH_RATIO_MASK  0xffff
#       define RADEON_HORZ_STRETCH_RATIO_MAX   4096
#       define RADEON_HORZ_PANEL_SIZE          (0x1ff   << 16)
#       define RADEON_HORZ_PANEL_SIZE_SHIFT		16
#       define RADEON_HORZ_STRETCH_PIXREP      (0      << 25)
#       define RADEON_HORZ_STRETCH_BLEND       (1      << 26)
#       define RADEON_HORZ_STRETCH_ENABLE      (1      << 25)
#       define RADEON_HORZ_AUTO_RATIO          (1      << 27)
#       define RADEON_HORZ_FP_LOOP_STRETCH     (0x7    << 28)
#       define RADEON_HORZ_AUTO_RATIO_INC      (1      << 31)
#define RADEON_FP_VERT_STRETCH              0x0290
#       define RADEON_VERT_PANEL_SIZE          (0xfff << 12)
#       define RADEON_VERT_PANEL_SIZE_SHIFT		12
#       define RADEON_VERT_STRETCH_RATIO_MASK  0xfff
#       define RADEON_VERT_STRETCH_RATIO_SHIFT 0
#       define RADEON_VERT_STRETCH_RATIO_MAX   4096
#       define RADEON_VERT_STRETCH_ENABLE      (1     << 25)
#       define RADEON_VERT_STRETCH_LINEREP     (0     << 26)
#       define RADEON_VERT_STRETCH_BLEND       (1     << 26)
#       define RADEON_VERT_AUTO_RATIO_EN       (1     << 27)
#       define RADEON_VERT_STRETCH_RESERVED    0xf1000000

#define RADEON_TMDS_PLL_CNTL                0x02a8
#define RADEON_TMDS_TRANSMITTER_CNTL        0x02a4
#       define RADEON_TMDS_TRANSMITTER_PLLEN  1
#       define RADEON_TMDS_TRANSMITTER_PLLRST 2

#define RADEON_FP_H_SYNC_STRT_WID           0x02c4
#define RADEON_FP_V_SYNC_STRT_WID           0x02c8

#define RADEON_LVDS_GEN_CNTL                0x02d0
#       define RADEON_LVDS_ON               (1   <<  0)
#       define RADEON_LVDS_DISPLAY_DIS      (1   <<  1)
#       define RADEON_LVDS_PANEL_TYPE       (1   <<  2)
#       define RADEON_LVDS_PANEL_FORMAT     (1   <<  3)
#       define RADEON_LVDS_EN               (1   <<  7)
#       define RADEON_LVDS_DIGON            (1   << 18)
#       define RADEON_LVDS_BLON             (1   << 19)
#       define RADEON_LVDS_SEL_CRTC2        (1   << 23)
#define RADEON_LVDS_PLL_CNTL                0x02d4
#       define RADEON_HSYNC_DELAY_SHIFT     28
#       define RADEON_HSYNC_DELAY_MASK      (0xf << 28)

#define RADEON_FP_CRTC2_H_TOTAL_DISP        0x0350
#define RADEON_FP_CRTC2_V_TOTAL_DISP        0x0354
/*added for FP support------------------------------------------*/
#       define RADEON_FP_CRTC_H_TOTAL_MASK                0x000003ff
#       define RADEON_FP_CRTC_H_DISP_MASK                 0x01ff0000
#       define RADEON_FP_CRTC_V_TOTAL_MASK                0x00000fff
#       define RADEON_FP_CRTC_V_DISP_MASK                 0x0fff0000
#       define RADEON_FP_H_SYNC_STRT_CHAR_MASK      0x00001ff8
#       define RADEON_FP_H_SYNC_WID_MASK            0x003f0000
#       define RADEON_FP_V_SYNC_STRT_MASK           0x00000fff
#       define RADEON_FP_V_SYNC_WID_MASK            0x001f0000
#       define RADEON_FP_CRTC_H_TOTAL_SHIFT               0x00000000
#       define RADEON_FP_CRTC_H_DISP_SHIFT                0x00000010
#       define RADEON_FP_CRTC_V_TOTAL_SHIFT               0x00000000
#       define RADEON_FP_CRTC_V_DISP_SHIFT                0x00000010
#       define RADEON_FP_H_SYNC_STRT_CHAR_SHIFT     0x00000003
#       define RADEON_FP_H_SYNC_WID_SHIFT           0x00000010
#       define RADEON_FP_V_SYNC_STRT_SHIFT          0x00000000
#       define RADEON_FP_V_SYNC_WID_SHIFT           0x00000010
/*-----------------------------------------------------------------*/

#define RADEON_FP_HORZ2_STRETCH             0x038c
#define RADEON_FP_VERT2_STRETCH             0x0390
#define RADEON_FP_H2_SYNC_STRT_WID          0x03c4

#define RADEON_FP_V2_SYNC_STRT_WID          0x03c8

#define RADEON_BIOS_0_SCRATCH 0x0010
#define RADEON_BIOS_1_SCRATCH 0x0014
#define RADEON_BIOS_2_SCRATCH 0x0018
#define RADEON_BIOS_3_SCRATCH 0x001c
#define RADEON_BIOS_4_SCRATCH 0x0020
#define RADEON_BIOS_5_SCRATCH 0x0024
#define RADEON_BIOS_6_SCRATCH 0x0028
#define RADEON_BIOS_7_SCRATCH 0x002c

#endif
