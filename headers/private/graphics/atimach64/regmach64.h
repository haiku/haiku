/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach64/regmach64.h,v 3.15.2.4 1998/10/18 20:42:06 hohndel Exp $ */
/*
 * Copyright 1992,1993,1994,1995,1996,1997 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Kevin E. Martin not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Kevin E. Martin
 * makes no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * KEVIN E. MARTIN, RICKARD E. FAITH, AND TIAGO GONS DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Modified for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach64 by Kevin E. Martin (martin@cs.unc.edu)
 */
/* $XConsortium: regmach64.h /main/13 1996/10/27 18:06:41 kaleb $ */

#ifndef REGMACH64_H
#define REGMACH64_H

/*#include "compiler.h"*/

/* NON-GUI IO MAPPED Registers */

/*extern unsigned ioCONFIG_CHIP_ID;
extern unsigned ioCONFIG_CNTL;
extern unsigned ioSCRATCH_REG0;
extern unsigned ioSCRATCH_REG1;
extern unsigned ioCONFIG_STAT0;
extern unsigned ioMEM_CNTL;
extern unsigned ioDAC_REGS;
extern unsigned ioDAC_CNTL;
extern unsigned ioGEN_TEST_CNTL;
extern unsigned ioCLOCK_CNTL;
extern unsigned ioCRTC_GEN_CNTL;
*/
/* NON-GUI sparse IO register offsets */

#define sioCONFIG_CHIP_ID	0x1B
#define sioCONFIG_CNTL		0x1A
#define sioSCRATCH_REG0		0x10
#define sioSCRATCH_REG1		0x11
#define sioCONFIG_STAT0		0x1C
#define sioMEM_CNTL		0x14
#define sioDAC_REGS		0x17
#define sioDAC_CNTL		0x18
#define sioGEN_TEST_CNTL 	0x19
#define sioCLOCK_CNTL	 	0x12
#define sioCRTC_GEN_CNTL 	0x07

/* NON-GUI MEMORY MAPPED Registers - expressed in BYTE offsets */

#define CRTC_H_TOTAL_DISP       0x0000  /* Dword offset 00 */
#define CRTC_H_SYNC_STRT_WID    0x0004  /* Dword offset 01 */
#define CRTC_V_TOTAL_DISP       0x0008  /* Dword offset 02 */
#define CRTC_V_SYNC_STRT_WID    0x000C  /* Dword offset 03 */
#define CRTC_VLINE_CRNT_VLINE   0x0010  /* Dword offset 04 */
#define CRTC_OFF_PITCH          0x0014  /* Dword offset 05 */
#define CRTC_INT_CNTL           0x0018  /* Dword offset 06 */
#define CRTC_GEN_CNTL           0x001C  /* Dword offset 07 */

#define DSP_CONFIG              0x0020  /* Dword offset 08 */
#define DSP_ON_OFF              0x0024  /* Dword offset 09 */

#define SHARED_CNTL             0x0038  /* Dword offset 0E */

#define OVR_CLR                 0x0040  /* Dword offset 10 */
#define OVR_WID_LEFT_RIGHT      0x0044  /* Dword offset 11 */
#define OVR_WID_TOP_BOTTOM      0x0048  /* Dword offset 12 */

#define CUR_CLR0                0x0060  /* Dword offset 18 */
#define CUR_CLR1                0x0064  /* Dword offset 19 */
#define CUR_OFFSET              0x0068  /* Dword offset 1A */
#define CUR_HORZ_VERT_POSN      0x006C  /* Dword offset 1B */
#define CUR_HORZ_VERT_OFF       0x0070  /* Dword offset 1C */

#define HW_DEBUG                0x007C  /* Dword offset 1F */

#define SCRATCH_REG0            0x0080  /* Dword offset 20 */
#define SCRATCH_REG1            0x0084  /* Dword offset 21 */

#define CLOCK_CNTL              0x0090  /* Dword offset 24 */

#define BUS_CNTL                0x00A0  /* Dword offset 28 */

#define MEM_CNTL                0x00B0  /* Dword offset 2C */

