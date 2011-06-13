/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2001, Rob Judd <judd@ob-wan.com>
 * Copyright 2002, Marcus Overhagen <marcus@overhagen.de>
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#include "debugger_keymaps.h"
#include "ps2_defs.h"

#include <KernelExport.h>
#include <driver_settings.h>
#include <int.h>

#include <arch/cpu.h>
#include <arch/debug_console.h>
#include <boot/stage2.h>
#include <debug.h>

#include <string.h>
#include <stdlib.h>


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

	LEFT_ALT		= 56,
	RIGHT_ALT		= 58,

	CURSOR_LEFT		= 75,
	CURSOR_RIGHT	= 77,
	CURSOR_UP		= 72,
	CURSOR_DOWN		= 80,
	CURSOR_HOME		= 71,
	CURSOR_END		= 79,
	PAGE_UP			= 73,
	PAGE_DOWN		= 81,

	DELETE			= 83,
	SYS_REQ			= 84,
	F12				= 88,
};


static const uint32 kSerialBaudRate = 115200;
static uint16 sSerialBasePort = 0x3f8;
	// COM1 is the default debug output port

static bool sKeyboardHandlerInstalled = false;

static spinlock sSerialOutputSpinlock = B_SPINLOCK_INITIALIZER;


static void
init_serial_port(uint16 basePort, uint32 baudRate)
{
	sSerialBasePort = basePort;

	uint16 divisor = (uint16)(115200 / baudRate);

	out8(0x80, sSerialBasePort + SERIAL_LINE_CONTROL);	/* set divisor latch access bit */
	out8(divisor & 0xf, sSerialBasePort + SERIAL_DIVISOR_LATCH_LOW);
	out8(divisor >> 8, sSerialBasePort + SERIAL_DIVISOR_LATCH_HIGH);
	out8(3, sSerialBasePort + SERIAL_LINE_CONTROL);		/* 8N1 */
}


static void
put_char(const char c)
{
	// wait until the transmitter empty bit is set
	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x20) == 0)
		asm volatile ("pause;");

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
	static bool controlPressed = false;
	static bool altPressed = false;
	static bool sysReqPressed = false;
	uint8 key;

	key = in8(PS2_PORT_DATA);
	//dprintf("debug_keyboard_interrupt: key = 0x%x\n", key);

	if (key & 0x80) {
		if (key == LEFT_CONTROL)
			controlPressed = false;
		else if (key == LEFT_ALT)
			altPressed = false;
		else if (key == SYS_REQ)
			sysReqPressed = false;

		return B_HANDLED_INTERRUPT;
	}

	switch (key) {
		case LEFT_CONTROL:
			controlPressed = true;
			break;

		case LEFT_ALT:
		case RIGHT_ALT:
			altPressed = true;
			break;

		case SYS_REQ:
			sysReqPressed = true;
			break;

		case DELETE:
			if (controlPressed && altPressed)
				arch_cpu_shutdown(true);
			break;

		default:
			if (altPressed && sysReqPressed) {
				if (debug_emergency_key_pressed(kUnshiftedKeymap[key])) {
					// we probably have lost some keys, so reset our key states
					controlPressed = false;
					sysReqPressed = false;
					altPressed = false;
				}
			}
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


int
arch_debug_blue_screen_try_getchar(void)
{
	/* polling the keyboard, similar to code in keyboard
	 * driver, but without using an interrupt
	 */
	static bool shiftPressed = false;
	static bool controlPressed = false;
	static bool altPressed = false;
	static uint8 special = 0;
	static uint8 special2 = 0;
	uint8 key = 0;

	if (special & 0x80) {
		special &= ~0x80;
		return '[';
	}
	if (special != 0) {
		key = special;
		special = 0;
		return key;
	}
	if (special2 != 0) {
		key = special2;
		special2 = 0;
		return key;
	}

	uint8 status = in8(PS2_PORT_CTRL);

	if ((status & PS2_STATUS_OUTPUT_BUFFER_FULL) == 0) {
		// no data in keyboard buffer
		return -1;
	}

	key = in8(PS2_PORT_DATA);

	if (status & PS2_STATUS_AUX_DATA) {
		// we read mouse data, ignore it
		return -1;
	}

	if (key & 0x80) {
		// key up
		switch (key & ~0x80) {
			case LEFT_SHIFT:
			case RIGHT_SHIFT:
				shiftPressed = false;
				return -1;
			case LEFT_CONTROL:
				controlPressed = false;
				return -1;
			case LEFT_ALT:
				altPressed = false;
				return -1;
		}
	} else {
		// key down
		switch (key) {
			case LEFT_SHIFT:
			case RIGHT_SHIFT:
				shiftPressed = true;
				return -1;

			case LEFT_CONTROL:
				controlPressed = true;
				return -1;

			case LEFT_ALT:
				altPressed = true;
				return -1;

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
			case CURSOR_HOME:
				special = 0x80 | 'H';
				return '\x1b';
			case CURSOR_END:
				special = 0x80 | 'F';
				return '\x1b';
			case PAGE_UP:
				special = 0x80 | '5';
				special2 = '~';
				return '\x1b';
			case PAGE_DOWN:
				special = 0x80 | '6';
				special2 = '~';
				return '\x1b';


			case DELETE:
				if (controlPressed && altPressed)
					arch_cpu_shutdown(true);

				special = 0x80 | '3';
				special2 = '~';
				return '\x1b';

			default:
				if (controlPressed) {
					char c = kShiftedKeymap[key];
					if (c >= 'A' && c <= 'Z')
						return 0x1f & c;
				}

				if (altPressed)
					return kAltedKeymap[key];

				return shiftPressed
					? kShiftedKeymap[key] : kUnshiftedKeymap[key];
		}
	}

	return -1;
}


char
arch_debug_blue_screen_getchar(void)
{
	while (true) {
		int c = arch_debug_blue_screen_try_getchar();
		if (c >= 0)
			return (char)c;

		PAUSE();
	}
}


int
arch_debug_serial_try_getchar(void)
{
	if ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x1) == 0)
		return -1;

	return in8(sSerialBasePort + SERIAL_RECEIVE_BUFFER);
}


