/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
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
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 */
#ifndef __R500_REG_H__
#define __R500_REG_H__
 

/* pipe config regs */
#define R300_GA_POLY_MODE				0x4288
#       define R300_FRONT_PTYPE_POINT                   (0 << 4)
#       define R300_FRONT_PTYPE_LINE                    (1 << 4)
#       define R300_FRONT_PTYPE_TRIANGE                 (2 << 4)
#       define R300_BACK_PTYPE_POINT                    (0 << 7)
#       define R300_BACK_PTYPE_LINE                     (1 << 7)
#       define R300_BACK_PTYPE_TRIANGE                  (2 << 7)
#define R300_GA_ROUND_MODE				0x428c
#       define R300_GEOMETRY_ROUND_TRUNC                (0 << 0)
#       define R300_GEOMETRY_ROUND_NEAREST              (1 << 0)
#       define R300_COLOR_ROUND_TRUNC                   (0 << 2)
#       define R300_COLOR_ROUND_NEAREST                 (1 << 2)
#define R300_GB_MSPOS0				        0x4010
#       define R300_MS_X0_SHIFT                         0
#       define R300_MS_Y0_SHIFT                         4
#       define R300_MS_X1_SHIFT                         8
#       define R300_MS_Y1_SHIFT                         12
#       define R300_MS_X2_SHIFT                         16
#       define R300_MS_Y2_SHIFT                         20
#       define R300_MSBD0_Y_SHIFT                       24
#       define R300_MSBD0_X_SHIFT                       28
#define R300_GB_MSPOS1				        0x4014
#       define R300_MS_X3_SHIFT                         0
#       define R300_MS_Y3_SHIFT                         4
#       define R300_MS_X4_SHIFT                         8
#       define R300_MS_Y4_SHIFT                         12
#       define R300_MS_X5_SHIFT                         16
#       define R300_MS_Y5_SHIFT                         20
#       define R300_MSBD1_SHIFT                         24

#define R300_GA_ENHANCE				        0x4274
#       define R300_GA_DEADLOCK_CNTL                    (1 << 0)
#       define R300_GA_FASTSYNC_CNTL                    (1 << 1)
#define R300_RB3D_DSTCACHE_CTLSTAT              0x4e4c
#	define R300_RB3D_DC_FLUSH		(2 << 0)
#	define R300_RB3D_DC_FREE		(2 << 2)
#	define R300_RB3D_DC_FINISH		(1 << 4)
#define R300_RB3D_ZCACHE_CTLSTAT			0x4f18
#       define R300_ZC_FLUSH                            (1 << 0)
#       define R300_ZC_FREE                             (1 << 1)
#       define R300_ZC_FLUSH_ALL                        0x3
#define R400_GB_PIPE_SELECT             0x402c
#define R500_DYN_SCLK_PWMEM_PIPE        0x000d /* PLL */
#define R500_SU_REG_DEST                0x42c8
#define R300_GB_TILE_CONFIG             0x4018
#       define R300_ENABLE_TILING       (1 << 0)
#       define R300_PIPE_COUNT_RV350    (0 << 1)
#       define R300_PIPE_COUNT_R300     (3 << 1)
#       define R300_PIPE_COUNT_R420_3P  (6 << 1)
#       define R300_PIPE_COUNT_R420     (7 << 1)
#       define R300_TILE_SIZE_8         (0 << 4)
#       define R300_TILE_SIZE_16        (1 << 4)
#       define R300_TILE_SIZE_32        (2 << 4)
#       define R300_SUBPIXEL_1_12       (0 << 16)
#       define R300_SUBPIXEL_1_16       (1 << 16)
#define R300_DST_PIPE_CONFIG            0x170c
#       define R300_PIPE_AUTO_CONFIG    (1 << 31)
#define R300_RB2D_DSTCACHE_MODE         0x3428
#       define R300_DC_AUTOFLUSH_ENABLE (1 << 8)
#       define R300_DC_DC_DISABLE_IGNORE_PE (1 << 17)

