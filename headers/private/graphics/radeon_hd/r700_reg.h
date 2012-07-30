/*
 * Copyright 2009 Advanced Micro Devices, Inc.
 * Copyright 2009 Red Hat Inc.
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
#ifndef R700_H
#define R700_H


/* Scratch Registers */
#define	R700_SCRATCH_REG0					0x8500
#define	R700_SCRATCH_REG1					0x8504
#define	R700_SCRATCH_REG2					0x8508
#define	R700_SCRATCH_REG3					0x850C
#define	R700_SCRATCH_REG4					0x8510
#define	R700_SCRATCH_REG5					0x8514
#define	R700_SCRATCH_REG6					0x8518
#define	R700_SCRATCH_REG7					0x851C
#define	R700_SCRATCH_UMSK					0x8540
#define	R700_SCRATCH_ADDR					0x8544

/* CRT controler register offset */
#define R700_CRTC0_REGISTER_OFFSET			0x0
#define R700_CRTC1_REGISTER_OFFSET			0x800

#define R700_MAX_SH_GPRS				256
#define R700_MAX_TEMP_GPRS				16
#define R700_MAX_SH_THREADS				256
#define R700_MAX_SH_STACK_ENTRIES		4096
#define R700_MAX_BACKENDS				8
#define R700_MAX_BACKENDS_MASK			0xff
#define R700_MAX_SIMDS					16
#define R700_MAX_SIMDS_MASK				0xffff
#define R700_MAX_PIPES					8
#define R700_MAX_PIPES_MASK				0xff

/* Registers */
#define R700_CB_COLOR0_BASE				0x28040
#define R700_CB_COLOR1_BASE				0x28044
#define R700_CB_COLOR2_BASE				0x28048
#define R700_CB_COLOR3_BASE				0x2804C
#define R700_CB_COLOR4_BASE				0x28050
#define R700_CB_COLOR5_BASE				0x28054
#define R700_CB_COLOR6_BASE				0x28058
#define R700_CB_COLOR7_BASE				0x2805C
#define R700_CB_COLOR7_FRAG				0x280FC

#define	R700_CC_GC_SHADER_PIPE_CONFIG	0x8950
#define	R700_CC_RB_BACKEND_DISABLE		0x98F4
#define		R700_BACKEND_DISABLE(x)		((x) << 16)
#define	R700_CC_SYS_RB_BACKEND_DISABLE	0x3F88

#define	R700_CGTS_SYS_TCC_DISABLE		0x3F90
#define	R700_CGTS_TCC_DISABLE			0x9148
#define	R700_CGTS_USER_SYS_TCC_DISABLE	0x3F94
#define	R700_CGTS_USER_TCC_DISABLE		0x914C

#define	R700_CP_ME_CNTL					0x86D8
#define		R700_CP_ME_HALT				(1<<28)
#define		R700_CP_PFP_HALT			(1<<26)
#define	R700_CP_ME_RAM_DATA				0xC160
#define	R700_CP_ME_RAM_RADDR			0xC158
#define	R700_CP_ME_RAM_WADDR			0xC15C
#define R700_CP_MEQ_THRESHOLDS			0x8764
#define		STQ_SPLIT(x)				((x) << 0)
#define	R700_CP_PERFMON_CNTL			0x87FC
#define	R700_CP_PFP_UCODE_ADDR			0xC150
#define	R700_CP_PFP_UCODE_DATA			0xC154
#define	R700_CP_QUEUE_THRESHOLDS		0x8760
#define		R700_ROQ_IB1_START(x)		((x) << 0)
#define		R700_ROQ_IB2_START(x)		((x) << 8)
#define R700_CP_DEBUG					0xC1FC
#define R700_CP_RB_BASE					0xC100
#define	R700_CP_RB_CNTL					0xC104
#define		R700_RB_BUFSZ(x)			((x) << 0)
#define		R700_RB_BLKSZ(x)			((x) << 8)
#define		R700_RB_NO_UPDATE			(1 << 27)
#define		R700_RB_RPTR_WR_ENA			(1 << 31)
#define		R700_BUF_SWAP_32BIT			(2 << 16)
#define	R700_CP_RB_RPTR					0x8700
#define	R700_CP_RB_RPTR_ADDR			0xC10C
#define	R700_CP_RB_RPTR_ADDR_HI			0xC110
#define	R700_CP_RB_RPTR_WR				0xC108
#define	R700_CP_RB_WPTR					0xC114
#define	R700_CP_RB_WPTR_ADDR			0xC118
#define	R700_CP_RB_WPTR_ADDR_HI			0xC11C
#define	R700_CP_RB_WPTR_DELAY			0x8704
#define	R700_CP_SEM_WAIT_TIMER			0x85BC

