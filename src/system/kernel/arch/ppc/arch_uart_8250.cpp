/*
 * Copyright 2012, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <debug.h>
#include <arch/generic/debug_uart_8250.h>
//#include <board_config.h>


// we shouldn't need special setup, we use plain MMIO.

DebugUART8250 *arch_get_uart_8250(addr_t base, int64 clock)
{
	static char buffer[sizeof(DebugUART8250)];
	DebugUART8250 *uart = new(buffer) DebugUART8250(base, clock);
	return uart;
}

