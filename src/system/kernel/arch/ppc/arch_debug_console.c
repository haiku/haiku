/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <kernel.h>
#include <vm.h>
#include <boot/kernel_args.h>
#include <arch/debug_console.h>
#include <platform/openfirmware/openfirmware.h>

#include <string.h>


static int sInput = -1;
static int sOutput = -1;


char
arch_debug_blue_screen_getchar(void)
{
	return 0;
}


char
arch_debug_serial_getchar(void)
{
	return 0;
}


void
arch_debug_serial_putchar(const char c)
{
	if (c == '\n')
		of_write(sOutput, "\r\n", 2);
	else
		of_write(sOutput, &c, 1);
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
	if (of_getprop(gChosen, "stdin", &sInput, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	if (of_getprop(gChosen, "stdout", &sOutput, sizeof(int)) == OF_FAILED)
		return B_ERROR;

	return B_OK;
}


status_t
arch_debug_console_init_settings(kernel_args *args)
{
	return B_OK;
}

