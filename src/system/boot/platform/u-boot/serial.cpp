/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2012-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "serial.h"

#include <arch/generic/debug_uart_8250.h>

#if defined(__ARM__)
#include <arch/arm/arch_uart_pl011.h>
#endif

#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>
#include <new>
#include <string.h>

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

#include "fdt_serial.h"


DebugUART* gUART;

static int32 sSerialEnabled = 0;
static char sBuffer[16384];
static uint32 sBufferPosition;


static void
serial_putc(char c)
{
	if (gUART == NULL || sSerialEnabled <= 0)
		return;

	gUART->PutChar(c);
}


extern "C" int
serial_getc(bool wait)
{
	if (gUART == NULL || sSerialEnabled <= 0)
		return 0;

	return gUART->GetChar(wait);
}


extern "C" void
serial_puts(const char* string, size_t size)
{
	if (sSerialEnabled <= 0)
		return;

	if (sBufferPosition + size < sizeof(sBuffer)) {
		memcpy(sBuffer + sBufferPosition, string, size);
		sBufferPosition += size;
	}

	while (size-- != 0) {
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
	sSerialEnabled = 0;
}


extern "C" void
serial_enable(void)
{
	/* should already be initialized by U-Boot */
	gUART->InitEarly();
	gUART->InitPort(115200);
	sSerialEnabled++;
}


extern "C" void
serial_cleanup(void)
{
	if (sSerialEnabled <= 0)
		return;

	gKernelArgs.debug_output = kernel_args_malloc(sBufferPosition);
	if (gKernelArgs.debug_output != NULL) {
		memcpy(gKernelArgs.debug_output, sBuffer, sBufferPosition);
		gKernelArgs.debug_size = sBufferPosition;
	}
}


extern "C" void
serial_init(const void *fdt)
{
	// first try with hints from the FDT
	gUART = debug_uart_from_fdt(fdt);

	// Do we can some kind of direct fallback here
	// (aka, guess arch_get_uart_pl011 or arch_get_uart_8250?)
	if (gUART == NULL)
		return;

	serial_enable();
}
