/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	BusMemory Control registers
*/

#ifndef _MEMCNTRL_REGS_H
#define _MEMCNTRL_REGS_H

#define RADEON_AGP_BASE                     0x0170
#define RADEON_MEM_CNTL                     0x0140

#define RADEON_MC_AGP_LOCATION              0x014c
#define RADEON_MC_FB_LOCATION               0x0148
#define RADEON_MEM_INIT_LAT_TIMER           0x0154
#define RADEON_MEM_SDRAM_MODE_REG           0x0158
#       define RADEON_MEM_CFG_TYPE_MASK     (1 << 30)
#       define RADEON_MEM_CFG_SDR           (0 << 30)
#       define RADEON_MEM_CFG_DDR           (1 << 30)
#define	RADEON_GC_NB_TOM					0x015c
#define	RADEON_DISPLAY_BASE_ADDRESS			0x023c
#define	RADEON_CRTC2_DISPLAY_BASE_ADDRESS	0x033c
#define	RADEON_OV0_BASE_ADDRESS				0x043c

#endif