#define	R700_DB_DEBUG3					0x98B0
#define		R700_DB_CLK_OFF_DELAY(x)	((x) << 11)
#define R700_DB_DEBUG					0x9B8C
#define		R700_DISABLE_TILE_COVERED_FOR_PS_ITER (1 << 6)

#define	R700_DCP_TILING_CONFIG			0x6CA0
#define		R700_PIPE_TILING(x)			((x) << 1)
#define 	R700_BANK_TILING(x)			((x) << 4)
#define		R700_GROUP_SIZE(x)			((x) << 6)
#define		R700_ROW_TILING(x)			((x) << 8)
#define		R700_BANK_SWAPS(x)			((x) << 11)
#define		R700_SAMPLE_SPLIT(x)		((x) << 14)
#define		R700_BACKEND_MAP(x)			((x) << 16)

#define R700_GB_TILING_CONFIG			0x98F0

#define	R700_GC_USER_SHADER_PIPE_CONFIG	0x8954
#define		R700_INACTIVE_QD_PIPES(x)	((x) << 8)
#define		R700_INACTIVE_QD_PIPES_MASK	0x0000FF00
#define		R700_INACTIVE_SIMDS(x)		((x) << 16)
#define		R700_INACTIVE_SIMDS_MASK	0x00FF0000

#define	R700_GRBM_CNTL					0x8000
#define		R700_GRBM_READ_TIMEOUT(x)	((x) << 0)
#define	R700_GRBM_SOFT_RESET			0x8020
#define		R700_SOFT_RESET_CP			(1<<0)
#define	R700_GRBM_STATUS				0x8010
#define		R700_CMDFIFO_AVAIL_MASK		0x0000000F
#define		R700_GUI_ACTIVE				(1<<31)
#define	R700_GRBM_STATUS2				0x8014

#define	R700_CG_MULT_THERMAL_STATUS		0x740
#define		R700_ASIC_T(x)				((x) << 16)
#define		R700_ASIC_T_MASK			0x3FF0000
#define		R700_ASIC_T_SHIFT			16

#define	R700_HDP_HOST_PATH_CNTL				0x2C00
#define	R700_HDP_NONSURFACE_BASE			0x2C04
#define	R700_HDP_NONSURFACE_INFO			0x2C08
#define	R700_HDP_NONSURFACE_SIZE			0x2C0C
#define R700_HDP_REG_COHERENCY_FLUSH_CNTL	0x54A0
#define	R700_HDP_TILING_CONFIG				0x2F3C
#define R700_HDP_DEBUG1						0x2F34

#define R700_MC_SHARED_CHMAP				0x2004
#define		R700_NOOFCHAN_SHIFT				12
#define		R700_NOOFCHAN_MASK				0x00003000
#define R700_MC_SHARED_CHREMAP				0x2008

