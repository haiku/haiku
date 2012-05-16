/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include <debug.h>
#include <arch/arm/reg.h>
#include <arch/arm/uart.h>
#include <board_config.h>
//#include <target/debugconfig.h>


static void
barrier()
{
	asm volatile ("" : : : "memory");
}


UartPL011::UartPL011(addr_t base)
	:
	fUARTEnabled(true),
	fUARTBase(base)
{
	barrier();

	// ** Loopback test
	uint32 cr = PL01x_CR_UARTEN;
		// Enable UART
	cr |= PL011_CR_TXE;
		// Enable TX
	cr |= PL011_CR_LBE;
		// Enable Loopback mode
	WriteUart(PL011_CR, cr);

	WriteUart(PL011_FBRD, 0);
	WriteUart(PL011_IBRD, 1);
	WriteUart(PL011_LCRH, 0); // TODO: ST is different tx, rx lcr

	// Write a 0 to the port and wait for confim..
	WriteUart(PL01x_DR, 0);

	while (ReadUart(PL01x_FR) & PL01x_FR_BUSY)
		barrier();

	// ** Disable loopback, enable uart
	cr = PL01x_CR_UARTEN | PL011_CR_RXE | PL011_CR_TXE;
	WriteUart(PL011_CR, cr);

	// ** Clear interrupts
	WriteUart(PL011_ICR, PL011_OEIS | PL011_BEIS
		| PL011_PEIS | PL011_FEIS);

	// ** Disable interrupts
	WriteUart(PL011_IMSC, 0);
}


UartPL011::~UartPL011()
{
}


void
UartPL011::WriteUart(uint32 reg, uint32 data)
{
	*(volatile uint32*)(fUARTBase + reg) = data;
}


uint32
UartPL011::ReadUart(uint32 reg)
{
	return *(volatile uint32*)(fUARTBase + reg);
}


void
UartPL011::InitPort(uint32 baud)
{
	// Calculate baud divisor
	uint32 baudDivisor = BOARD_UART_CLOCK / (16 * baud);
	uint32 remainder = BOARD_UART_CLOCK % (16 * baud);
	uint32 baudFractional = ((8 * remainder) / baud >> 1)
		+ ((8 * remainder) / baud & 1);

	// Disable UART
	Disable();

	// Set baud divisor
	WriteUart(PL011_IBRD, baudDivisor);
	WriteUart(PL011_FBRD, baudFractional);

	// Set LCR 8n1, enable fifo
	WriteUart(PL011_LCRH, PL01x_LCRH_WLEN_8 | PL01x_LCRH_FEN);

	// Enable UART
	Enable();
}


void
UartPL011::InitEarly()
{
	// Perform special hardware UART configuration
}


void
UartPL011::Enable()
{
	uint32 cr = PL01x_CR_UARTEN;
		// Enable UART
	cr |= PL011_CR_TXE | PL011_CR_RXE;
		// Enable TX and RX

	WriteUart(PL011_CR, cr);

	fUARTEnabled = true;
}


void
UartPL011::Disable()
{
	// Disable everything
	WriteUart(PL011_CR, 0);
	fUARTEnabled = false;
}


int
UartPL011::PutChar(char c)
{
	if (fUARTEnabled == true) {
		// Wait until there is room in fifo
		while ((ReadUart(PL01x_FR) & PL01x_FR_TXFF) != 0)
			barrier();

		WriteUart(PL01x_DR, c);
		return 0;
	}

	return -1;
}


int
UartPL011::GetChar(bool wait)
{
	#warning ARM Amba PL011 UART incomplete
	return -1;
}


void
UartPL011::FlushTx()
{
	// Wait until transmit fifo empty
	while ((ReadUart(PL01x_FR) & PL011_FR_TXFE) == 0)
		barrier();
}


void
UartPL011::FlushRx()
{
	// Wait until receive fifo empty
	while ((ReadUart(PL01x_FR) & PL01x_FR_RXFE) == 0)
		barrier();
}
