/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	DAC registers
*/

#ifndef _DAC_REGS_H
#define _DAC_REGS_H

#define RADEON_DAC_CNTL                     0x0058
#       define RADEON_DAC_RANGE_CNTL        (3 <<  0)
#       define RADEON_DAC_BLANKING          (1 <<  2)
#       define RADEON_DAC_8BIT_EN           (1 <<  8)
#       define RADEON_DAC_VGA_ADR_EN        (1 << 13)
#       define RADEON_DAC_PDWN              (1 << 15)
#       define RADEON_DAC_MASK_ALL          (0xff << 24)
#define RADEON_DAC_CNTL2                    0x007c
#       define RADEON_DAC_CLK_SEL           (1 <<  0)
#       define RADEON_DAC_CLK_SEL_CRTC      (0 <<  0)
#       define RADEON_DAC_CLK_SEL_CRTC2     (1 <<  0)
#       define RADEON_DAC2_CLK_SEL          (1 <<  1)
#       define RADEON_DAC2_PALETTE_ACC_CTL  (1 <<  5)
#define RADEON_PALETTE_INDEX                0x00b0
#define RADEON_PALETTE_DATA                 0x00b4
#define RADEON_PALETTE_30_DATA              0x00b8

#define RADEON_DAC_CRC_SIG                  0x02cc
#define RADEON_DAC_DATA                     0x03c9 /* VGA */
#define RADEON_DAC_MASK                     0x03c6 /* VGA */
#define RADEON_DAC_R_INDEX                  0x03c7 /* VGA */
#define RADEON_DAC_W_INDEX                  0x03c8 /* VGA */

#define RADEON_DISP_OUTPUT_CNTL             0x0d64
#       define RADEON_DISP_DAC_SOURCE_MASK  0x03
#       define RADEON_DISP_DAC_SOURCE_CRTC2 0x01


#endif
