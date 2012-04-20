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

#define BOARD_UART1_BASE UART0_BASE
#define BOARD_UART2_BASE UART1_BASE
#define BOARD_UART3_BASE UART1_BASE
#warning NO UART3!!!

#define BOARD_DEBUG_UART 0


#endif /* _BOARD_RASPBERRY_PI_BOARD_CONFIG_H */
