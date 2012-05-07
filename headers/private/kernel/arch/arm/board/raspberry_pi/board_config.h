/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _BOARD_RASPBERRY_PI_BOARD_CONFIG_H
#define _BOARD_RASPBERRY_PI_BOARD_CONFIG_H


#define BOARD_NAME_PRETTY "Raspberry Pi"

#define BOARD_CPU_ARM6 1

#include <arch/arm/bcm2708.h>

// UART Settings
#define BOARD_UART_AMBA_PL011 1

#define BOARD_UART1_BASE UART0_BASE
#define BOARD_UART2_BASE UART1_BASE + 0x40
#define BOARD_UART3_BASE 0

#define BOARD_UART_DEBUG BOARD_UART2_BASE

#define BOARD_UART_CLOCK 3000000
	/* 3Mhz */


#endif /* _BOARD_RASPBERRY_PI_BOARD_CONFIG_H */
