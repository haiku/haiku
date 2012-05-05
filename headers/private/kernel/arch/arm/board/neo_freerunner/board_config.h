/*
 * Copyright 2009 François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BOARD_FREERUNNER_BOARD_CONFIG_H
#define _BOARD_FREERUNNER_BOARD_CONFIG_H


#define BOARD_NAME_PRETTY "Openmoko Neo FreeRunner"

#define BOARD_CPU_TYPE_ARM9 1
#define BOARD_CPU_ARM920T 1

#include <arch/arm/arm920t.h>

// UART Settings
#define BOARD_UART1_BASE UART0_BASE
#define BOARD_UART2_BASE UART1_BASE
#define BOARD_UART3_BASE UART2_BASE

#define BOARD_DEBUG_UART 2

#define BOARD_UART_CLOCK 48000000
	// 48MHz (APLL96/2)


#endif /* _BOARD_FREERUNNER_BOARD_CONFIG_H */