#define	R700_MC_ARB_RAMCFG					0x2760
#define		R700_NOOFBANK_SHIFT				0
#define		R700_NOOFBANK_MASK				0x00000003
#define		R700_NOOFRANK_SHIFT				2
#define		R700_NOOFRANK_MASK				0x00000004
#define		R700_NOOFROWS_SHIFT				3
#define		R700_NOOFROWS_MASK				0x00000038
#define		R700_NOOFCOLS_SHIFT				6
#define		R700_NOOFCOLS_MASK				0x000000C0
#define		R700_CHANSIZE_SHIFT				8
#define		R700_CHANSIZE_MASK				0x00000100
#define		R700_BURSTLENGTH_SHIFT			9
#define		R700_BURSTLENGTH_MASK			0x00000200
#define		R700_CHANSIZE_OVERRIDE			(1 << 11)
#define	R700_MC_VM_AGP_TOP					0x2028
#define	R700_MC_VM_AGP_BOT					0x202C
#define	R700_MC_VM_AGP_BASE					0x2030
#define	R700_MC_VM_FB_LOCATION				0x2024
#define	R700_MC_VM_MB_L1_TLB0_CNTL			0x2234
#define	R700_MC_VM_MB_L1_TLB1_CNTL			0x2238
#define	R700_MC_VM_MB_L1_TLB2_CNTL			0x223C
#define	R700_MC_VM_MB_L1_TLB3_CNTL			0x2240
#define		R700_ENABLE_L1_TLB					(1 << 0)
#define		R700_ENABLE_L1_FRAGMENT_PROCESSING	(1 << 1)
#define		R700_SYSTEM_ACCESS_MODE_PA_ONLY		(0 << 3)
#define		R700_SYSTEM_ACCESS_MODE_USE_SYS_MAP	(1 << 3)
#define		R700_SYSTEM_ACCESS_MODE_IN_SYS		(2 << 3)
#define		R700_SYSTEM_ACCESS_MODE_NOT_IN_SYS	(3 << 3)
#define		R700_SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU (0 << 5)
#define		R700_EFFECTIVE_L1_TLB_SIZE(x)		((x)<<15)
#define		R700_EFFECTIVE_L1_QUEUE_SIZE(x)		((x)<<18)
#define	R700_MC_VM_MD_L1_TLB0_CNTL				0x2654
#define	R700_MC_VM_MD_L1_TLB1_CNTL				0x2658
#define	R700_MC_VM_MD_L1_TLB2_CNTL				0x265C
#define	R700_MC_VM_SYSTEM_APERTURE_DEFAULT_ADDR 0x203C
#define	R700_MC_VM_SYSTEM_APERTURE_HIGH_ADDR	0x2038
#define	R700_MC_VM_SYSTEM_APERTURE_LOW_ADDR		0x2034

#define	R700_PA_CL_ENHANCE						0x8A14
#define		R700_CLIP_VTX_REORDER_ENA			(1 << 0)
#define		R700_NUM_CLIP_SEQ(x)				((x) << 1)
#define R700_PA_SC_AA_CONFIG					0x28C04
#define R700_PA_SC_CLIPRECT_RULE				0x2820C
#define	R700_PA_SC_EDGERULE						0x28230
#define	R700_PA_SC_FIFO_SIZE					0x8BCC
#define		R700_SC_PRIM_FIFO_SIZE(x)			((x) << 0)
#define		R700_SC_HIZ_TILE_FIFO_SIZE(x)		((x) << 12)
#define	R700_PA_SC_FORCE_EOV_MAX_CNTS			0x8B24
#define		R700_FORCE_EOV_MAX_CLK_CNT(x)		((x)<<0)
#define		R700_FORCE_EOV_MAX_REZ_CNT(x)		((x)<<16)
#define R700_PA_SC_LINE_STIPPLE					0x28A0C
#define	R700_PA_SC_LINE_STIPPLE_STATE			0x8B10
#define R700_PA_SC_MODE_CNTL					0x28A4C
#define	R700_PA_SC_MULTI_CHIP_CNTL				0x8B20
#define		R700_SC_EARLYZ_TILE_FIFO_SIZE(x)	((x) << 20)

#if 0
#define	SMX_DC_CTL0					0xA020
#define		USE_HASH_FUNCTION				(1 << 0)
#define		CACHE_DEPTH(x)					((x) << 1)
#define		FLUSH_ALL_ON_EVENT				(1 << 10)
#define		STALL_ON_EVENT					(1 << 11)
#define	SMX_EVENT_CTL					0xA02C
#define		ES_FLUSH_CTL(x)					((x) << 0)
#define		GS_FLUSH_CTL(x)					((x) << 3)
#define		ACK_FLUSH_CTL(x)				((x) << 6)
#define		SYNC_FLUSH_CTL					(1 << 8)

