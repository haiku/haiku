/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <arch/debug_console.h>
#include <arch/generic/debug_uart.h>
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
	return -1;
}


char
arch_debug_blue_screen_getchar(void)
{
	return -1;
}


int
arch_debug_serial_try_getchar(void)
{
	return -1;
}


char
arch_debug_serial_getchar(void)
{
	return -1;
}


void
arch_debug_serial_putchar(const char c)
{
}


void
arch_debug_serial_puts(const char *s)
{
}


void
arch_debug_serial_early_boot_message(const char *string)
{
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
