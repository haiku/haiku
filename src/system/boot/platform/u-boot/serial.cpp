/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2012, Alexander von Gluck, kallisti5@unixzen.com
 * Distributed under the terms of the MIT License.
 */


#include "serial.h"

#include "uart.h"
#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>
#include <string.h>


struct uart_info* gUARTInfo;

static int32 sSerialEnabled = 0;
static char sBuffer[16384];
static uint32 sBufferPosition;


static void
serial_putc(char c)
{
	gUARTInfo->putchar(gUARTInfo->base, c);
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
	/*
	gUARTInfo->init_early();
	gUARTInfo->init(gUARTInfo->base);
	gUARTInfo->init_port(gUARTInfo->base, 9600);
	*/
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
serial_init(void)
{
	// Setup information on uart
	gUARTInfo = uart_create();

	if (gUARTInfo == NULL) {
		// TODO: Notify user somehow.
		return;
	}

	serial_enable();
}