#define RADEON_CP_STAT		0x7C0
#define RADEON_RBBM_CMDFIFO_ADDR	0xE70
#define RADEON_RBBM_CMDFIFO_DATA	0xE74
#define RADEON_ISYNC_CNTL		0x1724
#	define RADEON_ISYNC_ANY2D_IDLE3D	(1 << 0)
#	define RADEON_ISYNC_ANY3D_IDLE2D	(1 << 1)
#	define RADEON_ISYNC_TRIG2D_IDLE3D	(1 << 2)
#	define RADEON_ISYNC_TRIG3D_IDLE2D	(1 << 3)
#	define RADEON_ISYNC_WAIT_IDLEGUI	(1 << 4)
#	define RADEON_ISYNC_CPSCRATCH_IDLEGUI	(1 << 5)

#define RS480_NB_MC_INDEX               0x168
#	define RS480_NB_MC_IND_WR_EN	(1 << 8)
#define RS480_NB_MC_DATA                0x16c

/*
 * RS690
 */
#define RS690_MCCFG_FB_LOCATION		0x100
#define		RS690_MC_FB_START_MASK		0x0000FFFF
#define		RS690_MC_FB_START_SHIFT		0
#define		RS690_MC_FB_TOP_MASK		0xFFFF0000
#define		RS690_MC_FB_TOP_SHIFT		16
#define RS690_MCCFG_AGP_LOCATION	0x101
#define		RS690_MC_AGP_START_MASK		0x0000FFFF
#define		RS690_MC_AGP_START_SHIFT	0
#define		RS690_MC_AGP_TOP_MASK		0xFFFF0000
#define		RS690_MC_AGP_TOP_SHIFT		16
#define RS690_MCCFG_AGP_BASE		0x102
#define RS690_MCCFG_AGP_BASE_2		0x103
#define RS690_MC_INIT_MISC_LAT_TIMER            0x104
#define RS690_HDP_FB_LOCATION		0x0134
#define RS690_MC_INDEX				0x78
#	define RS690_MC_INDEX_MASK		0x1ff
#	define RS690_MC_INDEX_WR_EN		(1 << 9)
#	define RS690_MC_INDEX_WR_ACK		0x7f
#define RS690_MC_DATA				0x7c
#define RS690_MC_STATUS                         0x90
#define RS690_MC_STATUS_IDLE                    (1 << 0)
#define RS480_AGP_BASE_2		0x0164
#define RS480_MC_MISC_CNTL              0x18
#	define RS480_DISABLE_GTW	(1 << 1)
#	define RS480_GART_INDEX_REG_EN	(1 << 12)
#	define RS690_BLOCK_GFX_D3_EN	(1 << 14)
#define RS480_GART_FEATURE_ID           0x2b
#	define RS480_HANG_EN	        (1 << 11)
#	define RS480_TLB_ENABLE	        (1 << 18)
#	define RS480_P2P_ENABLE	        (1 << 19)
#	define RS480_GTW_LAC_EN	        (1 << 25)
#	define RS480_2LEVEL_GART	(0 << 30)
#	define RS480_1LEVEL_GART	(1 << 30)
#	define RS480_PDC_EN	        (1 << 31)
#define RS480_GART_BASE                 0x2c
#define RS480_GART_CACHE_CNTRL          0x2e
#	define RS480_GART_CACHE_INVALIDATE (1 << 0) /* wait for it to clear */
#define RS480_AGP_ADDRESS_SPACE_SIZE    0x38
#	define RS480_GART_EN	        (1 << 0)
#	define RS480_VA_SIZE_32MB	(0 << 1)
#	define RS480_VA_SIZE_64MB	(1 << 1)
#	define RS480_VA_SIZE_128MB	(2 << 1)
#	define RS480_VA_SIZE_256MB	(3 << 1)
#	define RS480_VA_SIZE_512MB	(4 << 1)
#	define RS480_VA_SIZE_1GB	(5 << 1)
#	define RS480_VA_SIZE_2GB	(6 << 1)
#define RS480_AGP_MODE_CNTL             0x39
#	define RS480_POST_GART_Q_SIZE	(1 << 18)
#	define RS480_NONGART_SNOOP	(1 << 19)
#	define RS480_AGP_RD_BUF_SIZE	(1 << 20)
#	define RS480_REQ_TYPE_SNOOP_SHIFT 22
#	define RS480_REQ_TYPE_SNOOP_MASK  0x3
#	define RS480_REQ_TYPE_SNOOP_DIS	(1 << 24)

