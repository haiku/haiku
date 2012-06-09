/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 */
#ifndef _BOARD_SAM460EX_BOARD_CONFIG_H
#define _BOARD_SAM460EX_BOARD_CONFIG_H


#define BOARD_NAME_PRETTY "ACube Sam460ex"

#define BOARD_CPU_TYPE_PPC440 1
#define BOARD_CPU_PPC460EX 1

// UART Settings
// TODO: use the FDT instead of hardcoding
#define BOARD_UART1_BASE 0xef600300
#define BOARD_UART2_BASE 0
#define BOARD_UART3_BASE 0

#define BOARD_UART_DEBUG BOARD_UART1_BASE

#define BOARD_UART_CLOCK 0
	/* ?Mhz */


#endif /* _BOARD_SAM460EX_BOARD_CONFIG_H */
