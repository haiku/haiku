/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _BOARD_RPI2_BOARD_CONFIG_H
#define _BOARD_RPI2_BOARD_CONFIG_H


#define BOARD_NAME_PRETTY "Raspberry Pi 2"

#define BOARD_CPU_TYPE_ARM7 1
#define BOARD_CPU_BCM2836 1

#include <arch/arm/bcm283X.h>

#define DEVICE_BASE BCM2836_PERIPHERAL_BASE
#define DEVICE_SIZE 0xFFFFFF

#define VECT_BASE 0xFFFF0000
#define VECT_SIZE SIZE_4K

#define SDRAM_BASE      BCM283X_SDRAM_BASE
#define SDRAM_SIZE      0x4000000
	// 64Mb

// UART Settings
#define BOARD_UART_PL011 1

#define BOARD_UART1_BASE UART0_BASE
	// PL011 UART
#define BOARD_UART2_BASE UART1_BASE + 0x40
	// miniUART
#define BOARD_UART3_BASE 0
	// N/A

#define BOARD_UART_DEBUG DEVICE_BASE + BOARD_UART1_BASE

#define BOARD_UART_CLOCK 3000000
	/* 3Mhz */


#endif /* _BOARD_RPI2_BOARD_CONFIG_H */