#define RS690_AIC_CTRL_SCRATCH		0x3A
#	define RS690_DIS_OUT_OF_PCI_GART_ACCESS	(1 << 1)

/*
 * RS600
 */
#define RS600_MC_STATUS                         0x0
#define RS600_MC_STATUS_IDLE                    (1 << 0)
#define RS600_MC_INDEX                          0x70
#       define RS600_MC_ADDR_MASK               0xffff
#       define RS600_MC_IND_SEQ_RBS_0           (1 << 16)
#       define RS600_MC_IND_SEQ_RBS_1           (1 << 17)
#       define RS600_MC_IND_SEQ_RBS_2           (1 << 18)
#       define RS600_MC_IND_SEQ_RBS_3           (1 << 19)
#       define RS600_MC_IND_AIC_RBS             (1 << 20)
#       define RS600_MC_IND_CITF_ARB0           (1 << 21)
#       define RS600_MC_IND_CITF_ARB1           (1 << 22)
#       define RS600_MC_IND_WR_EN               (1 << 23)
#define RS600_MC_DATA                           0x74
#define RS600_MC_STATUS                         0x0
#       define RS600_MC_IDLE                    (1 << 1)
#define RS600_MC_FB_LOCATION                    0x4
#define		RS600_MC_FB_START_MASK		0x0000FFFF
#define		RS600_MC_FB_START_SHIFT		0
#define		RS600_MC_FB_TOP_MASK		0xFFFF0000
#define		RS600_MC_FB_TOP_SHIFT		16
#define RS600_MC_AGP_LOCATION                   0x5
#define		RS600_MC_AGP_START_MASK		0x0000FFFF
#define		RS600_MC_AGP_START_SHIFT	0
#define		RS600_MC_AGP_TOP_MASK		0xFFFF0000
#define		RS600_MC_AGP_TOP_SHIFT		16
#define RS600_MC_AGP_BASE                          0x6
#define RS600_MC_AGP_BASE_2                        0x7
#define RS600_MC_CNTL1                          0x9
#       define RS600_ENABLE_PAGE_TABLES         (1 << 26)
#define RS600_MC_PT0_CNTL                       0x100
#       define RS600_ENABLE_PT                  (1 << 0)
#       define RS600_EFFECTIVE_L2_CACHE_SIZE(x) ((x) << 15)
#       define RS600_EFFECTIVE_L2_QUEUE_SIZE(x) ((x) << 21)
#       define RS600_INVALIDATE_ALL_L1_TLBS     (1 << 28)
#       define RS600_INVALIDATE_L2_CACHE        (1 << 29)
#define RS600_MC_PT0_CONTEXT0_CNTL              0x102
#       define RS600_ENABLE_PAGE_TABLE          (1 << 0)
#       define RS600_PAGE_TABLE_TYPE_FLAT       (0 << 1)
#define RS600_MC_PT0_SYSTEM_APERTURE_LOW_ADDR   0x112
#define RS600_MC_PT0_SYSTEM_APERTURE_HIGH_ADDR  0x114
#define RS600_MC_PT0_CONTEXT0_DEFAULT_READ_ADDR 0x11c
#define RS600_MC_PT0_CONTEXT0_FLAT_BASE_ADDR    0x12c
#define RS600_MC_PT0_CONTEXT0_FLAT_START_ADDR   0x13c
#define RS600_MC_PT0_CONTEXT0_FLAT_END_ADDR     0x14c
#define RS600_MC_PT0_CLIENT0_CNTL               0x16c
#       define RS600_ENABLE_TRANSLATION_MODE_OVERRIDE       (1 << 0)
#       define RS600_TRANSLATION_MODE_OVERRIDE              (1 << 1)
#       define RS600_SYSTEM_ACCESS_MODE_MASK                (3 << 8)
#       define RS600_SYSTEM_ACCESS_MODE_PA_ONLY             (0 << 8)
#       define RS600_SYSTEM_ACCESS_MODE_USE_SYS_MAP         (1 << 8)
#       define RS600_SYSTEM_ACCESS_MODE_IN_SYS              (2 << 8)
#       define RS600_SYSTEM_ACCESS_MODE_NOT_IN_SYS          (3 << 8)
#       define RS600_SYSTEM_APERTURE_UNMAPPED_ACCESS_PASSTHROUGH        (0 << 10)
#       define RS600_SYSTEM_APERTURE_UNMAPPED_ACCESS_DEFAULT_PAGE       (1 << 10)
#       define RS600_EFFECTIVE_L1_CACHE_SIZE(x) ((x) << 11)
#       define RS600_ENABLE_FRAGMENT_PROCESSING (1 << 14)
#       define RS600_EFFECTIVE_L1_QUEUE_SIZE(x) ((x) << 15)
#       define RS600_INVALIDATE_L1_TLB          (1 << 20)
/* rs600/rs690/rs740 */
#	define RS600_BUS_MASTER_DIS		(1 << 14)
#	define RS600_MSI_REARM		        (1 << 20)
/* see RS400_MSI_REARM in AIC_CNTL for rs480 */



