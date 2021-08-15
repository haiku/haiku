/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <arch/riscv64/arch_uart_sifive.h>
#include <new>


ArchUARTSifive::ArchUARTSifive(addr_t base, int64 clock)
	:
	DebugUART(base, clock)
{
}


ArchUARTSifive::~ArchUARTSifive()
{
}


void
ArchUARTSifive::InitEarly()
{
	//Regs()->ie = 0;
	//Enable();
}


void
ArchUARTSifive::Init()
{
}


void
ArchUARTSifive::InitPort(uint32 baud)
{
	uint64 quotient = (Clock() + baud - 1) / baud;

	if (quotient == 0)
		Regs()->div = 0;
	else
		Regs()->div = (uint32)(quotient - 1);
}


void
ArchUARTSifive::Enable()
{
	//Regs()->txctrl.enable = 1;
	//Regs()->rxctrl.enable = 1;
	DebugUART::Enable();
}


void
ArchUARTSifive::Disable()
{
	//Regs()->txctrl.enable = 0;
	//Regs()->rxctrl.enable = 0;
	DebugUART::Disable();
}


int
ArchUARTSifive::PutChar(char ch)
{
	// TODO: Needs to be more atomic?
	// Character drop will happen if there is one space left
	// in the FIFO and two harts race. atomic_fetch_or of
	// register, retrying if isFull.
	while (Regs()->txdata.isFull) {}
	Regs()->txdata.val = ch;
	return 0;
}


int
ArchUARTSifive::GetChar(bool wait)
{
	UARTSifiveRegs::Rxdata data;
	do {
		data.val = Regs()->rxdata.val;
	} while (!wait || data.isEmpty);

	return data.isEmpty ? -1 : data.data;
}


void
ArchUARTSifive::FlushTx()
{
}


void
ArchUARTSifive::FlushRx()
{
}


void
ArchUARTSifive::Barrier()
{
	asm volatile ("" : : : "memory");
}


ArchUARTSifive*
arch_get_uart_sifive(addr_t base, int64 clock)
{
	static char buffer[sizeof(ArchUARTSifive)];
	ArchUARTSifive* uart = new(buffer) ArchUARTSifive(base, clock);
	return uart;
}
