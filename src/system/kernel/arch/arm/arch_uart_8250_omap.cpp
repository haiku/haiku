/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 */


#include <arch/arm/reg.h>
#include <arch/arm/debug_uart_8250_omap.h>
#include <debug.h>
#include <omap3.h>
#include <new>


ArchUART8250Omap::ArchUART8250Omap(addr_t base, int64 clock)
	:
	DebugUART8250(base, clock)
{
}


ArchUART8250Omap::~ArchUART8250Omap()
{
}


void
ArchUART8250Omap::InitEarly()
{
	// Perform special hardware UART configuration
	/* UART1 */
	RMWREG32(CM_FCLKEN1_CORE, 13, 1, 1);
	RMWREG32(CM_ICLKEN1_CORE, 13, 1, 1);

	/* UART2 */
	RMWREG32(CM_FCLKEN1_CORE, 14, 1, 1);
	RMWREG32(CM_ICLKEN1_CORE, 14, 1, 1);

	/* UART3 */
	RMWREG32(CM_FCLKEN_PER, 11, 1, 1);
	RMWREG32(CM_ICLKEN_PER, 11, 1, 1);
}


void
ArchUART8250Omap::Out8(int reg, uint8 value)
{
	*((uint8 *)Base() + reg * sizeof(uint32)) = value;
}


uint8
ArchUART8250Omap::In8(int reg)
{
	return *((uint8 *)Base() + reg * sizeof(uint32));
}


DebugUART8250*
arch_get_uart_8250_omap(addr_t base, int64 clock)
{
	static char buffer[sizeof(ArchUART8250Omap)];
	ArchUART8250Omap* uart = new(buffer) ArchUART8250Omap(base, clock);
	return uart;
}
