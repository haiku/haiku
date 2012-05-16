/*
 * Copyright 2004-2008, Axel D??rfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "serial.h"
#include "uart.h"

#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>

#include <string.h>


UartPL011 gLoaderUART(uart_base_debug());


static bool sSerialEnabled = false;


static void
serial_putc(char c)
{
	gLoaderUART.PutChar(c);
}


extern "C" int
serial_getc(bool wait)
{
	return gLoaderUART.GetChar(wait);
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

	gLoaderUART.Disable();
}


extern "C" void
serial_enable(void)
{
	gLoaderUART.Enable();
	sSerialEnabled = true;
}


extern "C" void
serial_cleanup(void)
{
	sSerialEnabled = false;
	gLoaderUART.FlushTx();
}


extern "C" void
serial_init(void)
{
	gLoaderUART.InitEarly();
	gLoaderUART.InitPort(9600);

	serial_enable();

	serial_puts("\n\n********************\n", 23);
	serial_puts("Haiku serial startup\n", 21);
}
