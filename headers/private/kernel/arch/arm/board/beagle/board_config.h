/*
 * Copyright 2009
 * Distributed under the terms of the MIT License.
 */
#ifndef _BOARD_OVERO_BOARD_CONFIG_H
#define _BOARD_OVERO_BOARD_CONFIG_H


#define BOARD_NAME_PRETTY "Beagle Board"

#define BOARD_CPU_TYPE_OMAP 1
#define BOARD_CPU_OMAP3 1

#include <arch/arm/omap3.h>

// UART Settings
#define BOARD_UART_8250 1

#define BOARD_UART1_BASE OMAP_UART1_BASE
#define BOARD_UART2_BASE OMAP_UART2_BASE
#define BOARD_UART3_BASE OMAP_UART3_BASE

#define BOARD_UART_DEBUG BOARD_UART3_BASE

#define BOARD_UART_CLOCK 48000000
	// 48MHz (APLL96/2)


#endif /* _BOARD_OVERO_BOARD_CONFIG_H */
