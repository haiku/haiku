/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "serial.h"
#include "arch_uart_pl011.h"

#include "board_config.h"
#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>
#include "mmu.h"

#include <string.h>


DebugUART *gUART;


static bool sSerialEnabled = false;
extern addr_t gPeripheralBase;


static void
serial_putc(char c)
{
	gUART->PutChar(c);
}


extern "C" int
serial_getc(bool wait)
{
	return gUART->GetChar(wait);
}


extern "C" void
serial_puts(const char* string, size_t size)
{
	if (!sSerialEnabled)
		return;

	while (size-- > 0) {
		char c = string[0];

		if (c == '\n') {
			serial_putc('\r');
			serial_putc('\n');
		} else if (c != '\r')
			serial_putc(c);

		string++;
	}
}


extern "C" void
serial_disable(void)
{
	if (sSerialEnabled)
		serial_cleanup();

	gUART->Disable();
}


extern "C" void
serial_enable(void)
{
	gUART->Enable();
	sSerialEnabled = true;
}


extern "C" void
serial_cleanup(void)
{
	sSerialEnabled = false;
	gUART->FlushTx();
}


extern "C" void
serial_init(void)
{
	gUART = arch_get_uart_pl011(gPeripheralBase + BOARD_UART_DEBUG,
		BOARD_UART_CLOCK);
	gUART->InitEarly();
	gUART->InitPort(9600);

	serial_enable();

	serial_puts("\n\n********************\n", 23);
	serial_puts("Haiku serial startup\n", 21);
}
