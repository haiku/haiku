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


static inline void
write_pl011(addr_t base, uint reg, unsigned char data)
{
	*(volatile unsigned char *)(base + reg) = data;
}


static inline unsigned char
read_pl011(addr_t base, uint reg)
{
	return *(volatile unsigned char *)(base + reg);
}


void
uart_pl011_init_port(addr_t base, uint baud)
{
	uint16 clockDiv
		= (baud > BOARD_UART_CLOCK / 16) ? 8 : 4;
	uint16 baudDivisor = BOARD_UART_CLOCK * clockDiv / baud;
		// TODO: Round to closest baud divisor
	uint16 lcr = PL01x_LCRH_WLEN_8;

	// Disable everything
	unsigned char originalCR
		= read_pl011(base, PL011_CR);
	write_pl011(base, PL011_CR, 0);

	// Set baud divisor
	write_pl011(base, PL011_FBRD, baudDivisor & 0x3f);
	write_pl011(base, PL011_IBRD, baudDivisor >> 6);

	// Set LCR
	write_pl011(base, PL011_LCRH, lcr);

	// Disable auto RTS / CTS in original CR
	originalCR &= ~(PL011_CR_CTSEN | PL011_CR_RTSEN);

	write_pl011(base, PL011_CR, originalCR);
}


void
uart_pl011_init_early(void)
{
	// Perform special hardware UART configuration
}


void
uart_pl011_init(addr_t base)
{
	// TODO: Enable clock producer?
	// TODO: Clear pending error and receive interrupts

	// Provoke TX FIFO into asserting
	unsigned char cr = PL011_CR_UARTEN | PL011_CR_TXE | PL011_IFLS;
	write_pl011(base, PL011_CR, cr);
	write_pl011(base, PL011_FBRD, 0);
	write_pl011(base, PL011_IBRD, 1);

	// TODO: For arm vendor, st different rx vs tx
	write_pl011(base, PL011_LCRH, 0);

	write_pl011(base, PL01x_DR, 0);

	while (read_pl011(base, PL01x_FR) & PL01x_FR_BUSY);
		// Wait for xmit
}


int
uart_pl011_putchar(addr_t base, char c)
{
	write_pl011(base, PL01x_DR, (unsigned int)c);

	while (read_pl011(base, PL01x_FR) & PL01x_FR_TXFF);
		// wait for the last char to get out

	return 0;
}


/* returns -1 if no data available */
int
uart_pl011_getchar(addr_t base, bool wait)
{
	#warning ARM Amba PL011 UART incomplete
	return -1;
}


void
uart_pl011_flush_tx(addr_t base)
{
	while (read_pl011(base, PL01x_FR) & PL01x_FR_TXFF);
		// wait for the last char to get out
}


void
uart_pl011_flush_rx(addr_t base)
{
	#warning ARM Amba PL011 UART incomplete
}
