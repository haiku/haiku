/*
 * Copyright 2019-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#include "string.h"

#include <boot/platform.h>
#include <boot/stage2.h>
#include <arch_acpi.h>

#include "serial.h"
#include "acpi.h"

#include <arch/arm/arch_uart_pl011.h>
#include <arch/generic/debug_uart_8250.h>


static void arch_acpi_get_uart_pl011(const uart_info &uart)
{
	static char sUART[sizeof(ArchUARTPL011)];
	gUART = new(sUART) ArchUARTPL011(uart.regs.start,
		uart.clock != 0 ? uart.clock : 0x16e3600);
}


void
arch_handle_acpi()
{
	acpi_spcr *spcr = (acpi_spcr*)acpi_find_table(ACPI_SPCR_SIGNATURE);
	if (spcr != NULL) {
		uart_info &uart = gKernelArgs.arch_args.uart;

		if (spcr->interface_type == ACPI_SPCR_INTERFACE_TYPE_PL011) {
			strcpy(uart.kind, UART_KIND_PL011);
		} else if (spcr->interface_type == ACPI_SPCR_INTERFACE_TYPE_16550) {
			strcpy(uart.kind, UART_KIND_8250);
		}

		uart.regs.start = spcr->base_address.address;
		uart.regs.size = B_PAGE_SIZE;
		uart.irq = spcr->gisv;
		uart.clock = spcr->clock;

		if (spcr->interface_type == ACPI_SPCR_INTERFACE_TYPE_PL011) {
			arch_acpi_get_uart_pl011(uart);
		}

		dprintf("discovered uart from acpi: base=%lx, irq=%u, clock=%lu\n",
			uart.regs.start, uart.irq, uart.clock);
	}
}
