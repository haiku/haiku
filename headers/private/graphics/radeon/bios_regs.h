/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	BIOS scratch registers
*/

#ifndef _BIOS_REGS_H
#define _BIOS_REGS_H

#define RADEON_BIOS_0_SCRATCH               0x0010
#define RADEON_BIOS_1_SCRATCH               0x0014
#define RADEON_BIOS_2_SCRATCH               0x0018
#define RADEON_BIOS_3_SCRATCH               0x001c
#define RADEON_BIOS_4_SCRATCH               0x0020
#define RADEON_BIOS_5_SCRATCH               0x0024
#define RADEON_BIOS_6_SCRATCH               0x0028
#define RADEON_BIOS_7_SCRATCH               0x002c

#define RADEON_TEST_DEBUG_CNTL					0x0120
#		define RADEON_TEST_DEBUG_CNTL_OUT_EN	(1 << 0)
#define RADEON_TEST_DEBUG_MUX					0x0124
#define RADEON_TEST_DEBUG_OUT					0x012c

#endif
