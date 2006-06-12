/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2001, Rob Judd <judd@ob-wan.com>
 * Copyright 2002, Marcus Overhagen <marcus@overhagen.de>
 * Distributed under the terms of the Haiku License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "ps2_defs.h"

#include <KernelExport.h>
#include <driver_settings.h>
#include <int.h>

#include <arch/cpu.h>
#include <arch/debug_console.h>
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

enum keycodes {
	LEFT_SHIFT		= 42,
	RIGHT_SHIFT		= 54,

	LEFT_CONTROL	= 29,
	RIGHT_CONTROL	= 157,

	LEFT_ALT		= 56,
	RIGHT_ALT		= 184,

	CURSOR_LEFT		= 75,
	CURSOR_RIGHT	= 77,
	CURSOR_UP		= 72,
	CURSOR_DOWN		= 80,

	BREAK			= 198,
	DELETE			= 201,
	F12				= 88,
};

static const char kUnshiftedKeymap[128] = {
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, '\t',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'\\', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char kShiftedKeymap[128] = {
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, '\t',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint32 kSerialBaudRate = 115200;

static uint16 sSerialBasePort = 0x3f8;
	// COM1 is the default debug output port

static bool sKeyboardHandlerInstalled = false;
#if BOCHS_DEBUG_HACK
static bool sBochsOutput = false;
#endif


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


/**	Minimal keyboard handler to be able to get into the debugger and
 *	reboot the machine before the input_server is up and running.
 *	It is added as soon as interrupts become available, and removed
 *	again if anything else requests the interrupt 1.
 */

static int32
debug_keyboard_interrupt(void *data)
{
	static bool controlPressed;
	static bool altPressed;
	uint8 key;

	key = in8(PS2_PORT_DATA);
	//dprintf("debug_keyboard_interrupt: key = 0x%x\n", key);

	if (key & 0x80) {
		if (key == LEFT_CONTROL)
			controlPressed = false;
		if (key == LEFT_ALT)
			altPressed = false;

		return B_HANDLED_INTERRUPT;
	}

	switch (key) {
		case LEFT_CONTROL:
			controlPressed = true;
			break;

		case LEFT_ALT:
			altPressed = true;
			break;

		case DELETE:
			if (controlPressed && altPressed)
				arch_cpu_shutdown(true);
			break;

		/* the following code has two possibilities because of issues
		 * with BeBochs & BeOS (all special keys don't map to anything
		 * useful, and SYS_REQ does a screen dump in BeOS).
		 * ToDo: remove these key functions some day...
		 */
		case F12:
		case BREAK:
			panic("Keyboard Requested Halt\n");
			break;
	}

	return B_HANDLED_INTERRUPT;
}


//	#pragma mark -


void
arch_debug_remove_interrupt_handler(uint32 line)
{
	if (line != INT_PS2_KEYBOARD || !sKeyboardHandlerInstalled)
		return;

	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &debug_keyboard_interrupt,
		NULL);
	sKeyboardHandlerInstalled = false;
}


void
arch_debug_install_interrupt_handlers(void)
{
	install_io_interrupt_handler(INT_PS2_KEYBOARD, &debug_keyboard_interrupt,
		NULL, 0);
	sKeyboardHandlerInstalled = true;
}


char
arch_debug_blue_screen_getchar(void)
{
	/* polling the keyboard, similar to code in keyboard
	 * driver, but without using an interrupt
	 */
	static bool shift = false;
	static uint8 special = 0;
	uint8 key, ascii = 0;

	if (special & 0x80) {
		special &= ~0x80;
		return '[';
	}
	if (special != 0) {
		key = special;
		special = 0;
		return key;
	}

	while (true) {
		uint8 status = in8(PS2_PORT_CTRL);

		if ((status & PS2_STATUS_OUTPUT_BUFFER_FULL) == 0) {
			// no data in keyboard buffer
			spin(200);
			continue;
		}

		spin(200);
		key = in8(PS2_PORT_DATA);

		if (status & PS2_STATUS_AUX_DATA) {
			// we read mouse data, ignore it
			continue;
		}

		if (key & 0x80) {
			// key up
			if (key == (0x80 | LEFT_SHIFT) || key == (0x80 | RIGHT_SHIFT))
				shift = false;
		} else {
			// key down
			switch (key) {
				case LEFT_SHIFT:
				case RIGHT_SHIFT:
					shift = true;
					break;

				// start escape sequence for cursor movement
				case CURSOR_UP:
					special = 0x80 | 'A';
					return '\x1b';
				case CURSOR_DOWN:
					special = 0x80 | 'B';
					return '\x1b';
				case CURSOR_RIGHT:
					special = 0x80 | 'C';
					return '\x1b';
				case CURSOR_LEFT:
					special = 0x80 | 'D';
					return '\x1b';

				default:
					return shift ? kShiftedKeymap[key] : kUnshiftedKeymap[key];
			}
		}
	}
}


char
arch_debug_serial_getchar(void)
{
#if BOCHS_DEBUG_HACK
	if (sBochsOutput)
		return arch_debug_blue_screen_getchar();
#endif

	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x1) == 0)
		;

	return in8(sSerialBasePort + SERIAL_RECEIVE_BUFFER);
}


void
arch_debug_serial_putchar(const char c)
{
	if (c == '\n') {
		put_char('\r');
		put_char('\n');
	} else if (c != '\r')
		put_char(c);
}


void
arch_debug_serial_puts(const char *s)
{
	while (*s != '\0') {
		arch_debug_serial_putchar(*s);
		s++;
	}
}


void
arch_debug_serial_early_boot_message(const char *string)
{
	// this function will only be called in fatal situations
	// ToDo: also enable output via text console?!
	arch_debug_console_init(NULL);
	arch_debug_serial_puts(string);
}


status_t
arch_debug_console_init(kernel_args *args)
{
	uint16 divisor = (uint16)(115200 / kSerialBaudRate);

	// only use the port if we could find one, else use the standard port
	if (args != NULL && args->platform_args.serial_base_ports[0] != 0)
		sSerialBasePort = args->platform_args.serial_base_ports[0];

	out8(0x80, sSerialBasePort + SERIAL_LINE_CONTROL);	/* set divisor latch access bit */
	out8(divisor & 0xf, sSerialBasePort + SERIAL_DIVISOR_LATCH_LOW);
	out8(divisor >> 8, sSerialBasePort + SERIAL_DIVISOR_LATCH_HIGH);
	out8(3, sSerialBasePort + SERIAL_LINE_CONTROL);		/* 8N1 */

	return B_OK;
}


status_t
arch_debug_console_init_settings(kernel_args *args)
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
			if (number >= MAX_SERIAL_PORTS) {
				// use as port number directly
				basePort = number;
			} else if (number >= 0) {
				// use as index into port array
				if (args->platform_args.serial_base_ports[number] != 0)
					basePort = args->platform_args.serial_base_ports[number];
			} else {
				// ignore value and use default
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
