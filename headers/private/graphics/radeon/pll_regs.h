/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	PLL registers and access macros
*/

#ifndef _PLL_REG_H
#define _PLL_REG_H

#include "mmio.h"

// atomic updates of PLL clock don't seem to always work and stick, thus
// the bit never resets. Here - we use our own check by reading back the
// register we've just wrote to make sure it's got the right value 
#define RADEON_ATOMIC_UPDATE 0  // Use PLL Atomic updates (seems broken)


// mmio registers
#define RADEON_CLOCK_CNTL_DATA              0x000c
#define RADEON_CLOCK_CNTL_INDEX             0x0008
#       define RADEON_PLL_WR_EN             (1 << 7)
#       define RADEON_PLL_DIV_SEL_MASK      (3 << 8)
#       define RADEON_PLL_DIV_SEL_DIV0      (0 << 8)
#       define RADEON_PLL_DIV_SEL_DIV1      (1 << 8)
#       define RADEON_PLL_DIV_SEL_DIV2      (2 << 8)
#       define RADEON_PLL_DIV_SEL_DIV3      (3 << 8)


// indirect PLL registers
#define RADEON_CLK_PIN_CNTL                 0x0001
#define RADEON_PPLL_CNTL                    0x0002
#       define RADEON_PPLL_RESET                (1 <<  0)
#       define RADEON_PPLL_SLEEP                (1 <<  1)
#       define RADEON_PPLL_ATOMIC_UPDATE_EN     (1 << 16)
#       define RADEON_PPLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#       define RADEON_PPLL_ATOMIC_UPDATE_VSYNC  (1 << 18)
#define RADEON_PPLL_REF_DIV                 0x0003
#       define RADEON_PPLL_REF_DIV_MASK     0x03ff
#       define RADEON_PPLL_ATOMIC_UPDATE_R  (1 << 15) /* same as _W */
#       define RADEON_PPLL_ATOMIC_UPDATE_W  (1 << 15) /* same as _R */
#define RADEON_PPLL_DIV_0                   0x0004
#define RADEON_PPLL_DIV_1                   0x0005
#define RADEON_PPLL_DIV_2                   0x0006
#define RADEON_PPLL_DIV_3                   0x0007
#       define RADEON_PPLL_FB3_DIV_MASK     0x07ff
#       define RADEON_PPLL_POST3_DIV_MASK   0x00070000
#define RADEON_VCLK_ECP_CNTL                0x0008
#       define RADEON_VCLK_SRC_SEL_MASK     (3 << 0)
#       define RADEON_VCLK_SRC_CPU_CLK      (0 << 0)
#       define RADEON_VCLK_SRC_PSCAN_CLK    (1 << 0)
#       define RADEON_VCLK_SRC_BYTE_CLK     (2 << 0)
#       define RADEON_VCLK_SRC_PPLL_CLK     (3 << 0)
#       define RADEON_ECP_DIV_SHIFT         8
#       define RADEON_ECP_DIV_MASK          (3 << 8)
#       define RADEON_ECP_DIV_VCLK          (0 << 8)
#       define RADEON_ECP_DIV_VCLK_2        (1 << 8)
#define RADEON_HTOTAL_CNTL                  0x0009
#define RADEON_SCLK_CNTL                    0x000d
#       define RADEON_DYN_STOP_LAT_MASK     0x00007ff8
#       define RADEON_CP_MAX_DYN_STOP_LAT   0x0008
#       define RADEON_SCLK_FORCEON_MASK     0xffff8000
#define RADEON_SCLK_MORE_CNTL               0x0035
#       define RADEON_SCLK_MORE_FORCEON     0x0700
#define RADEON_MCLK_CNTL                    0x0012
#       define RADEON_FORCEON_MCLKA         (1 << 16)
#       define RADEON_FORCEON_MCLKB         (1 << 17)
#       define RADEON_FORCEON_YCLKA         (1 << 18)
#       define RADEON_FORCEON_YCLKB         (1 << 19)
#       define RADEON_FORCEON_MC            (1 << 20)
#       define RADEON_FORCEON_AIC           (1 << 21)
#define RADEON_P2PLL_CNTL                   0x002a
#       define RADEON_P2PLL_RESET               (1 <<  0)
#       define RADEON_P2PLL_SLEEP               (1 <<  1)
#       define RADEON_P2PLL_ATOMIC_UPDATE_EN    (1 << 16)
#       define RADEON_P2PLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#       define RADEON_P2PLL_ATOMIC_UPDATE_VSYNC  (1 << 18)
#define RADEON_P2PLL_REF_DIV                 0x002B
#       define RADEON_P2PLL_REF_DIV_MASK     0x03ff
#       define RADEON_P2PLL_ATOMIC_UPDATE_R  (1 << 15) /* same as _W */
#       define RADEON_P2PLL_ATOMIC_UPDATE_W  (1 << 15) /* same as _R */
#define RADEON_P2PLL_DIV_0                   0x002c
#       define RADEON_P2PLL_FB0_DIV_MASK     0x07ff
#       define RADEON_P2PLL_POST0_DIV_MASK   0x00070000
#define RADEON_PIXCLKS_CNTL                  0x0002d
#       define RADEON_PIX2CLK_SRC_SEL_MASK       (3 << 0)
#       define RADEON_PIX2CLK_SRC_SEL_CPU_CLK    (0 << 0)
#       define RADEON_PIX2CLK_SRC_SEL_PSCAN_CLK  (1 << 0)
#       define RADEON_PIX2CLK_SRC_SEL_P2PLL_CLK  (3 << 0)
#       define RADEON_PIXCLK_TV_SRC_SEL_MASK     (1 << 8)
#       define RADEON_PIXCLK_TV_SRC_SEL_PIXCLK   (0 << 8)
#       define RADEON_PIXCLK_TV_SRC_SEL_PIX2CLK  (1 << 8)
#define RADEON_HTOTAL2_CNTL                  0x002e

// r300: to be called after each CLOCK_CNTL_INDEX access;
// all functions declared in this header take care of that
// (hardware bug fix suggested by XFree86)
void R300_PLLFix( accelerator_info *ai );

// in general:
// - the PLL is connected via special port
// - you need first to choose the PLL register and then write/read its value
//
// if atomic updates are not safe we:
// - verify each time whether the right register is chosen
// - verify all values written to PLL-registers


// read value "val" from PLL-register "addr"
uint32 Radeon_INPLL( accelerator_info *ai, int addr );

// write value "val" to PLL-register "addr" 
void Radeon_OUTPLL( accelerator_info *ai, uint8 addr, uint32 val );

// write "val" to PLL-register "addr" keeping bits "mask"
void Radeon_OUTPLLP( accelerator_info *ai, uint8 addr, uint32 val, uint32 mask );

#endif