#define MEM_VGA_WP_SEL          0x00B4  /* Dword offset 2D */
#define MEM_VGA_RP_SEL          0x00B8  /* Dword offset 2E */

#define DAC_REGS                0x00C0  /* Dword offset 30 */
#define DAC_W_INDEX             0x00C0  /* Dword offset 30 */
#define DAC_DATA                0x00C1  /* Dword offset 30 */
#define DAC_MASK                0x00C2  /* Dword offset 30 */
#define DAC_R_INDEX             0x00C3  /* Dword offset 30 */
#define DAC_CNTL                0x00C4  /* Dword offset 31 */

#define GEN_TEST_CNTL           0x00D0  /* Dword offset 34 */

#define CONFIG_CNTL		0x00DC	/* Dword offset 37 (CT, ET, VT) */
#define CONFIG_CHIP_ID          0x00E0  /* Dword offset 38 */
#define CONFIG_STAT0            0x00E4  /* Dword offset 39 */
#define CONFIG_STAT1            0x00E8  /* Dword offset 3A */


/* GUI MEMORY MAPPED Registers */

#define DST_OFF_PITCH           0x0100  /* Dword offset 40 */
#define DST_X                   0x0104  /* Dword offset 41 */
#define DST_Y                   0x0108  /* Dword offset 42 */
#define DST_Y_X                 0x010C  /* Dword offset 43 */
#define DST_WIDTH               0x0110  /* Dword offset 44 */
#define DST_HEIGHT              0x0114  /* Dword offset 45 */
#define DST_HEIGHT_WIDTH        0x0118  /* Dword offset 46 */
#define DST_X_WIDTH             0x011C  /* Dword offset 47 */
#define DST_BRES_LNTH           0x0120  /* Dword offset 48 */
#define DST_BRES_ERR            0x0124  /* Dword offset 49 */
#define DST_BRES_INC            0x0128  /* Dword offset 4A */
#define DST_BRES_DEC            0x012C  /* Dword offset 4B */
#define DST_CNTL                0x0130  /* Dword offset 4C */

#define SRC_OFF_PITCH           0x0180  /* Dword offset 60 */
#define SRC_X                   0x0184  /* Dword offset 61 */
#define SRC_Y                   0x0188  /* Dword offset 62 */
#define SRC_Y_X                 0x018C  /* Dword offset 63 */
#define SRC_WIDTH1              0x0190  /* Dword offset 64 */
#define SRC_HEIGHT1             0x0194  /* Dword offset 65 */
#define SRC_HEIGHT1_WIDTH1      0x0198  /* Dword offset 66 */
#define SRC_X_START             0x019C  /* Dword offset 67 */
#define SRC_Y_START             0x01A0  /* Dword offset 68 */
#define SRC_Y_X_START           0x01A4  /* Dword offset 69 */
#define SRC_WIDTH2              0x01A8  /* Dword offset 6A */
#define SRC_HEIGHT2             0x01AC  /* Dword offset 6B */
#define SRC_HEIGHT2_WIDTH2      0x01B0  /* Dword offset 6C */
#define SRC_CNTL                0x01B4  /* Dword offset 6D */

#define HOST_DATA0              0x0200  /* Dword offset 80 */
#define HOST_DATA1              0x0204  /* Dword offset 81 */
#define HOST_DATA2              0x0208  /* Dword offset 82 */
#define HOST_DATA3              0x020C  /* Dword offset 83 */
#define HOST_DATA4              0x0210  /* Dword offset 84 */
#define HOST_DATA5              0x0214  /* Dword offset 85 */
#define HOST_DATA6              0x0218  /* Dword offset 86 */
#define HOST_DATA7              0x021C  /* Dword offset 87 */
#define HOST_DATA8              0x0220  /* Dword offset 88 */
#define HOST_DATA9              0x0224  /* Dword offset 89 */
#define HOST_DATAA              0x0228  /* Dword offset 8A */
#define HOST_DATAB              0x022C  /* Dword offset 8B */
#define HOST_DATAC              0x0230  /* Dword offset 8C */
#define HOST_DATAD              0x0234  /* Dword offset 8D */
#define HOST_DATAE              0x0238  /* Dword offset 8E */
#define HOST_DATAF              0x023C  /* Dword offset 8F */
#define HOST_CNTL               0x0240  /* Dword offset 90 */

