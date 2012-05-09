/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include <arch/arm/reg.h>
#include <arch/arm/uart.h>
#include <board_config.h>
#include <debug.h>
#include <stdlib.h>
//#include <target/debugconfig.h>


#define DEBUG_UART BOARD_UART_DEBUG


addr_t
uart_base_debug()
{
	return DEBUG_UART;
}


addr_t
uart_base_port(int port)
{
	switch (port) {
		case 1:
			return BOARD_UART1_BASE;
		case 2:
			return BOARD_UART2_BASE;
		case 3:
			return BOARD_UART3_BASE;
	}

	return uart_base_debug();
}
