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


UartPL011::UartPL011(addr_t base)
	:
	fUARTEnabled(true),
	fUARTBase(base)
{
}


UartPL011::~UartPL011()
{
}


void
UartPL011::WriteUart(uint32 reg, unsigned char data)
{
	*(volatile unsigned char *)(fUARTBase + reg) = data;
}


unsigned char
UartPL011::ReadUart(uint32 reg)
{
	return *(volatile unsigned char *)(fUARTBase + reg);
}


void
UartPL011::InitPort(uint32 baud)
{
	// Disable UART
	Disable();

	// Calculate baud divisor
	uint16 baudDivisor = BOARD_UART_CLOCK / (16 * baud);
	uint16 baudFractional = BOARD_UART_CLOCK % (16 * baud);

	// Set baud divisor
	WriteUart(PL011_IBRD, baudDivisor);
	WriteUart(PL011_FBRD, baudFractional);

	// Set LCR
	WriteUart(PL011_LCRH, PL01x_LCRH_WLEN_8);

	// Enable UART
	Enable();
}


void
UartPL011::InitEarly()
{
	// Perform special hardware UART configuration
	// Raspberry Pi: Early setup handled by gpio_init in platform code
}


void
UartPL011::Enable()
{
	// TODO: Enable clock producer?

	unsigned char cr = PL011_CR_UARTEN;
		// Enable UART
	cr |= PL011_CR_TXE; // | PL011_CR_RXE;
		// Enable TX and RX
	WriteUart(PL011_CR, cr);

	// TODO: For arm vendor, st different rx vs tx
	// WriteUart(PL011_LCRH, 0);

	WriteUart(PL01x_DR, 0);

	while (ReadUart(PL01x_FR) & PL01x_FR_BUSY);
		// Wait for xmit

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
		WriteUart(PL01x_DR, (unsigned int)c);
		while (ReadUart(PL01x_FR) & PL01x_FR_TXFF);
			// wait for the last char to get out
		return 0;
	}

	return -1;
}


/* returns -1 if no data available */
int
UartPL011::GetChar(bool wait)
{
	#warning ARM Amba PL011 UART incomplete
	return -1;
}


void
UartPL011::FlushTx()
{
	while (ReadUart(PL01x_FR) & PL01x_FR_TXFF);
		// wait for the last char to get out
}


void
UartPL011::FlushRx()
{
	#warning ARM Amba PL011 UART incomplete
}