#define	SPI_CONFIG_CNTL					0x9100
#define		GPR_WRITE_PRIORITY(x)				((x) << 0)
#define		DISABLE_INTERP_1				(1 << 5)
#define	SPI_CONFIG_CNTL_1				0x913C
#define		VTX_DONE_DELAY(x)				((x) << 0)
#define		INTERP_ONE_PRIM_PER_ROW				(1 << 4)
#define	SPI_INPUT_Z					0x286D8
#define	SPI_PS_IN_CONTROL_0				0x286CC
#define		NUM_INTERP(x)					((x)<<0)
#define		POSITION_ENA					(1<<8)
#define		POSITION_CENTROID				(1<<9)
#define		POSITION_ADDR(x)				((x)<<10)
#define		PARAM_GEN(x)					((x)<<15)
#define		PARAM_GEN_ADDR(x)				((x)<<19)
#define		BARYC_SAMPLE_CNTL(x)				((x)<<26)
#define		PERSP_GRADIENT_ENA				(1<<28)
#define		LINEAR_GRADIENT_ENA				(1<<29)
#define		POSITION_SAMPLE					(1<<30)
#define		BARYC_AT_SAMPLE_ENA				(1<<31)

#define	SQ_CONFIG					0x8C00
#define		VC_ENABLE					(1 << 0)
#define		EXPORT_SRC_C					(1 << 1)
#define		DX9_CONSTS					(1 << 2)
#define		ALU_INST_PREFER_VECTOR				(1 << 3)
#define		DX10_CLAMP					(1 << 4)
#define		CLAUSE_SEQ_PRIO(x)				((x) << 8)
#define		PS_PRIO(x)					((x) << 24)
#define		VS_PRIO(x)					((x) << 26)
#define		GS_PRIO(x)					((x) << 28)
#define	SQ_DYN_GPR_SIZE_SIMD_AB_0			0x8DB0
#define		SIMDA_RING0(x)					((x)<<0)
#define		SIMDA_RING1(x)					((x)<<8)
#define		SIMDB_RING0(x)					((x)<<16)
#define		SIMDB_RING1(x)					((x)<<24)
#define	SQ_DYN_GPR_SIZE_SIMD_AB_1			0x8DB4
#define	SQ_DYN_GPR_SIZE_SIMD_AB_2			0x8DB8
#define	SQ_DYN_GPR_SIZE_SIMD_AB_3			0x8DBC
#define	SQ_DYN_GPR_SIZE_SIMD_AB_4			0x8DC0
#define	SQ_DYN_GPR_SIZE_SIMD_AB_5			0x8DC4
#define	SQ_DYN_GPR_SIZE_SIMD_AB_6			0x8DC8
#define	SQ_DYN_GPR_SIZE_SIMD_AB_7			0x8DCC
#define		ES_PRIO(x)					((x) << 30)
#define	SQ_GPR_RESOURCE_MGMT_1				0x8C04
#define		NUM_PS_GPRS(x)					((x) << 0)
#define		NUM_VS_GPRS(x)					((x) << 16)
#define		DYN_GPR_ENABLE					(1 << 27)
#define		NUM_CLAUSE_TEMP_GPRS(x)				((x) << 28)
#define	SQ_GPR_RESOURCE_MGMT_2				0x8C08
#define		NUM_GS_GPRS(x)					((x) << 0)
#define		NUM_ES_GPRS(x)					((x) << 16)
#define	SQ_MS_FIFO_SIZES				0x8CF0
#define		CACHE_FIFO_SIZE(x)				((x) << 0)
#define		FETCH_FIFO_HIWATER(x)				((x) << 8)
#define		DONE_FIFO_HIWATER(x)				((x) << 16)
#define		ALU_UPDATE_FIFO_HIWATER(x)			((x) << 24)
#define	SQ_STACK_RESOURCE_MGMT_1			0x8C10
#define		NUM_PS_STACK_ENTRIES(x)				((x) << 0)
#define		NUM_VS_STACK_ENTRIES(x)				((x) << 16)
#define	SQ_STACK_RESOURCE_MGMT_2			0x8C14
#define		NUM_GS_STACK_ENTRIES(x)				((x) << 0)
#define		NUM_ES_STACK_ENTRIES(x)				((x) << 16)
#define	SQ_THREAD_RESOURCE_MGMT				0x8C0C
#define		NUM_PS_THREADS(x)				((x) << 0)
#define		NUM_VS_THREADS(x)				((x) << 8)
#define		NUM_GS_THREADS(x)				((x) << 16)
#define		NUM_ES_THREADS(x)				((x) << 24)

