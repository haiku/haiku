/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/debug_console.h>
#include <boot/kernel_args.h>
#include <kernel.h>
#include <vm/vm.h>

#include <string.h>


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
	return 0;
}


char
arch_debug_blue_screen_getchar(void)
{
	return 0;
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
	return 0;
}


void
arch_debug_serial_putchar(const char c)
{
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
}


status_t
arch_debug_console_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_debug_console_init_settings(kernel_args *args)
{
	return B_OK;
}


