/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/debug_console.h>
#include <arch/generic/debug_uart.h>
#include <arch/generic/debug_uart_8250.h>
#include <arch/riscv64/arch_uart_sifive.h>
#include <boot/kernel_args.h>
#include <kernel.h>
#include <vm/vm.h>
#include <Htif.h>

#include <string.h>


static DebugUART* sArchDebugUART = NULL;


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
	if (sArchDebugUART != NULL)
		return sArchDebugUART->GetChar(false);

	return 0;
}


void
arch_debug_serial_putchar(const char c)
{
	if (sArchDebugUART != NULL) {
		sArchDebugUART->PutChar(c);
		return;
	}

	HtifOutChar(c);
}


void
arch_debug_serial_puts(const char *s)
{
	while (*s != '\0') {
		char ch = *s;
		if (ch == '\n') {
			arch_debug_serial_putchar('\r');
			arch_debug_serial_putchar('\n');
		} else if (ch != '\r')
			arch_debug_serial_putchar(ch);
		s++;
	}
}


void
arch_debug_serial_early_boot_message(const char *string)
{
	arch_debug_serial_puts(string);
}


status_t
arch_debug_console_init(kernel_args *args)
{
	if (strncmp(args->arch_args.uart.kind, UART_KIND_8250,
			sizeof(args->arch_args.uart.kind)) == 0) {
		sArchDebugUART = arch_get_uart_8250(args->arch_args.uart.regs.start,
			args->arch_args.uart.clock);
	} else if (strncmp(args->arch_args.uart.kind, UART_KIND_SIFIVE,
			sizeof(args->arch_args.uart.kind)) == 0) {
		sArchDebugUART = arch_get_uart_sifive(args->arch_args.uart.regs.start,
			args->arch_args.uart.clock);
	}

	if (sArchDebugUART != NULL)
		sArchDebugUART->InitEarly();

	return B_OK;
}


status_t
arch_debug_console_init_settings(kernel_args *args)
{
	return B_OK;
}
