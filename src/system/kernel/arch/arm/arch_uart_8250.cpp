/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 */

#include <debug.h>
#include <arch/generic/debug_uart_8250.h>
#include <new>
#include <board_config.h>


class ArchUART8250 : public DebugUART8250 {
public:
							ArchUART8250(addr_t base, int64 clock);
							~ArchUART8250();
	void					InitEarly();

	// ARM MMIO: on ARM the UART regs are aligned on 32bit
	virtual void			Out8(int reg, uint8 value);
	virtual uint8			In8(int reg);
};


ArchUART8250::ArchUART8250(addr_t base, int64 clock)
	: DebugUART8250(base, clock)
{
}


ArchUART8250::~ArchUART8250()
{
}


void
ArchUART8250::InitEarly()
{
	// Perform special hardware UART configuration

#if BOARD_CPU_OMAP3
	/* UART1 */
	RMWREG32(CM_FCLKEN1_CORE, 13, 1, 1);
	RMWREG32(CM_ICLKEN1_CORE, 13, 1, 1);

	/* UART2 */
	RMWREG32(CM_FCLKEN1_CORE, 14, 1, 1);
	RMWREG32(CM_ICLKEN1_CORE, 14, 1, 1);

	/* UART3 */
	RMWREG32(CM_FCLKEN_PER, 11, 1, 1);
	RMWREG32(CM_ICLKEN_PER, 11, 1, 1);
#else
#warning INTITIALIZE UART!!!!!
#endif
}


void
ArchUART8250::Out8(int reg, uint8 value)
{
	*((uint8 *)Base() + reg * sizeof(uint32)) = value;
}


uint8
ArchUART8250::In8(int reg)
{
	return *((uint8 *)Base() + reg * sizeof(uint32));
}


DebugUART8250 *arch_get_uart_8250(addr_t base, int64 clock)
{
	static char buffer[sizeof(ArchUART8250)];
	ArchUART8250 *uart = new(buffer) ArchUART8250(base, clock);
	return uart;
}
