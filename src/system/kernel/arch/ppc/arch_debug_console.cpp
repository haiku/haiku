/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch_platform.h>
#include <arch/debug_console.h>
#include <boot/kernel_args.h>
#include <kernel.h>
#include <vm.h>

#include <string.h>


char
arch_debug_blue_screen_getchar(void)
{
	return 0;
}


char
arch_debug_serial_getchar(void)
{
	return PPCPlatform::Default()->SerialDebugGetChar();
}


void
arch_debug_serial_putchar(const char c)
{
	return PPCPlatform::Default()->SerialDebugPutChar(c);
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
	return PPCPlatform::Default()->InitSerialDebug(args);
}


status_t
arch_debug_console_init_settings(kernel_args *args)
{
	return B_OK;
}

