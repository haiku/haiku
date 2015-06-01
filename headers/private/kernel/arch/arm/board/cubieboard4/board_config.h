/*
 * Copyright 2012-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _BOARD_SUN9I_BOARD_CONFIG_H
#define _BOARD_SUN9I_BOARD_CONFIG_H

// https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/arch/arm/boot/dts/sun9i-a80.dtsi
// http://linux-sunxi.org/A80/Memory_map

#define BOARD_NAME_PRETTY "Allwinner A80"

#define BOARD_CPU_TYPE_ARM7 1
#define BOARD_CPU_A80 1

//#include <arch/arm/sun9i-a80.h>

// For now we just pick up APB1 devices + CPUS
#define DEVICE_BASE 0x07000000
#define DEVICE_SIZE 0x1008FFF

#define VECT_BASE 0xFFFF0000
#define VECT_SIZE SIZE_4K

#define SDRAM_BASE 0x20000000
#define SDRAM_SIZE 0x4000000
	// 64Mb (although it is really 0x1FFFFFFFF) 

// UART Settings

// snps,dw-apb-uart?
#define BOARD_UART1_BASE 0x07000000
#define BOARD_UART2_BASE BOARD_UART1_BASE + 0x400
#define BOARD_UART3_BASE BOARD_UART2_BASE + 0x400

#define BOARD_UART_DEBUG BOARD_UART1_BASE

#define BOARD_UART_CLOCK 24000000
	/* 2.4Mhz */


#endif /* _BOARD_SUN9I_BOARD_CONFIG_H */