#define PAT_REG0                0x0280  /* Dword offset A0 */
#define PAT_REG1                0x0284  /* Dword offset A1 */
#define PAT_CNTL                0x0288  /* Dword offset A2 */

#define SC_LEFT                 0x02A0  /* Dword offset A8 */
#define SC_RIGHT                0x02A4  /* Dword offset A9 */
#define SC_LEFT_RIGHT           0x02A8  /* Dword offset AA */
#define SC_TOP                  0x02AC  /* Dword offset AB */
#define SC_BOTTOM               0x02B0  /* Dword offset AC */
#define SC_TOP_BOTTOM           0x02B4  /* Dword offset AD */

#define DP_BKGD_CLR             0x02C0  /* Dword offset B0 */
#define DP_FRGD_CLR             0x02C4  /* Dword offset B1 */
#define DP_WRITE_MASK           0x02C8  /* Dword offset B2 */
#define DP_CHAIN_MASK           0x02CC  /* Dword offset B3 */
#define DP_PIX_WIDTH            0x02D0  /* Dword offset B4 */
#define DP_MIX                  0x02D4  /* Dword offset B5 */
#define DP_SRC                  0x02D8  /* Dword offset B6 */

#define CLR_CMP_CLR             0x0300  /* Dword offset C0 */
#define CLR_CMP_MASK            0x0304  /* Dword offset C1 */
#define CLR_CMP_CNTL            0x0308  /* Dword offset C2 */

#define FIFO_STAT               0x0310  /* Dword offset C4 */

#define CONTEXT_MASK            0x0320  /* Dword offset C8 */
#define CONTEXT_LOAD_CNTL       0x032C  /* Dword offset CB */

#define GUI_TRAJ_CNTL           0x0330  /* Dword offset CC */
#define GUI_STAT                0x0338  /* Dword offset CE */


/* CRTC control values */

#define CRTC_H_SYNC_NEG		0x00200000
#define CRTC_V_SYNC_NEG		0x00200000

#define CRTC_DBL_SCAN_EN	0x00000001
#define CRTC_INTERLACE_EN	0x00000002
#define CRTC_HSYNC_DIS		0x00000004
#define CRTC_VSYNC_DIS		0x00000008
#define CRTC_CSYNC_EN		0x00000010
#define CRTC_PIX_BY_2_EN	0x00000020

#define CRTC_PIX_WIDTH		0x00000700
#define CRTC_PIX_WIDTH_4BPP	0x00000100
#define CRTC_PIX_WIDTH_8BPP	0x00000200
#define CRTC_PIX_WIDTH_15BPP	0x00000300
#define CRTC_PIX_WIDTH_16BPP	0x00000400
#define CRTC_PIX_WIDTH_24BPP	0x00000500
#define CRTC_PIX_WIDTH_32BPP	0x00000600

#define CRTC_BYTE_PIX_ORDER	0x00000800
#define CRTC_PIX_ORDER_MSN_LSN	0x00000000
#define CRTC_PIX_ORDER_LSN_MSN	0x00000800

#define CRTC_FIFO_LWM		0x000f0000
#define CRTC_LOCK_REGS		0x00400000
#define CRTC_EXT_DISP_EN	0x01000000
#define CRTC_EXT_EN		0x02000000

#define CRTC_CRNT_VLINE		0x07f00000
#define CRTC_VBLANK		0x00000001

/* DAC control values */

#define DAC_EXT_SEL_RS2		0x01
#define DAC_EXT_SEL_RS3		0x02
#define DAC_8BIT_EN		0x00000100
#define DAC_PIX_DLY_MASK	0x00000600
#define DAC_PIX_DLY_0NS		0x00000000
#define DAC_PIX_DLY_2NS		0x00000200
#define DAC_PIX_DLY_4NS		0x00000400
#define DAC_BLANK_ADJ_MASK	0x00001800
#define DAC_BLANK_ADJ_0		0x00000000
#define DAC_BLANK_ADJ_1		0x00000800
#define DAC_BLANK_ADJ_2		0x00001000