#define	SX_DEBUG_1					0x9058
#define		ENABLE_NEW_SMX_ADDRESS				(1 << 16)
#define	SX_EXPORT_BUFFER_SIZES				0x900C
#define		COLOR_BUFFER_SIZE(x)				((x) << 0)
#define		POSITION_BUFFER_SIZE(x)				((x) << 8)
#define		SMX_BUFFER_SIZE(x)				((x) << 16)
#define	SX_MISC						0x28350

#define	TA_CNTL_AUX					0x9508
#define		DISABLE_CUBE_WRAP				(1 << 0)
#define		DISABLE_CUBE_ANISO				(1 << 1)
#define		SYNC_GRADIENT					(1 << 24)
#define		SYNC_WALKER					(1 << 25)
#define		SYNC_ALIGNER					(1 << 26)
#define		BILINEAR_PRECISION_6_BIT			(0 << 31)
#define		BILINEAR_PRECISION_8_BIT			(1 << 31)

#define	TCP_CNTL					0x9610
#define	TCP_CHAN_STEER					0x9614

#define	VGT_CACHE_INVALIDATION				0x88C4
#define		CACHE_INVALIDATION(x)				((x)<<0)
#define			VC_ONLY						0
#define			TC_ONLY						1
#define			VC_AND_TC					2
#define		AUTO_INVLD_EN(x)				((x) << 6)
#define			NO_AUTO						0
#define			ES_AUTO						1
#define			GS_AUTO						2
#define			ES_AND_GS_AUTO					3
#define	VGT_ES_PER_GS					0x88CC
#define	VGT_GS_PER_ES					0x88C8
#define	VGT_GS_PER_VS					0x88E8
#define	VGT_GS_VERTEX_REUSE				0x88D4
#define	VGT_NUM_INSTANCES				0x8974
#define	VGT_OUT_DEALLOC_CNTL				0x28C5C
#define		DEALLOC_DIST_MASK				0x0000007F
#define	VGT_STRMOUT_EN					0x28AB0
#define	VGT_VERTEX_REUSE_BLOCK_CNTL			0x28C58
#define		VTX_REUSE_DEPTH_MASK				0x000000FF

#define VM_CONTEXT0_CNTL				0x1410
#define		ENABLE_CONTEXT					(1 << 0)
#define		PAGE_TABLE_DEPTH(x)				(((x) & 3) << 1)
#define		RANGE_PROTECTION_FAULT_ENABLE_DEFAULT		(1 << 4)
#define	VM_CONTEXT0_PAGE_TABLE_BASE_ADDR		0x153C
#define	VM_CONTEXT0_PAGE_TABLE_END_ADDR			0x157C
#define	VM_CONTEXT0_PAGE_TABLE_START_ADDR		0x155C
#define VM_CONTEXT0_PROTECTION_FAULT_DEFAULT_ADDR	0x1518
#define VM_L2_CNTL					0x1400
#define		ENABLE_L2_CACHE					(1 << 0)
#define		ENABLE_L2_FRAGMENT_PROCESSING			(1 << 1)
#define		ENABLE_L2_PTE_CACHE_LRU_UPDATE_BY_WRITE		(1 << 9)
#define		EFFECTIVE_L2_QUEUE_SIZE(x)			(((x) & 7) << 14)
#define VM_L2_CNTL2					0x1404
#define		INVALIDATE_ALL_L1_TLBS				(1 << 0)
#define		INVALIDATE_L2_CACHE				(1 << 1)
#define VM_L2_CNTL3					0x1408
#define		BANK_SELECT(x)					((x) << 0)
#define		CACHE_UPDATE_MODE(x)				((x) << 6)
#define	VM_L2_STATUS					0x140C
#define		L2_BUSY						(1 << 0)

