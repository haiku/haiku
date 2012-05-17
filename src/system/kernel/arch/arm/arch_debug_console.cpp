/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


//#include <arch_platform.h>
#include <arch/debug_console.h>
#include <arch/generic/debug_uart_8250.h>
#include <arch/arm/arch_uart_pl011.h>
#include <boot/kernel_args.h>
#include <kernel.h>
#include <vm/vm.h>
#include <string.h>

#include "board_config.h"


DebugUART *gArchDebugUART;


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
	// TODO: Implement correctly!
	return arch_debug_blue_screen_getchar();
}


char
arch_debug_blue_screen_getchar(void)
{
	return arch_debug_serial_getchar();
}


int
arch_debug_serial_try_getchar(void)
{
	// TODO: Implement correctly!
	return arch_debug_serial_getchar();
}


char
arch_debug_serial_getchar(void)
{
	return gArchDebugUART->GetChar(false);
}


void
arch_debug_serial_putchar(const char c)
{
	gArchDebugUART->PutChar(c);
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
	arch_debug_serial_puts(string);
}


status_t
arch_debug_console_init(kernel_args *args)
{
	#if defined(BOARD_UART_AMBA_PL011)
	gArchDebugUART = arch_get_uart_pl011(BOARD_UART_DEBUG, BOARD_UART_CLOCK);
	#else
	// More Generic 8250
	gArchDebugUART = arch_get_uart_8250(BOARD_UART_DEBUG, BOARD_UART_CLOCK);
	#endif

	gArchDebugUART->InitEarly();

	return B_OK;
}


status_t
arch_debug_console_init_settings(kernel_args *args)
{
	return B_OK;
}
