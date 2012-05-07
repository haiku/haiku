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
uart_base_debug(void)
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


uart_info*
uart_create(void)
{
	struct uart_info* uart;

	uart = (uart_info*)malloc(sizeof(uart_info));
	uart->base = uart_base_debug();

	#if defined(BOARD_UART_8250)
	uart->init = uart_8250_init;
	uart->init_early = uart_8250_init_early;
	uart->init_port = uart_8250_init_port;
	uart->putchar = uart_8250_putchar;
	uart->getchar = uart_8250_getchar;
	uart->flush_tx = uart_8250_flush_tx;
	uart->flush_rx = uart_8250_flush_rx;
	#elif defined(BOARD_UART_AMBA_PL011)
	uart->init = uart_pl011_init;
	uart->init_early = uart_pl011_init_early;
	uart->init_port = uart_pl011_init_port;
	uart->putchar = uart_pl011_putchar;
	uart->getchar = uart_pl011_getchar;
	uart->flush_tx = uart_pl011_flush_tx;
	uart->flush_rx = uart_pl011_flush_rx;
	#else
	#error Unknown UART Type (or no UART provided)
	#endif

	return uart;
}


void
uart_destroy(uart_info* uart)
{
	free(uart);
}
