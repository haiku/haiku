/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2001, Rob Judd <judd@ob-wan.com>
 * Copyright 2002, Marcus Overhagen <marcus@overhagen.de>
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


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


static const uint32 kSerialBaudRate = 115200;
static uint16 sSerialBasePort = 0x3f8;
	// COM1 is the default debug output port

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


//	#pragma mark -


void
arch_debug_remove_interrupt_handler(uint32 line)
{

}


void
arch_debug_install_interrupt_handlers(void)
{

}


int
arch_debug_blue_screen_try_getchar(void)
{
	return -1;
}


char
arch_debug_blue_screen_getchar(void)
{
	while(true)
		PAUSE();
	return 0;
}


int
arch_debug_serial_try_getchar(void)
{
	uint8 lineStatus = in8(sSerialBasePort + SERIAL_LINE_STATUS);
	if (lineStatus == 0xff) {
		// The "data available" bit is set, but also all error bits. Likely we
		// don't have a valid I/O port.
		return -1;
	}

	if ((lineStatus & 0x1) == 0)
		return -1;

	return in8(sSerialBasePort + SERIAL_RECEIVE_BUFFER);
}


char
arch_debug_serial_getchar(void)
{
	while (true) {
		uint8 lineStatus = in8(sSerialBasePort + SERIAL_LINE_STATUS);
		if (lineStatus == 0xff) {
			// The "data available" bit is set, but also all error bits. Likely
			// we don't have a valid I/O port.
			return 0;
		}

		if ((lineStatus & 0x1) != 0)
			break;

		PAUSE();
	}

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
	return B_OK;
}
