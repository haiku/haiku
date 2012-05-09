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
	uint16 clockDiv
		= (baud > BOARD_UART_CLOCK / 16) ? 8 : 4;
	uint16 baudDivisor = BOARD_UART_CLOCK * clockDiv / baud;
		// TODO: Round to closest baud divisor
	uint16 lcr = PL01x_LCRH_WLEN_8;

	// Disable everything
	unsigned char originalCR
		= ReadUart(PL011_CR);
	WriteUart(PL011_CR, 0);

	// Set baud divisor
	WriteUart(PL011_FBRD, baudDivisor & 0x3f);
	WriteUart(PL011_IBRD, baudDivisor >> 6);

	// Set LCR
	WriteUart(PL011_LCRH, lcr);

	// Disable auto RTS / CTS in original CR
	originalCR &= ~(PL011_CR_CTSEN | PL011_CR_RTSEN);

	WriteUart(PL011_CR, originalCR);
}


void
UartPL011::InitEarly()
{
	// Perform special hardware UART configuration
	// Raspberry Pi: Early setup handled by gpio_init in platform code
}


void
UartPL011::Init()
{
	// TODO: Enable clock producer?
	// TODO: Clear pending error and receive interrupts

	// Provoke TX FIFO into asserting
	uint32 cr = PL011_CR_UARTEN | PL011_CR_TXE | PL011_IFLS;
	WriteUart(PL011_CR, cr);
	WriteUart(PL011_FBRD, 0);
	WriteUart(PL011_IBRD, 1);

	// TODO: For arm vendor, st different rx vs tx
	WriteUart(PL011_LCRH, 0);

	WriteUart(PL01x_DR, 0);

	while (ReadUart(PL01x_FR) & PL01x_FR_BUSY);
		// Wait for xmit
}


int
UartPL011::PutChar(char c)
{
	WriteUart(PL01x_DR, (unsigned int)c);

	while (ReadUart(PL01x_FR) & PL01x_FR_TXFF);
		// wait for the last char to get out

	return 0;
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
