/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	DAC registers
*/

#ifndef _DAC_REGS_H
#define _DAC_REGS_H

#define RADEON_DAC_CNTL                     0x0058
#       define RADEON_DAC_RANGE_CNTL_MASK   (3 <<  0)
#       define RADEON_DAC_RANGE_CNTL_PS2	(2 <<  0)
#       define RADEON_DAC_BLANKING          (1 <<  2)
#		define RADEON_DAC_CMP_EN			(1 <<  3)
#		define RADEON_DAC_CMP_OUTPUT		(1 <<  7)
#       define RADEON_DAC_8BIT_EN           (1 <<  8)
#       define RADEON_DAC_VGA_ADR_EN        (1 << 13)
#       define RADEON_DAC_PDWN              (1 << 15)
#       define RADEON_DAC_MASK_ALL          (0xff << 24)
#define RADEON_DAC_CNTL2                    0x007c
#       define RADEON_DAC_CLK_SEL_MASK      (1 <<  0)
#       define RADEON_DAC_CLK_SEL_CRTC      (0 <<  0)
#       define RADEON_DAC_CLK_SEL_CRTC2     (1 <<  0)
#       define RADEON_DAC2_CLK_SEL_MASK     (1 <<  1)
#       define RADEON_DAC2_CLK_SEL_TV       (0 <<  1)
#       define RADEON_DAC2_CLK_SEL_CRT      (1 <<  1)
#       define RADEON_DAC2_PALETTE_ACC_CTL  (1 <<  5)
#       define RADEON_DAC2_CMP_EN			(1 <<  7)
#       define RADEON_DAC2_CMP_OUT_R		(1 <<  8)
#       define RADEON_DAC2_CMP_OUT_G		(1 <<  9)
#       define RADEON_DAC2_CMP_OUT_B		(1 <<  10)
#       define RADEON_DAC2_CMP_OUTPUT		(1 <<  11)
#define RADEON_PALETTE_INDEX                0x00b0
#define RADEON_PALETTE_DATA                 0x00b4
#define RADEON_PALETTE_30_DATA              0x00b8

#define RADEON_DAC_EXT_CNTL						0x0280
#		define RADEON_DAC2_FORCE_BLANK_OFF_EN	(1 << 0)
#		define RADEON_DAC2_FORCE_DATA_EN 		(1 << 1)
#		define RADEON_DAC_FORCE_BLANK_OFF_EN	(1 << 4)
#		define RADEON_DAC_FORCE_DATA_EN 		(1 << 5)
#		define RADEON_DAC_FORCE_DATA_SEL_MASK	(3 << 6)
#		define RADEON_DAC_FORCE_DATA_SEL_R		(0 << 6)
#		define RADEON_DAC_FORCE_DATA_SEL_G		(1 << 6)
#		define RADEON_DAC_FORCE_DATA_SEL_B		(2 << 6)
#		define RADEON_DAC_FORCE_DATA_SEL_RGB	(3 << 6)
#		define RADEON_DAC_FORCE_DATA_SHIFT		8
#		define RADEON_DAC_FORCE_DATA_MASK		(0x3ff << 8)
#define RADEON_DAC_CRC_SIG                  0x02cc

#define RADEON_DAC_DATA                     0x03c9 /* VGA */
#define RADEON_DAC_MASK                     0x03c6 /* VGA */
#define RADEON_DAC_R_INDEX                  0x03c7 /* VGA */
#define RADEON_DAC_W_INDEX                  0x03c8 /* VGA */

#define RADEON_DISP_OUTPUT_CNTL             0x0d64
#       define RADEON_DISP_DAC_SOURCE_MASK  0x03
#       define RADEON_DISP_DAC_SOURCE_CRTC1	0x00
#       define RADEON_DISP_DAC_SOURCE_CRTC2	0x01
#       define RADEON_DISP_DAC_SOURCE_RMX	0x02
#       define RADEON_DISP_TVDAC_SOURCE_MASK  0x0c
#       define RADEON_DISP_TVDAC_SOURCE_CRTC2 0x04

#define RADEON_DISP_HW_DEBUG				0x0d14
#		define RADEON_CRT2_DISP1_SEL		(1 << 5)


#endif
