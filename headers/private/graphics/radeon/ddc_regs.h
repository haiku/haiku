/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	DDC registers
*/


#ifndef _DDC_REGS_H
#define _DDC_REGS_H

#define RADEON_GPIO_VGA_DDC                 0x0060
#define RADEON_GPIO_DVI_DDC                 0x0064
#define RADEON_GPIO_MONID                   0x0068
#define RADEON_GPIO_CRT2_DDC                0x006c
#       define RADEON_GPIO_A_0        (1 <<  0)
#       define RADEON_GPIO_A_1        (1 <<  1)
#       define RADEON_GPIO_Y_0        (1 <<  8)
#       define RADEON_GPIO_Y_1        (1 <<  9)
#       define RADEON_GPIO_Y_SHIFT_0  8
#       define RADEON_GPIO_Y_SHIFT_1  9
#       define RADEON_GPIO_EN_0       (1 << 16)
#       define RADEON_GPIO_EN_1       (1 << 17)
#       define RADEON_GPIO_EN_SHIFT_0 16
#       define RADEON_GPIO_EN_SHIFT_1 17

#define RADEON_DVI_I2C_CNTL_0               0x02e0
#define RADEON_DVI_I2C_CNTL_1               0x02e4
#define RADEON_DVI_I2C_DATA					0x02e8

#endif

