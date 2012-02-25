/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_GPU_H
#define RADEON_HD_GPU_H


#include "accelerant.h"

#include <video_configuration.h>


#define HDP_REG_COHERENCY_FLUSH_CNTL 0x54A0
#define HDP_NONSURFACE_BASE			0x2C04
#define HDP_NONSURFACE_INFO			0x2C08
#define HDP_NONSURFACE_SIZE			0x2C0C


// GPU Control registers. These are combined as
// the registers exist on all models, some flags
// are different though and are commented as such
#define CP_ME_CNTL					0x86D8
#define     CP_ME_HALT				(1 << 28)
#define     CP_PFP_HALT				(1 << 26)
#define CP_ME_RAM_DATA				0xC160
#define CP_ME_RAM_RADDR				0xC158
#define CP_ME_RAM_WADDR				0xC15C
#define CP_MEQ_THRESHOLDS			0x8764
#define     STQ_SPLIT(x)			((x) << 0)
#define CP_PERFMON_CNTL				0x87FC
#define CP_PFP_UCODE_ADDR			0xC150
#define CP_PFP_UCODE_DATA			0xC154
#define CP_QUEUE_THRESHOLDS			0x8760
#define     ROQ_IB1_START(x)		((x) << 0)
#define     ROQ_IB2_START(x)		((x) << 8)
#define CP_RB_BASE					0xC100
#define CP_RB_CNTL					0xC104
#define     RB_BUFSZ(x)				((x) << 0)
#define     RB_BLKSZ(x)				((x) << 8)
#define     RB_NO_UPDATE			(1 << 27)
#define     RB_RPTR_WR_ENA			(1 << 31)
#define     BUF_SWAP_32BIT			(2 << 16)
#define CP_RB_RPTR					0x8700
#define CP_RB_RPTR_ADDR				0xC10C
#define     RB_RPTR_SWAP(x)			((x) << 0)
#define CP_RB_RPTR_ADDR_HI			0xC110
#define CP_RB_RPTR_WR				0xC108
#define CP_RB_WPTR					0xC114
#define CP_RB_WPTR_ADDR				0xC118
#define CP_RB_WPTR_ADDR_HI			0xC11C
#define CP_RB_WPTR_DELAY			0x8704
#define CP_SEM_WAIT_TIMER			0x85BC
#define CP_DEBUG					0xC1FC

