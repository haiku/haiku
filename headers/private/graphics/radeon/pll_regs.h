/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	PLL registers
*/

#ifndef _PLL_REG_H
#define _PLL_REG_H

// mmio registers
#define RADEON_CLOCK_CNTL_DATA              0x000c
#define RADEON_CLOCK_CNTL_INDEX             0x0008
#       define RADEON_PLL_WR_EN             (1 << 7)
#       define RADEON_PLL_DIV_SEL_MASK      (3 << 8)
#       define RADEON_PLL_DIV_SEL_DIV0      (0 << 8)
#       define RADEON_PLL_DIV_SEL_DIV1      (1 << 8)
#       define RADEON_PLL_DIV_SEL_DIV2      (2 << 8)
#       define RADEON_PLL_DIV_SEL_DIV3      (3 << 8)
#define RADEON_CLK_PIN_CNTL                 0x0001 /* PLL */
#       define RADEON_SCLK_DYN_START_CNTL   (1 << 15)

// PLL Power management registers
#define RADEON_CLK_PWRMGT_CNTL              0x0014
#       define RADEON_ENGIN_DYNCLK_MODE     (1 << 12)
#       define RADEON_ACTIVE_HILO_LAT_MASK  (3 << 13)
#       define RADEON_ACTIVE_HILO_LAT_SHIFT 13
#       define RADEON_DISP_DYN_STOP_LAT_MASK (1 << 12)
#       define RADEON_DYN_STOP_MODE_MASK    (7 << 21)
#define RADEON_PLL_PWRMGT_CNTL              0x0015
#       define RADEON_TCL_BYPASS_DISABLE    (1 << 20)

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
#		define RADEON_PPLL_REF_DIV_ACC_SHIFT 18
#		define RADEON_PPLL_REF_DIV_ACC_MASK	(0x3ff << 18)
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
#		define RADEON_PIXCLK_ALWAYS_ONb		(1 << 6)	// negated
#		define RADEON_PIXCLK_DAC_ALWAYS_ONb	(1 << 7)	// negated
#       define RADEON_ECP_DIV_SHIFT         8
#       define RADEON_ECP_DIV_MASK          (3 << 8)
#       define RADEON_ECP_DIV_VCLK          (0 << 8)
#       define RADEON_ECP_DIV_VCLK_2        (1 << 8)
#		define RADEON_VCLK_ECP_CNTL_BYTE_CLK_POST_DIV_SHIFT 16
#		define RADEON_VCLK_ECP_CNTL_BYTE_CLK_POST_DIV_MASK (3 << 16)
#       define R300_DISP_DAC_PIXCLK_DAC_BLANK_OFF (1<<23)

