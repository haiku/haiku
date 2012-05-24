/*
 * Copyright 2012, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <debug.h>
#include <arch/generic/debug_uart_8250.h>
#include <arch/ppc/arch_cpu.h>
#include <new>


class ArchUART8250 : public DebugUART8250 {
public:
							ArchUART8250(addr_t base, int64 clock);
							~ArchUART8250();

	virtual void			Barrier();
};


ArchUART8250::ArchUART8250(addr_t base, int64 clock)
	: DebugUART8250(base, clock)
{
}


ArchUART8250::~ArchUART8250()
{
}


void
ArchUART8250::Barrier()
{
	eieio();
}


DebugUART8250 *arch_get_uart_8250(addr_t base, int64 clock)
{
	static char buffer[sizeof(ArchUART8250)];
	ArchUART8250 *uart = new(buffer) ArchUART8250(base, clock);
	return uart;
}