/* Mix control values */

#define MIX_NOT_DST                     0x0000
#define MIX_0                           0x0001
#define MIX_1                           0x0002
#define MIX_DST                         0x0003
#define MIX_NOT_SRC                     0x0004
#define MIX_XOR                         0x0005
#define MIX_XNOR                        0x0006
#define MIX_SRC                         0x0007
#define MIX_NAND                        0x0008
#define MIX_NOT_SRC_OR_DST              0x0009
#define MIX_SRC_OR_NOT_DST              0x000a
#define MIX_OR                          0x000b
#define MIX_AND                         0x000c
#define MIX_SRC_AND_NOT_DST             0x000d
#define MIX_NOT_SRC_AND_DST             0x000e
#define MIX_NOR                         0x000f

/* Maximum engine dimensions */
#define ENGINE_MIN_X        0
#define ENGINE_MIN_Y        0
#define ENGINE_MAX_X        4095
#define ENGINE_MAX_Y        16383

/* Mach64 engine bit constants - these are typically ORed together */

/* HW_DEBUG register constants */
/* For RagePro only... */
#define AUTO_FF_DIS             0x000001000
#define AUTO_BLKWRT_DIS         0x000002000

/* BUS_CNTL register constants */
#define BUS_FIFO_ERR_ACK        0x00200000
#define BUS_HOST_ERR_ACK        0x00800000
#define BUS_APER_REG_DIS        0x00000010

/* GEN_TEST_CNTL register constants */
#define GEN_OVR_OUTPUT_EN       0x20
#define HWCURSOR_ENABLE         0x80
#define GUI_ENGINE_ENABLE       0x100
#define BLOCK_WRITE_ENABLE      0x200

/* DSP_CONFIG register constants */
#define DSP_XCLKS_PER_QW        0x00003fff
#define DSP_LOOP_LATENCY        0x000f0000
#define DSP_PRECISION           0x00700000

/* DSP_ON_OFF register constants */
#define DSP_OFF                 0x000007ff
#define DSP_ON                  0x07ff0000

/* SHARED_CNTL register constants */
#define CTD_FIFO5               0x01000000

/* CLOCK_CNTL register constants */
#define CLOCK_SEL		0x0f
#define CLOCK_DIV		0x30
#define CLOCK_DIV1		0x00
#define CLOCK_DIV2		0x10
#define CLOCK_DIV4		0x20
#define CLOCK_STROBE		0x40
#define PLL_WR_EN		0x02

/* PLL registers */
#define PLL_MACRO_CNTL		0x01
#define PLL_REF_DIV		0x02
#define PLL_GEN_CNTL		0x03
#define MCLK_FB_DIV		0x04
#define PLL_VCLK_CNTL		0x05
#define VCLK_POST_DIV		0x06
#define VCLK0_FB_DIV		0x07
#define VCLK1_FB_DIV		0x08
#define VCLK2_FB_DIV		0x09
#define VCLK3_FB_DIV		0x0A
#define PLL_XCLK_CNTL		0x0B
#define PLL_TEST_CTRL		0x0E
#define PLL_TEST_COUNT		0x0F

/* Fields in PLL registers */
#define PLL_PC_GAIN		0x07
#define PLL_VC_GAIN		0x18
#define PLL_DUTY_CYC		0xE0
#define PLL_OVERRIDE		0x01
#define PLL_MCLK_RST		0x02
#define OSC_EN			0x04
#define EXT_CLK_EN		0x08
#define MCLK_SRC_SEL		0x70
#define EXT_CLK_CNTL		0x80
#define VCLK_SRC_SEL		0x03
#define PLL_VCLK_RST		0x04
#define VCLK_INVERT		0x08
#define VCLK0_POST		0x03
#define VCLK1_POST		0x0C
#define VCLK2_POST		0x30
#define VCLK3_POST		0xC0

/* CONFIG_CNTL register constants */
#define APERTURE_4M_ENABLE      1
#define APERTURE_8M_ENABLE      2
#define VGA_APERTURE_ENABLE     4

