/*
 * Copyright 2009
 * Distributed under the terms of the MIT License.
 */
#ifndef _BOARD_VERDEX_BOARD_CONFIG_H
#define _BOARD_VERDEX_BOARD_CONFIG_H


#define BOARD_NAME_PRETTY "Gumstix Verdex"

#define BOARD_CPU_TYPE_PXA 1
#define BOARD_CPU_PXA270 1

#include <arch/arm/pxa270.h>

// UART Settings
#define BOARD_UART_8250 1

#define BOARD_UART1_BASE FFUART_BASE
#define BOARD_UART2_BASE BTUART_BASE
#define BOARD_UART3_BASE STUART_BASE

#define BOARD_UART_DEBUG BOARD_UART1_BASE

#define BOARD_UART_CLOCK 48000000
	// 48MHz (APLL96/2)


#endif /* _BOARD_VERDEX_BOARD_CONFIG_H */
