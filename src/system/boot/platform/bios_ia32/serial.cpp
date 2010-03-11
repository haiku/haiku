/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "serial.h"

#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>

#include <string.h>


//#define ENABLE_SERIAL
	// define this to always enable serial output


enum serial_register_offsets {
	SERIAL_TRANSMIT_BUFFER		= 0,
	SERIAL_RECEIVE_BUFFER		= 0,
	SERIAL_DIVISOR_LATCH_LOW	= 0,
	SERIAL_DIVISOR_LATCH_HIGH	= 1,
	SERIAL_FIFO_CONTROL			= 2,
	SERIAL_LINE_CONTROL			= 3,
	SERIAL_MODEM_CONTROL		= 4,
	SERIAL_LINE_STATUS			= 5,
	SERIAL_MODEM_STATUS			= 6,
};

static const uint32 kSerialBaudRate = 115200;

static int32 sSerialEnabled = 0;
static uint16 sSerialBasePort = 0x3f8;

static void
serial_putc(char c)
{
	// wait until the transmitter empty bit is set
	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x20) == 0)
		asm volatile ("pause;");

	out8(c, sSerialBasePort + SERIAL_TRANSMIT_BUFFER);
}


extern "C" void
serial_puts(const char* string, size_t size)
{
	if (sSerialEnabled <= 0)
		return;

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
#ifdef ENABLE_SERIAL
	sSerialEnabled = 0;
#else
	sSerialEnabled--;
#endif
}


extern "C" void
serial_enable(void)
{
	sSerialEnabled++;
}


extern "C" void
serial_init(void)
{
	// copy the base ports of the optional 4 serial ports to the kernel args
	// 0x0000:0x0400 is the location of that information in the BIOS data
	// segment
	uint16* ports = (uint16*)0x400;
	memcpy(gKernelArgs.platform_args.serial_base_ports, ports,
		sizeof(uint16) * MAX_SERIAL_PORTS);

	// only use the port if we could find one, else use the standard port
	if (gKernelArgs.platform_args.serial_base_ports[0] != 0)
		sSerialBasePort = gKernelArgs.platform_args.serial_base_ports[0];

	uint16 divisor = uint16(115200 / kSerialBaudRate);

	out8(0x80, sSerialBasePort + SERIAL_LINE_CONTROL);
		// set divisor latch access bit
	out8(divisor & 0xf, sSerialBasePort + SERIAL_DIVISOR_LATCH_LOW);
	out8(divisor >> 8, sSerialBasePort + SERIAL_DIVISOR_LATCH_HIGH);
	out8(3, sSerialBasePort + SERIAL_LINE_CONTROL);
		// 8N1

#ifdef ENABLE_SERIAL
	serial_enable();
#endif
}