/* CONFIG_STAT0 register constants (GX, CX) */
#define CFG_BUS_TYPE		0x00000007
#define CFG_MEM_TYPE		0x00000038
#define CFG_INIT_DAC_TYPE	0x00000e00

/* CONFIG_STAT0 register constants (CT, ET, VT) */
#define CFG_MEM_TYPE_xT		0x00000007

#define ISA			0
#define EISA			1
#define LOCAL_BUS		6
#define PCI			7

/* Memory types for GX, CX */
#define DRAMx4			0
#define VRAMx16			1
#define VRAMx16ssr		2
#define DRAMx16			3
#define GraphicsDRAMx16		4
#define EnhancedVRAMx16		5
#define EnhancedVRAMx16ssr	6

/* Memory types for CT, ET, VT, GT */
#define DRAM			1
#define EDO_DRAM		2
#define PSEUDO_EDO		3
#define SDRAM			4
#define SGRAM			5

#define DAC_INTERNAL		0x00
#define DAC_IBMRGB514		0x01
#define DAC_ATI68875		0x02
#define DAC_TVP3026_A		0x72
#define DAC_BT476		0x03
#define DAC_BT481		0x04
#define DAC_ATT20C491		0x14
#define DAC_SC15026		0x24
#define DAC_MU9C1880		0x34
#define DAC_IMSG174		0x44
#define DAC_ATI68860_B		0x05
#define DAC_ATI68860_C		0x15
#define DAC_TVP3026_B		0x75
#define DAC_STG1700		0x06
#define DAC_ATT498		0x16
#define DAC_STG1702		0x07
#define DAC_SC15021		0x17
#define DAC_ATT21C498		0x27
#define DAC_STG1703		0x37
#define DAC_CH8398		0x47
#define DAC_ATT20C408		0x57

#define CLK_ATI18818_0		0
#define CLK_ATI18818_1		1
#define CLK_STG1703		2
#define CLK_CH8398		3
#define CLK_INTERNAL		4
#define CLK_ATT20C408		5
#define CLK_IBMRGB514		6

/* MEM_CNTL register constants */
#define MEM_SIZE_ALIAS		0x00000007
#define MEM_SIZE_512K		0x00000000
#define MEM_SIZE_1M		0x00000001
#define MEM_SIZE_2M		0x00000002
#define MEM_SIZE_4M		0x00000003
#define MEM_SIZE_6M		0x00000004
#define MEM_SIZE_8M		0x00000005
#define MEM_SIZE_16M		0x00000006
#define MEM_SIZE_ALIAS_GTB	0x0000000F
#define MEM_SIZE_2M_GTB		0x00000003
#define MEM_SIZE_4M_GTB		0x00000007
#define MEM_SIZE_6M_GTB		0x00000009
#define MEM_SIZE_8M_GTB		0x0000000B
#define MEM_SIZE_16M_GTB	0x0000000F
#define MEM_TRP			0x00000300
#define MEM_TRCD		0x00000C00
#define MEM_TCRD		0x00001000
#define MEM_TRAS		0x00070000
#define MEM_BNDRY               0x00030000
#define MEM_BNDRY_0K            0x00000000
#define MEM_BNDRY_256K          0x00010000
#define MEM_BNDRY_512K          0x00020000
#define MEM_BNDRY_1M            0x00030000
#define MEM_BNDRY_EN            0x00040000

/* ATI PCI constants */
#define PCI_ATI_VENDOR_ID	0x1002
#define PCI_MACH64_GX_ID	0x4758
#define PCI_MACH64_CX_ID	0x4358
#define PCI_MACH64_CT_ID	0x4354
#define PCI_MACH64_ET_ID	0x4554
#define PCI_MACH64_VT_ID	0x5654
#define PCI_MACH64_VU_ID	0x5655
#define PCI_MACH64_GT_ID	0x4754
#define PCI_MACH64_GU_ID	0x4755
#define PCI_MACH64_GB_ID	0x4742
#define PCI_MACH64_GD_ID	0x4744
#define PCI_MACH64_GI_ID	0x4749
#define PCI_MACH64_GP_ID	0x4750
#define PCI_MACH64_GQ_ID	0x4751
#define PCI_MACH64_VV_ID	0x5656
#define PCI_MACH64_GV_ID	0x4756
#define PCI_MACH64_GW_ID	0x4757
#define PCI_MACH64_GZ_ID	0x475A
#define PCI_MACH64_LD_ID	0x4C44
#define PCI_MACH64_LG_ID	0x4C47
#define PCI_MACH64_LB_ID	0x4C42
#define PCI_MACH64_LI_ID	0x4C49
#define PCI_MACH64_LP_ID	0x4C50