#define	NI_GRBM_CNTL				0x8000
#define		GRBM_READ_TIMEOUT(x)	((x) << 0)
#define	GRBM_STATUS					0x8010
#define		CMDFIFO_AVAIL_MASK		0x0000000F
#define		RING2_RQ_PENDING		(1 << 4)
#define		SRBM_RQ_PENDING			(1 << 5)
#define		RING1_RQ_PENDING		(1 << 6)
#define		CF_RQ_PENDING			(1 << 7)
#define		PF_RQ_PENDING			(1 << 8)
#define		GDS_DMA_RQ_PENDING		(1 << 9)
#define		GRBM_EE_BUSY			(1 << 10)
#define		SX_CLEAN				(1 << 11) // ni
#define		VC_BUSY					(1 << 11) // r600
#define		DB_CLEAN				(1 << 12)
#define		CB_CLEAN				(1 << 13)
#define		TA_BUSY 				(1 << 14)
#define		GDS_BUSY 				(1 << 15)
#define		VGT_BUSY_NO_DMA			(1 << 16)
#define		VGT_BUSY				(1 << 17)
#define		IA_BUSY_NO_DMA			(1 << 18) // ni
#define		TA03_BUSY				(1 << 18) // r600
#define		IA_BUSY					(1 << 19) // ni
#define		TC_BUSY					(1 << 19) // r600
#define		SX_BUSY 				(1 << 20)
#define		SH_BUSY 				(1 << 21)
#define		SPI_BUSY				(1 << 22) // AKA SPI03_BUSY r600
#define		SMX_BUSY				(1 << 23)
#define		SC_BUSY 				(1 << 24)
#define		PA_BUSY 				(1 << 25)
#define		DB_BUSY 				(1 << 26) // AKA DB03_BUSY r600
#define		CR_BUSY					(1 << 27)
#define		CP_COHERENCY_BUSY      	(1 << 28)
#define		CP_BUSY 				(1 << 29)
#define		CB_BUSY 				(1 << 30)
#define		GUI_ACTIVE				(1 << 31)
#define	GRBM_STATUS2				0x8014	// AKA GRBM_STATUS_SE0 ON NI
#define     CR_CLEAN				(1 << 0)
#define     SMX_CLEAN				(1 << 1)
#define     SPI0_BUSY				(1 << 8)
#define     SPI1_BUSY				(1 << 9)
#define     SPI2_BUSY				(1 << 10)
#define     SPI3_BUSY				(1 << 11)
#define     TA0_BUSY				(1 << 12)
#define     TA1_BUSY				(1 << 13)
#define     TA2_BUSY				(1 << 14)
#define     TA3_BUSY				(1 << 15)
#define     DB0_BUSY				(1 << 16)
#define     DB1_BUSY				(1 << 17)
#define     DB2_BUSY				(1 << 18)
#define     DB3_BUSY				(1 << 19)
#define     CB0_BUSY				(1 << 20)
#define     CB1_BUSY				(1 << 21)
#define     CB2_BUSY				(1 << 22)
#define     CB3_BUSY				(1 << 23)
#define	NI_GRBM_STATUS_SE1			0x8018
#define		SE_SX_CLEAN				(1 << 0)
#define		SE_DB_CLEAN				(1 << 1)
#define		SE_CB_CLEAN				(1 << 2)
#define		SE_VGT_BUSY				(1 << 23)
#define		SE_PA_BUSY				(1 << 24)
#define		SE_TA_BUSY				(1 << 25)
#define		SE_SX_BUSY				(1 << 26)
#define		SE_SPI_BUSY				(1 << 27)
#define		SE_SH_BUSY				(1 << 28)
#define		SE_SC_BUSY				(1 << 29)
#define		SE_DB_BUSY				(1 << 30)
#define		SE_CB_BUSY				(1 << 31)
#define	GRBM_SOFT_RESET				0x8020
#define SRBM_STATUS					0x0E50
#define		RLC_RQ_PENDING			(1 << 3)
#define		RCU_RQ_PENDING			(1 << 4)
#define		GRBM_RQ_PENDING			(1 << 5)
#define		HI_RQ_PENDING			(1 << 6)
#define		IO_EXTERN_SIGNAL		(1 << 7)
#define		VMC_BUSY				(1 << 8)
#define		MCB_BUSY				(1 << 9)
#define		MCDZ_BUSY				(1 << 10)
#define		MCDY_BUSY				(1 << 11)
#define		MCDX_BUSY				(1 << 12)
#define		MCDW_BUSY				(1 << 13)
#define		SEM_BUSY				(1 << 14)
#define		SRBM_STATUS__RLC_BUSY	(1 << 15)
#define		PDMA_BUSY				(1 << 16)
#define		IH_BUSY					(1 << 17)
#define		CSC_BUSY				(1 << 20)
#define		CMC7_BUSY				(1 << 21)
#define		CMC6_BUSY				(1 << 22)
#define		CMC5_BUSY				(1 << 23)
#define		CMC4_BUSY				(1 << 24)
#define		CMC3_BUSY				(1 << 25)
#define		CMC2_BUSY				(1 << 26)
#define		CMC1_BUSY				(1 << 27)
#define		CMC0_BUSY				(1 << 28)
#define		BIF_BUSY				(1 << 29)
#define		IDCT_BUSY				(1 << 30)
#define SRBM_SOFT_RESET				0x0E60
#define		SOFT_RESET_CP			(1 << 0)
#define		SOFT_RESET_CB			(1 << 1)
#define		SOFT_RESET_CR			(1 << 2)
#define		SOFT_RESET_DB			(1 << 3)
#define		SOFT_RESET_GDS			(1 << 4)
#define		SOFT_RESET_PA			(1 << 5)
#define		SOFT_RESET_SC			(1 << 6)
#define		SOFT_RESET_SMX			(1 << 7)
#define		SOFT_RESET_SPI			(1 << 8)
#define		SOFT_RESET_SH			(1 << 9)
#define		SOFT_RESET_SX			(1 << 10)
#define		SOFT_RESET_TC			(1 << 11)
#define		SOFT_RESET_TA			(1 << 12)
#define		SOFT_RESET_VC			(1 << 13)
#define		SOFT_RESET_VGT			(1 << 14)
#define		SOFT_RESET_IA			(1 << 15)


status_t radeon_gpu_reset();
void radeon_gpu_mc_halt(struct gpu_state *gpuState);
void radeon_gpu_mc_resume(struct gpu_state *gpuState);
status_t radeon_gpu_mc_idlewait();
status_t radeon_gpu_mc_setup();
status_t radeon_gpu_irq_setup();
status_t radeon_gpu_ss_disable();


#endif
