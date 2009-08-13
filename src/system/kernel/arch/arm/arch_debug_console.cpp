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
#include <boot/kernel_args.h>
#include <kernel.h>
#include <vm.h>
#include <arch/arm/uart.h>
#include <string.h>


void
arch_debug_remove_interrupt_handler(uint32 line)
{
}


void
arch_debug_install_interrupt_handlers(void)
{
}


char
arch_debug_blue_screen_getchar(void)
{
#warning ARM WRITEME
	return 0;
//M68KPlatform::Default()->BlueScreenGetChar();
}


char
arch_debug_serial_getchar(void)
{
#warning ARM WRITEME
return uart_getc(0,FALSE);
//	return 0;
//M68KPlatform::Default()->SerialDebugGetChar();
}


void
arch_debug_serial_putchar(const char c)
{
//uart_putc(0,c);
//uart_putc(1,c);
uart_putc(0,c);
#warning ARM WRITEME

//	return 0;
//M68KPlatform::Default()->SerialDebugPutChar(c);
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

uart_init_early();
uart_init();//todo
uart_init_port(0,9600);
uart_init_port(1,9600);
uart_init_port(2,9600);
#warning ARM WRITEME

	return 0;
//M68KPlatform::Default()->InitSerialDebug(args);
}


status_t
arch_debug_console_init_settings(kernel_args *args)
{
	return B_OK;
}