#define	WAIT_UNTIL					0x8040

#define	SRBM_STATUS				        0x0E50
#endif

#define R700_D1GRPH_PRIMARY_SURFACE_ADDRESS			0x6110
#define R700_D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH	0x6914
#define R700_D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH	0x6114
#define R700_D1GRPH_SECONDARY_SURFACE_ADDRESS		0x6118
#define R700_D1GRPH_SECONDARY_SURFACE_ADDRESS_HIGH	0x691c
#define R700_D2GRPH_SECONDARY_SURFACE_ADDRESS_HIGH	0x611c

/* PCIE link stuff */
#define R700_PCIE_LC_TRAINING_CNTL				0xa1 /* PCIE_P */
#define R700_PCIE_LC_LINK_WIDTH_CNTL			0xa2 /* PCIE_P */
#define 	R700_LC_LINK_WIDTH_SHIFT			0
#define 	R700_LC_LINK_WIDTH_MASK				0x7
#define 	R700_LC_LINK_WIDTH_X0				0
#define 	R700_LC_LINK_WIDTH_X1				1
#define 	R700_LC_LINK_WIDTH_X2				2
#define 	R700_LC_LINK_WIDTH_X4				3
#define 	R700_LC_LINK_WIDTH_X8				4
#define 	R700_LC_LINK_WIDTH_X16				6
#define 	R700_LC_LINK_WIDTH_RD_SHIFT			4
#define 	R700_LC_LINK_WIDTH_RD_MASK			0x70
#define 	R700_LC_RECONFIG_ARC_MISSING_ESCAPE	(1 << 7)
#define 	R700_LC_RECONFIG_NOW				(1 << 8)
#define 	R700_LC_RENEGOTIATION_SUPPORT		(1 << 9)
#define 	R700_LC_RENEGOTIATE_EN				(1 << 10)
#define 	R700_LC_SHORT_RECONFIG_EN			(1 << 11)
#define 	R700_LC_UPCONFIGURE_SUPPORT			(1 << 12)
#define 	R700_LC_UPCONFIGURE_DIS				(1 << 13)
#define R700_PCIE_LC_SPEED_CNTL					0xa4 /* PCIE_P */
#define 	R700_LC_GEN2_EN_STRAP				(1 << 0)
#define 	R700_LC_TARGET_LINK_SPEED_OVERRIDE_EN (1 << 1)
#define 	R700_LC_FORCE_EN_HW_SPEED_CHANGE	(1 << 5)
#define 	R700_LC_FORCE_DIS_HW_SPEED_CHANGE	(1 << 6)
#define 	R700_LC_SPEED_CHANGE_ATTEMPTS_ALLOWED_MASK (0x3 << 8)
#define 	R700_LC_SPEED_CHANGE_ATTEMPTS_ALLOWED_SHIFT 3
#define 	R700_LC_CURRENT_DATA_RATE			(1 << 11)
#define 	R700_LC_VOLTAGE_TIMER_SEL_MASK		(0xf << 14)
#define 	R700_LC_CLR_FAILED_SPD_CHANGE_CNT	(1 << 21)
#define 	R700_LC_OTHER_SIDE_EVER_SENT_GEN2	(1 << 23)
#define 	R700_LC_OTHER_SIDE_SUPPORTS_GEN2	(1 << 24)
#define R700_MM_CFGREGS_CNTL					0x544c
#define 	R700_MM_WR_TO_CFG_EN				(1 << 3)
#define R700_LINK_CNTL2							0x88 /* F0 */
#define 	R700_TARGET_LINK_SPEED_MASK			(0xf << 0)
#define 	R700_SELECTABLE_DEEMPHASIS			(1 << 6)


#endif /* R700_H */