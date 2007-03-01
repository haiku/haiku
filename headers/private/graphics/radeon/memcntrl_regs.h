/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	BusMemory Control registers
*/

#ifndef _MEMCNTRL_REGS_H
#define _MEMCNTRL_REGS_H

#define RADEON_AGP_BASE                     0x0170
#define RADEON_MEM_CNTL                     0x0140
#       define RADEON_MEM_NUM_CHANNELS_MASK 0x01
#       define RADEON_MEM_USE_B_CH_ONLY     (1<<1)
#       define RV100_HALF_MODE              (1<<3)
#       define R300_MEM_NUM_CHANNELS_MASK   0x03
#       define R300_MEM_USE_CD_CH_ONLY      (1<<2)
#define RADEON_MC_AGP_LOCATION              0x014c
#define RADEON_MC_FB_LOCATION               0x0148
#define RADEON_MEM_INIT_LAT_TIMER           0x0154
#define RADEON_MEM_SDRAM_MODE_REG           0x0158
#       define RADEON_MEM_CFG_TYPE_MASK     (1 << 30)
#       define RADEON_MEM_CFG_SDR           (0 << 30)
#       define RADEON_MEM_CFG_DDR           (1 << 30)
#define	RADEON_NB_TOM					0x015c
#define	RADEON_DISPLAY_BASE_ADDRESS			0x023c
#define	RADEON_CRTC2_DISPLAY_BASE_ADDRESS	0x033c
#define	RADEON_OV0_BASE_ADDRESS				0x043c

#define RADEON_GRPH_BUFFER_CNTL             0x02f0
#       define RADEON_GRPH_START_REQ_MASK          (0x7f)
#       define RADEON_GRPH_START_REQ_SHIFT         0
#       define RADEON_GRPH_STOP_REQ_MASK           (0x7f<<8)
#       define RADEON_GRPH_STOP_REQ_SHIFT          8
#       define RADEON_GRPH_CRITICAL_POINT_MASK     (0x7f<<16)
#       define RADEON_GRPH_CRITICAL_POINT_SHIFT    16
#       define RADEON_GRPH_CRITICAL_CNTL           (1<<28)
#       define RADEON_GRPH_BUFFER_SIZE             (1<<29)
#       define RADEON_GRPH_CRITICAL_AT_SOF         (1<<30)
#       define RADEON_GRPH_STOP_CNTL               (1<<31)
#define RADEON_GRPH2_BUFFER_CNTL            0x03f0
#       define RADEON_GRPH2_START_REQ_MASK         (0x7f)
#       define RADEON_GRPH2_START_REQ_SHIFT         0
#       define RADEON_GRPH2_STOP_REQ_MASK          (0x7f<<8)
#       define RADEON_GRPH2_STOP_REQ_SHIFT         8
#       define RADEON_GRPH2_CRITICAL_POINT_MASK    (0x7f<<16)
#       define RADEON_GRPH2_CRITICAL_POINT_SHIFT   16
#       define RADEON_GRPH2_CRITICAL_CNTL          (1<<28)
#       define RADEON_GRPH2_BUFFER_SIZE            (1<<29)
#       define RADEON_GRPH2_CRITICAL_AT_SOF        (1<<30)
#       define RADEON_GRPH2_STOP_CNTL              (1<<31)

#endif