/* CONFIG_CHIP_ID register constants */
#define CFG_CHIP_TYPE		0x0000FFFF
#define CFG_CHIP_CLASS		0x00FF0000
#define CFG_CHIP_REV		0xFF000000
#define CFG_CHIP_VERSION	0x07000000
#define CFG_CHIP_FOUNDRY	0x38000000
#define CFG_CHIP_REVISION	0xC0000000

/* Chip IDs read from CONFIG_CHIP_ID */
#define MACH64_GX_ID		0xD7
#define MACH64_CX_ID		0x57
#define MACH64_CT_ID		0x4354
#define MACH64_ET_ID		0x4554
#define MACH64_VT_ID		0x5654
#define MACH64_VU_ID		0x5655
#define MACH64_GT_ID		0x4754
#define MACH64_GU_ID		0x4755
#define MACH64_GB_ID		0x4742
#define MACH64_GD_ID		0x4744
#define MACH64_GI_ID		0x4749
#define MACH64_GP_ID		0x4750
#define MACH64_GQ_ID		0x4751
#define MACH64_VV_ID		0x5656
#define MACH64_GV_ID		0x4756
#define MACH64_GW_ID		0x4757
#define MACH64_GZ_ID		0x475A
#define MACH64_LD_ID		0x4C44
#define MACH64_LG_ID		0x4C47
#define MACH64_LB_ID		0x4C42
#define MACH64_LI_ID		0x4C49
#define MACH64_LP_ID		0x4C50
#define MACH64_UNKNOWN_ID	0x0000

/* DST_CNTL register constants */
#define DST_X_RIGHT_TO_LEFT     0
#define DST_X_LEFT_TO_RIGHT     1
#define DST_Y_BOTTOM_TO_TOP     0
#define DST_Y_TOP_TO_BOTTOM     2
#define DST_X_MAJOR             0
#define DST_Y_MAJOR             4
#define DST_X_TILE              8
#define DST_Y_TILE              0x10
#define DST_LAST_PEL            0x20
#define DST_POLYGON_ENABLE      0x40
#define DST_24_ROTATION_ENABLE  0x80

/* SRC_CNTL register constants */
#define SRC_PATTERN_ENABLE      1
#define SRC_ROTATION_ENABLE     2
#define SRC_LINEAR_ENABLE       4
#define SRC_BYTE_ALIGN          8
#define SRC_LINE_X_RIGHT_TO_LEFT 0
#define SRC_LINE_X_LEFT_TO_RIGHT 0x10

/* HOST_CNTL register constants */
#define HOST_BYTE_ALIGN         1

/* GUI_TRAJ_CNTL register constants */
#define PAT_MONO_8x8_ENABLE     0x01000000
#define PAT_CLR_4x2_ENABLE      0x02000000
#define PAT_CLR_8x1_ENABLE      0x04000000

/* DP_CHAIN_MASK register constants */
#define DP_CHAIN_4BPP		0x8888
#define DP_CHAIN_7BPP		0xD2D2
#define DP_CHAIN_8BPP		0x8080
#define DP_CHAIN_8BPP_RGB	0x9292
#define DP_CHAIN_15BPP		0x4210
#define DP_CHAIN_16BPP		0x8410
#define DP_CHAIN_24BPP		0x8080
#define DP_CHAIN_32BPP		0x8080

