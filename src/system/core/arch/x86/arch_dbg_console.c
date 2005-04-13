/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2001, Rob Judd <judd@ob-wan.com>
 * Copyright 2002, Marcus Overhagen <marcus@overhagen.de>
 * Distributed under the terms of the Haiku License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <driver_settings.h>
#include <int.h>
#include <arch/cpu.h>
#include <arch/dbg_console.h>

#include <boot/stage2.h>

#include <string.h>
#include <stdlib.h>


/* If you've enabled Bochs debug output in the kernel driver
 * settings and has defined this, serial debug output will be
 * disabled and redirected to Bochs.
 * Define this only if not used as compile parameter.
 */
#ifndef BOCHS_DEBUG_HACK
#	define BOCHS_DEBUG_HACK 0
#endif


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

static uint16 sSerialBasePort = 0x3f8;
	// COM1 is the default debug output port

#if BOCHS_DEBUG_HACK
static bool sBochsOutput = false;
#endif


char
arch_dbg_con_read(void)
{
#if BOCHS_DEBUG_HACK
	if (sBochsOutput) {
		/* polling the keyboard, similar to code in keyboard
		 * driver, but without using an interrupt
		 */
		static const char unshifted_keymap[128] = {
			0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, '\t',
			'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
			'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
			'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			'\\', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};
		const char shifted_keymap[128] = {
			0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, '\t',
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
			'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
			'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};	
		static bool shift = false;
		static uint8 last = 0;
		uint8 key, ascii = 0;
		do {
			while ((key = in8(0x60)) == last)
				;
			last = key;
			if (key & 0x80) {
				if (key == (0x80 + 42) || key == (54 + 0x80))
					shift = false;
			} else {
				if (key == 42 || key == 54)
					shift = true;
				else
					ascii = shift ? shifted_keymap[key] : unshifted_keymap[key];
			}
		} while (!ascii);
		return ascii;
	}
#endif

	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x1) == 0)
		;

	return in8(sSerialBasePort + SERIAL_RECEIVE_BUFFER);
}


static void
put_char(const char c)
{
#if BOCHS_DEBUG_HACK
	if (sBochsOutput) {
		out8(c, 0xe9);
		return;
	}
#endif

	// wait until the transmitter empty bit is set
	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x20) == 0)
		;

	out8(c, sSerialBasePort + SERIAL_TRANSMIT_BUFFER);
}


char
arch_dbg_con_putch(const char c)
{
	if (c == '\n') {
		put_char('\r');
		put_char('\n');
	} else if (c != '\r')
		put_char(c);

	return c;
}


void
arch_dbg_con_puts(const char *s)
{
	while (*s != '\0') {
		arch_dbg_con_putch(*s);
		s++;
	}
}


void
arch_dbg_con_early_boot_message(const char *string)
{
	// this function will only be called in fatal situations
	// ToDo: also enable output via text console?!
	arch_dbg_con_init(NULL);
	arch_dbg_con_puts(string);
}


status_t
arch_dbg_con_init(kernel_args *args)
{
	uint16 divisor = (uint16)(115200 / kSerialBaudRate);

	// only use the port if we could find one, else use the standard port
	if (args->platform_args.serial_base_ports[0] != 0)
		sSerialBasePort = args->platform_args.serial_base_ports[0];

	out8(0x80, sSerialBasePort + SERIAL_LINE_CONTROL);	/* set divisor latch access bit */
	out8(divisor & 0xf, sSerialBasePort + SERIAL_DIVISOR_LATCH_LOW);
	out8(divisor >> 8, sSerialBasePort + SERIAL_DIVISOR_LATCH_HIGH);
	out8(3, sSerialBasePort + SERIAL_LINE_CONTROL);		/* 8N1 */

	return B_OK;
}


status_t
arch_dbg_con_init_settings(kernel_args *args)
{
	uint32 baudRate = kSerialBaudRate;
	uint16 basePort = sSerialBasePort;
	uint16 divisor;
	void *handle;

	// get debug settings
	handle = load_driver_settings("kernel");
	if (handle != NULL) {
		const char *value;

#if BOCHS_DEBUG_HACK
		sBochsOutput = get_driver_boolean_parameter(handle, "bochs_debug_output", false, false);
#endif
		value = get_driver_parameter(handle, "serial_debug_port", NULL, NULL);
		if (value != NULL) {
			int32 number = strtol(value, NULL, 0);
			if (number < MAX_SERIAL_PORTS) {
				// use as index into port array
				if (args->platform_args.serial_base_ports[number] != 0)
					basePort = args->platform_args.serial_base_ports[number];
			} else {
				// use as port number directly
				basePort = number;
			}
		}

		value = get_driver_parameter(handle, "serial_debug_speed", NULL, NULL);
		if (value != NULL) {
			int32 number = strtol(value, NULL, 0);
			switch (number) {
				case 9600:
				case 19200:
				case 38400:
				case 57600:
				case 115200:
				//case 230400:
					baudRate = number;
			}
		}

		unload_driver_settings(handle);
	}

	if (sSerialBasePort == basePort && baudRate == kSerialBaudRate)
		return B_OK;

	sSerialBasePort = basePort;
	divisor = (uint16)(115200 / baudRate);

	out8(0x80, sSerialBasePort + SERIAL_LINE_CONTROL);	/* set divisor latch access bit */
	out8(divisor & 0xf, sSerialBasePort + SERIAL_DIVISOR_LATCH_LOW);
	out8(divisor >> 8, sSerialBasePort + SERIAL_DIVISOR_LATCH_HIGH);
	out8(3, sSerialBasePort + SERIAL_LINE_CONTROL);		/* 8N1 */

	return B_OK;
}