#define RV515_MC_FB_LOCATION		0x01
#define		RV515_MC_FB_START_MASK		0x0000FFFF
#define		RV515_MC_FB_START_SHIFT		0
#define		RV515_MC_FB_TOP_MASK		0xFFFF0000
#define		RV515_MC_FB_TOP_SHIFT		16
#define RV515_MC_AGP_LOCATION		0x02
#define		RV515_MC_AGP_START_MASK		0x0000FFFF
#define		RV515_MC_AGP_START_SHIFT	0
#define		RV515_MC_AGP_TOP_MASK		0xFFFF0000
#define		RV515_MC_AGP_TOP_SHIFT		16
#define RV515_MC_AGP_BASE		0x03
#define RV515_MC_AGP_BASE_2		0x04

#define R520_MC_FB_LOCATION		0x04
#define		R520_MC_FB_START_MASK		0x0000FFFF
#define		R520_MC_FB_START_SHIFT		0
#define		R520_MC_FB_TOP_MASK		0xFFFF0000
#define		R520_MC_FB_TOP_SHIFT		16
#define R520_MC_AGP_LOCATION		0x05
#define		R520_MC_AGP_START_MASK		0x0000FFFF
#define		R520_MC_AGP_START_SHIFT		0
#define		R520_MC_AGP_TOP_MASK		0xFFFF0000
#define		R520_MC_AGP_TOP_SHIFT		16
#define R520_MC_AGP_BASE		0x06
#define R520_MC_AGP_BASE_2		0x07


#define R520_MC_STATUS 0x00
#define R520_MC_STATUS_IDLE (1<<1)
#define RV515_MC_STATUS 0x08
#define RV515_MC_STATUS_IDLE (1<<4)
#define RV515_MC_INIT_MISC_LAT_TIMER            0x09
#define R520_MC_IND_INDEX 0x70
#define R520_MC_IND_WR_EN (1 << 24)
#define R520_MC_IND_DATA  0x74

#define RV515_MC_CNTL          0x5
#	define RV515_MEM_NUM_CHANNELS_MASK  0x3
#define R520_MC_CNTL0          0x8
#	define R520_MEM_NUM_CHANNELS_MASK  (0x3 << 24)
#	define R520_MEM_NUM_CHANNELS_SHIFT  24
#	define R520_MC_CHANNEL_SIZE  (1 << 23)


#endif