/* DP_PIX_WIDTH register constants */
#define DST_1BPP                0
#define DST_4BPP                1
#define DST_8BPP                2
#define DST_15BPP               3
#define DST_16BPP               4
#define DST_32BPP               6
#define SRC_1BPP                0
#define SRC_4BPP                0x100
#define SRC_8BPP                0x200
#define SRC_15BPP               0x300
#define SRC_16BPP               0x400
#define SRC_32BPP               0x600
#define HOST_1BPP               0
#define HOST_4BPP               0x10000
#define HOST_8BPP               0x20000
#define HOST_15BPP              0x30000
#define HOST_16BPP              0x40000
#define HOST_32BPP              0x60000
#define BYTE_ORDER_MSB_TO_LSB   0
#define BYTE_ORDER_LSB_TO_MSB   0x1000000

/* DP_MIX register constants */
#define BKGD_MIX_NOT_D              0
#define BKGD_MIX_ZERO               1
#define BKGD_MIX_ONE                2
#define BKGD_MIX_D                  3
#define BKGD_MIX_NOT_S              4
#define BKGD_MIX_D_XOR_S            5
#define BKGD_MIX_NOT_D_XOR_S        6
#define BKGD_MIX_S                  7
#define BKGD_MIX_NOT_D_OR_NOT_S     8
#define BKGD_MIX_D_OR_NOT_S         9
#define BKGD_MIX_NOT_D_OR_S         10
#define BKGD_MIX_D_OR_S             11
#define BKGD_MIX_D_AND_S            12
#define BKGD_MIX_NOT_D_AND_S        13
#define BKGD_MIX_D_AND_NOT_S        14
#define BKGD_MIX_NOT_D_AND_NOT_S    15
#define BKGD_MIX_D_PLUS_S_DIV2      0x17
#define FRGD_MIX_NOT_D              0
#define FRGD_MIX_ZERO               0x10000
#define FRGD_MIX_ONE                0x20000
#define FRGD_MIX_D                  0x30000
#define FRGD_MIX_NOT_S              0x40000
#define FRGD_MIX_D_XOR_S            0x50000
#define FRGD_MIX_NOT_D_XOR_S        0x60000
#define FRGD_MIX_S                  0x70000
#define FRGD_MIX_NOT_D_OR_NOT_S     0x80000
#define FRGD_MIX_D_OR_NOT_S         0x90000
#define FRGD_MIX_NOT_D_OR_S         0xa0000
#define FRGD_MIX_D_OR_S             0xb0000
#define FRGD_MIX_D_AND_S            0xc0000
#define FRGD_MIX_NOT_D_AND_S        0xd0000
#define FRGD_MIX_D_AND_NOT_S        0xe0000
#define FRGD_MIX_NOT_D_AND_NOT_S    0xf0000
#define FRGD_MIX_D_PLUS_S_DIV2      0x170000

/* DP_SRC register constants */
#define BKGD_SRC_BKGD_CLR           0
#define BKGD_SRC_FRGD_CLR           1
#define BKGD_SRC_HOST               2
#define BKGD_SRC_BLIT               3
#define BKGD_SRC_PATTERN            4
#define FRGD_SRC_BKGD_CLR           0
#define FRGD_SRC_FRGD_CLR           0x100
#define FRGD_SRC_HOST               0x200
#define FRGD_SRC_BLIT               0x300
#define FRGD_SRC_PATTERN            0x400
#define MONO_SRC_ONE                0
#define MONO_SRC_PATTERN            0x10000
#define MONO_SRC_HOST               0x20000
#define MONO_SRC_BLIT               0x30000

/* CLR_CMP_CNTL register constants */
#define COMPARE_FALSE               0
#define COMPARE_TRUE                1
#define COMPARE_NOT_EQUAL           4
#define COMPARE_EQUAL               5
#define COMPARE_DESTINATION         0
#define COMPARE_SOURCE              0x1000000

/* FIFO_STAT register constants */
#define FIFO_ERR                    0x80000000

/* CONTEXT_LOAD_CNTL constants */
#define CONTEXT_NO_LOAD             0
#define CONTEXT_LOAD                0x10000
#define CONTEXT_LOAD_AND_DO_FILL    0x20000
#define CONTEXT_LOAD_AND_DO_LINE    0x30000
#define CONTEXT_EXECUTE             0
#define CONTEXT_CMD_DISABLE         0x80000000

