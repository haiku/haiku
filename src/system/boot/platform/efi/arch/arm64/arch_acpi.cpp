/*
 * Copyright 2019-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#include "string.h"

#include <boot/platform.h>
#include <boot/stage2.h>
#include <arch_acpi.h>
#include <arch_smp.h>

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
	} else {
		acpi_dbg2 *dbg2 = (acpi_dbg2*)acpi_find_table(ACPI_DBG2_SIGNATURE);
		if (dbg2 != NULL) {
			acpi_dbg2_device_info *info = (acpi_dbg2_device_info*)((char*)dbg2
				+ dbg2->offset_dbg_device_info);
			while (info != (acpi_dbg2_device_info*)((char*)dbg2 + dbg2->header.length)) {
				if (info->port_type == ACPI_DBG2_PORT_TYPE_SERIAL && info->num_addresses > 0) {
					uart_info &uart = gKernelArgs.arch_args.uart;

					if (info->port_subtype == ACPI_DBG2_PORT_SUBTYPE_PL011)
						strcpy(uart.kind, UART_KIND_PL011);
					else if (info->port_subtype == ACPI_DBG2_PORT_SUBTYPE_16550)
						strcpy(uart.kind, UART_KIND_8250);

					acpi_gas *base_addr = (acpi_gas*)((char*)info + info->base_addr_offset);
					uint32 *base_size = (uint32*)((char*)info + info->addr_size_offset);

					uart.regs.start = base_addr->address;
					uart.regs.size = *base_size;
					uart.irq = 0;
					uart.clock = 0;

					if (info->port_subtype == ACPI_DBG2_PORT_SUBTYPE_PL011)
						arch_acpi_get_uart_pl011(uart);

					dprintf("discovered uart from dbg2 acpi: base=%lx\n", uart.regs.start);
					break;
				}

				info = (acpi_dbg2_device_info*)((char*)info + info->length);
			}
		}
	}

	acpi_madt *madt = (acpi_madt*)acpi_find_table(ACPI_MADT_SIGNATURE);
	if (madt != NULL) {
		uint64 gicc_base = 0;
		uint64 gicd_base = 0;
		uint8 version = 0;

		acpi_apic *desc = (acpi_apic*)(madt + 1);
		while (desc != (acpi_apic*)((char*)madt + madt->header.length)) {
			if (desc->type == ACPI_MADT_GIC_INTERFACE) {
				acpi_gic_interface *acpi_gicc = (acpi_gic_interface*)desc;
				if (acpi_gicc->cpu_interface_num == 0)
					gicc_base = acpi_gicc->base_address;

				platform_cpu_info* cpu = NULL;
				arch_smp_register_cpu(&cpu);
				if (cpu == NULL)
					continue;
				cpu->id = acpi_gicc->cpu_interface_num;
				cpu->mpidr = acpi_gicc->mpidr;
			} else if (desc->type == ACPI_MADT_GIC_DISTRIBUTOR) {
				acpi_gic_distributor *acpi_gicd = (acpi_gic_distributor*)desc;
				gicd_base = acpi_gicd->base_address;
				version = acpi_gicd->gic_version;
			}
			desc = (acpi_apic*)((char*)desc + desc->length);
		}

		if (version == 2 && gicc_base != 0 && gicd_base != 0) {
			intc_info &intc = gKernelArgs.arch_args.interrupt_controller;
			strcpy(intc.kind, INTC_KIND_GICV2);
			intc.regs1.start = gicd_base;
			intc.regs2.start = gicc_base;

			dprintf("discovered gic from acpi: version=%d, gicd=%lx, gicc=%lx\n",
				version, gicd_base, gicc_base);
		}
	}
}
