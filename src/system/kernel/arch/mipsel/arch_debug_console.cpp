/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


//#include <arch_platform.h>
#include <arch/debug_console.h>
#include <boot/kernel_args.h>
#include <kernel.h>
#include <vm/vm.h>
#include <arch/arm/uart.h>
#include <string.h>


void
arch_debug_remove_interrupt_handler(uint32 line)
{
#warning IMPLEMENT arch_debug_remove_interrupt_handler
}


void
arch_debug_install_interrupt_handlers(void)
{
#warning IMPLEMENT arch_debug_install_interrupt_handlers
}


int
arch_debug_blue_screen_try_getchar(void)
{
#warning IMPLEMENT arch_debug_blue_screen_try_getchar
	return -1;
}


char
arch_debug_blue_screen_getchar(void)
{
#warning IMPLEMENT arch_debug_blue_screen_getchar
	return 0;
}


int
arch_debug_serial_try_getchar(void)
{
#warning IMPLEMENT arch_debug_serial_try_getchar
	return -1;
}


char
arch_debug_serial_getchar(void)
{
#warning IMPLEMENT arch_debug_serial_getchar
	return 0;
}


void
arch_debug_serial_putchar(const char c)
{
#warning IMPLEMENT arch_debug_serial_putchar
}


void
arch_debug_serial_puts(const char* s)
{
	while (*s != '\0') {
		arch_debug_serial_putchar(*s);
		s++;
	}
}


void
arch_debug_serial_early_boot_message(const char* string)
{
#warning IMPLEMENT arch_debug_serial_early_boot_message
}


status_t
arch_debug_console_init(kernel_args* args)
{
#warning IMPLEMENT arch_debug_console_init
	return 0;
}


status_t
arch_debug_console_init_settings(kernel_args* args)
{
#warning IMPLEMENT arch_debug_console_init_settings
	return B_ERROR;
}