char
arch_debug_serial_getchar(void)
{
	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x1) == 0)
		asm volatile ("pause;");

	return in8(sSerialBasePort + SERIAL_RECEIVE_BUFFER);
}


static void
_arch_debug_serial_putchar(const char c)
{
	if (c == '\n') {
		put_char('\r');
		put_char('\n');
	} else if (c != '\r')
		put_char(c);
}

void
arch_debug_serial_putchar(const char c)
{
	cpu_status state = 0;
	if (!debug_debugger_running()) {
		state = disable_interrupts();
		acquire_spinlock(&sSerialOutputSpinlock);
	}

	_arch_debug_serial_putchar(c);

	if (!debug_debugger_running()) {
		release_spinlock(&sSerialOutputSpinlock);
		restore_interrupts(state);
	}
}


void
arch_debug_serial_puts(const char *s)
{
	cpu_status state = 0;
	if (!debug_debugger_running()) {
		state = disable_interrupts();
		acquire_spinlock(&sSerialOutputSpinlock);
	}

	while (*s != '\0') {
		_arch_debug_serial_putchar(*s);
		s++;
	}

	if (!debug_debugger_running()) {
		release_spinlock(&sSerialOutputSpinlock);
		restore_interrupts(state);
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
	// only use the port if we could find one, else use the standard port
	if (args != NULL && args->platform_args.serial_base_ports[0] != 0)
		sSerialBasePort = args->platform_args.serial_base_ports[0];

	init_serial_port(sSerialBasePort, kSerialBaudRate);

	return B_OK;
}


status_t
arch_debug_console_init_settings(kernel_args *args)
{
	uint32 baudRate = kSerialBaudRate;
	uint16 basePort = sSerialBasePort;
	void *handle;

	// get debug settings
	handle = load_driver_settings("kernel");
	if (handle != NULL) {
		const char *value = get_driver_parameter(handle, "serial_debug_port",
			NULL, NULL);
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

	init_serial_port(sSerialBasePort, kSerialBaudRate);

	return B_OK;
}