/* GUI_STAT register constants */
#define ENGINE_IDLE                 0
#define ENGINE_BUSY                 1
#define SCISSOR_LEFT_FLAG           0x10
#define SCISSOR_RIGHT_FLAG          0x20
#define SCISSOR_TOP_FLAG            0x40
#define SCISSOR_BOTTOM_FLAG         0x80

/* ATI VGA Extended Regsiters */
#define sioATIEXT	0x1ce
#define bioATIEXT	0x3ce
extern unsigned ATIExtReg;
#define ATI2E		0xae
#define ATI32		0xb2
#define ATI36		0xb6

/* VGA Graphics Controller Registers */
#define VGAGRA		0x3ce
#define GRA06		0x06

/* VGA Seququencer Registers */
#define VGASEQ		0x3c4
#define SEQ02		0x02
#define SEQ04		0x04

#define MACH64_MAX_X	ENGINE_MAX_X
#define MACH64_MAX_Y	ENGINE_MAX_Y

#define INC_X           0x0020
#define INC_Y           0x0080

#define RGB16_555               0x0000
#define RGB16_565               0x0040
#define RGB16_655               0x0080
#define RGB16_664               0x00c0

#define POLY_TEXT_TYPE          0x0001
#define IMAGE_TEXT_TYPE         0x0002
#define TEXT_TYPE_8_BIT         0x0004
#define TEXT_TYPE_16_BIT        0x0008
#define POLY_TEXT_TYPE_8        (POLY_TEXT_TYPE | TEXT_TYPE_8_BIT)
#define IMAGE_TEXT_TYPE_8       (IMAGE_TEXT_TYPE | TEXT_TYPE_8_BIT)
#define POLY_TEXT_TYPE_16       (POLY_TEXT_TYPE | TEXT_TYPE_16_BIT)
#define IMAGE_TEXT_TYPE_16      (IMAGE_TEXT_TYPE | TEXT_TYPE_16_BIT)

#define MACH64_NUM_CLOCKS	16
#define MACH64_NUM_FREQS	50

typedef struct {
    unsigned char h_disp;
    unsigned char dacmask;
    unsigned char ram_req;
    unsigned char max_dot_clock;
    unsigned char color_depth;
} mach64FreqRec;

typedef struct {
    unsigned char r, g, b;
} LUTENTRY;

typedef struct {
    unsigned long h_total_disp, h_sync_strt_wid;
    unsigned long v_total_disp, v_sync_strt_wid;
    unsigned long crtc_gen_cntl;
    unsigned long color_depth;
    unsigned long clock_cntl;
    unsigned long dot_clock;
    unsigned long fifo_v1;
} mach64CRTCRegRec, *mach64CRTCRegPtr;


/* Wait until "v" queue entries are free */
#define WaitQueue(v)    { while ((inw(FIFO_STAT) & 0xffff) > \
			 ((unsigned short)(0x8000 >> (v)))); }

/* Wait until GP is idle and queue is empty */
#define WaitIdleEmpty() { WaitQueue(16); \
			  while ((inw(GUI_STAT) & 1) != 0); }

#define SKIP_2(_v) ((((_v)<<1)&0xfff8)|((_v)&0x3)|(((_v)&0x80)>>5))

#define MACH64_BIT_BLT(_srcx, _srcy, _dstx, _dsty, _w, _h, _dir) \
{ \
    WaitQueue(5); \
    outw(SRC_Y_X, (((_srcx) << 16) | ((_srcy) & 0x0000ffff))); \
    outw(SRC_WIDTH1, (_w)); \
    outw(DST_CNTL, (_dir)); \
    outw(DST_Y_X, (((_dstx) << 16) | ((_dsty) & 0x0000ffff))); \
    outw(DST_HEIGHT_WIDTH, (((_w) << 16) | ((_h) & 0x0000ffff))); \
}

#ifndef NULL
#define NULL    0
#endif

#endif /* REGMACH64_H */