#define RADEON_HTOTAL_CNTL                  0x0009
#define RADEON_SCLK_CNTL                    0x000d /* PLL */
#       define RADEON_SCLK_SRC_SEL_MASK     0x0007
#       define RADEON_DYN_STOP_LAT_MASK     0x00007ff8
#       define RADEON_CP_MAX_DYN_STOP_LAT   0x0008
#       define RADEON_SCLK_FORCEON_MASK     0xffff8000
#       define RADEON_SCLK_FORCE_DISP2      (1<<15)
#       define RADEON_SCLK_FORCE_CP         (1<<16)
#       define RADEON_SCLK_FORCE_HDP        (1<<17)
#       define RADEON_SCLK_FORCE_DISP1      (1<<18)
#       define RADEON_SCLK_FORCE_TOP        (1<<19)
#       define RADEON_SCLK_FORCE_E2         (1<<20)
#       define RADEON_SCLK_FORCE_SE         (1<<21)
#       define RADEON_SCLK_FORCE_IDCT       (1<<22)
#       define RADEON_SCLK_FORCE_VIP        (1<<23)
#       define RADEON_SCLK_FORCE_RE         (1<<24)
#       define RADEON_SCLK_FORCE_PB         (1<<25)
#       define RADEON_SCLK_FORCE_TAM        (1<<26)
#       define RADEON_SCLK_FORCE_TDM        (1<<27)
#       define RADEON_SCLK_FORCE_RB         (1<<28)
#       define RADEON_SCLK_FORCE_TV_SCLK    (1<<29)
#       define RADEON_SCLK_FORCE_SUBPIC     (1<<30)
#       define RADEON_SCLK_FORCE_OV0        (1<<31)
#       define R300_SCLK_FORCE_VAP          (1<<21)
#       define R300_SCLK_FORCE_SR           (1<<25)
#       define R300_SCLK_FORCE_PX           (1<<26)
#       define R300_SCLK_FORCE_TX           (1<<27)
#       define R300_SCLK_FORCE_US           (1<<28)
#       define R300_SCLK_FORCE_SU           (1<<30)
#define R300_SCLK_CNTL2                     0x1e   /* PLL */
#       define R300_SCLK_TCL_MAX_DYN_STOP_LAT (1<<10)
#       define R300_SCLK_GA_MAX_DYN_STOP_LAT  (1<<11)
#       define R300_SCLK_CBA_MAX_DYN_STOP_LAT (1<<12)
#       define R300_SCLK_FORCE_TCL          (1<<13)
#       define R300_SCLK_FORCE_CBA          (1<<14)
#       define R300_SCLK_FORCE_GA           (1<<15)
#define RADEON_SCLK_MORE_CNTL               0x0035 /* PLL */
#       define RADEON_SCLK_MORE_MAX_DYN_STOP_LAT 0x0007
#       define RADEON_SCLK_MORE_FORCEON     0x0700
#define RADEON_MCLK_CNTL                    0x0012
#       define RADEON_FORCEON_MCLKA         (1 << 16)
#       define RADEON_FORCEON_MCLKB         (1 << 17)
#       define RADEON_FORCEON_YCLKA         (1 << 18)
#       define RADEON_FORCEON_YCLKB         (1 << 19)
#       define RADEON_FORCEON_MC            (1 << 20)
#       define RADEON_FORCEON_AIC           (1 << 21)
#       define R300_DISABLE_MC_MCLKA        (1 << 21)
#       define R300_DISABLE_MC_MCLKB        (1 << 21)
#define RADEON_MCLK_MISC                    0x001f /* PLL */
#       define RADEON_MC_MCLK_MAX_DYN_STOP_LAT (1<<12)
#       define RADEON_IO_MCLK_MAX_DYN_STOP_LAT (1<<13)
#       define RADEON_MC_MCLK_DYN_ENABLE    (1 << 14)
#       define RADEON_IO_MCLK_DYN_ENABLE    (1 << 15)
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
#       define RADEON_PIX2CLK_SRC_SEL_BYTECLK  0x02
#       define RADEON_PIX2CLK_ALWAYS_ONb       (1<<6)
#       define RADEON_PIX2CLK_DAC_ALWAYS_ONb   (1<<7)
#       define RADEON_PIXCLK_TV_SRC_SEL        (1 << 8)
#       define RADEON_DISP_TVOUT_PIXCLK_TV_ALWAYS_ONb (1 << 9)
#       define R300_DVOCLK_ALWAYS_ONb          (1 << 10)
#       define RADEON_PIXCLK_BLEND_ALWAYS_ONb  (1 << 11)
#       define RADEON_PIXCLK_GV_ALWAYS_ONb     (1 << 12)
#       define RADEON_PIXCLK_DIG_TMDS_ALWAYS_ONb (1 << 13)
#       define R300_PIXCLK_DVO_ALWAYS_ONb      (1 << 13)
#       define RADEON_PIXCLK_LVDS_ALWAYS_ONb   (1 << 14)
#       define RADEON_PIXCLK_TMDS_ALWAYS_ONb   (1 << 15)
#       define R300_PIXCLK_TRANS_ALWAYS_ONb    (1 << 16)
#       define R300_PIXCLK_TVO_ALWAYS_ONb      (1 << 17)
#       define R300_P2G2CLK_ALWAYS_ONb         (1 << 18)
#       define R300_P2G2CLK_DAC_ALWAYS_ONb     (1 << 19)
#       define R300_DISP_DAC_PIXCLK_DAC2_BLANK_OFF (1 << 23)
#define RADEON_HTOTAL2_CNTL                  0x002e


#endif
