/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <SupportDefs.h>
#include <string.h>
#include <util/kernel_cpp.h>
#include <arch/cpu.h>
#include <boot/stage2.h>

#include "console.h"


class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);
};

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

static uint16 *sScreenBase = (uint16 *)0xb8000;
static uint32 sScreenWidth = 80;
static uint32 sScreenHeight = 25;
static uint32 sScreenOffset = 0;
static uint16 sColor = 0x0f00;

static int32 sSerialEnabled = 0;
static uint16 sSerialBasePort = 0x3f8;

static Console sInput, sOutput;
FILE *stdin, *stdout, *stderr;


//	serial debug output


static void
serial_putc(char c)
{
	// wait until the transmitter empty bit is set
	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x20) == 0)
		;

	out8(c, sSerialBasePort + SERIAL_TRANSMIT_BUFFER);
}


static void
serial_puts(const char *string, size_t size)
{
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
#if ENABLE_SERIAL
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


static void
serial_init(void)
{
	// copy the base ports of the optional 4 serial ports to the kernel args
	// 0x0000:0x0400 is the location of that information in the BIOS data segment
	uint16 *ports = (uint16 *)0x400;
	memcpy(gKernelArgs.platform_args.serial_base_ports, ports, sizeof(uint16) * MAX_SERIAL_PORTS);

	// only use the port if we could find one, else use the standard port
	if (gKernelArgs.platform_args.serial_base_ports[0] != 0)
		sSerialBasePort = gKernelArgs.platform_args.serial_base_ports[0];

	uint16 divisor = uint16(115200 / kSerialBaudRate);

	out8(0x80, sSerialBasePort + SERIAL_LINE_CONTROL);	/* set divisor latch access bit */
	out8(divisor & 0xf, sSerialBasePort + SERIAL_DIVISOR_LATCH_LOW);
	out8(divisor >> 8, sSerialBasePort + SERIAL_DIVISOR_LATCH_HIGH);
	out8(3, sSerialBasePort + SERIAL_LINE_CONTROL);		/* 8N1 */

#ifdef ENABLE_SERIAL
	serial_enable();
#endif
}


//	#pragma mark -


static void
scroll_up()
{
	memcpy(sScreenBase, sScreenBase + sScreenWidth,
		sScreenWidth * sScreenHeight * 2 - sScreenWidth * 2);
	sScreenOffset = (sScreenHeight - 1) * sScreenWidth;

	for (uint32 i = 0; i < sScreenWidth; i++)
		sScreenBase[sScreenOffset + i] = sColor | ' ';
}


//	#pragma mark -


Console::Console()
	: ConsoleNode()
{
}


ssize_t
Console::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	// don't seek in character devices
	// not implemented (and not yet? needed)
	return B_ERROR;
}


ssize_t
Console::WriteAt(void *cookie, off_t /*pos*/, const void *buffer, size_t bufferSize)
{
	const char *string = (const char *)buffer;

	if (sSerialEnabled > 0)
		serial_puts(string, bufferSize);

	if (gKernelArgs.frame_buffer.enabled)
		return bufferSize;

	for (uint32 i = 0; i < bufferSize; i++) {
		if (string[0] == '\n')
			sScreenOffset += sScreenWidth - (sScreenOffset % sScreenWidth);
		else
			sScreenBase[sScreenOffset++] = sColor | string[0];

		if (sScreenOffset >= sScreenWidth * sScreenHeight)
			scroll_up();

		string++;
	}
	return bufferSize;
}


//	#pragma mark -


void
console_clear_screen(void)
{
	if (gKernelArgs.frame_buffer.enabled)
		return;

	for (uint32 i = 0; i < sScreenWidth * sScreenHeight; i++)
		sScreenBase[i] = sColor;

	// reset cursor position as well
	sScreenOffset = 0;
}


int32
console_width(void)
{
	return sScreenWidth;
}


int32
console_height(void)
{
	return sScreenHeight;
}


void 
console_set_cursor(int32 x, int32 y)
{
	sScreenOffset = x + y * sScreenWidth;
}


void 
console_set_color(int32 foreground, int32 background)
{
	sColor = (background & 0x7) << 12 | (foreground & 0xf) << 8;
}


status_t
console_init(void)
{
	// ToDo: make screen size changeable via stage2_args

	serial_init();
	console_clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}


