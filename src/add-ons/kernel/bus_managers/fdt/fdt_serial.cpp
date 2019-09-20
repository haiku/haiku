/*
 * Copyright 2012, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */

#include "fdt_serial.h"

#include <KernelExport.h>
#include <ByteOrder.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/cdefs.h>

#include <arch/generic/debug_uart_8250.h>

#if defined(__arm__)
#include <arch/arm/arch_uart_pl011.h>
#endif

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

#include "fdt_support.h"


//#define TRACE_SERIAL
#ifdef TRACE_SERIAL
#	define TRACE(x...) dprintf("INIT: " x)
#else
#	define TRACE(x...) ;
#endif


// If we dprintf before the UART is initalized there will be no output

static DebugUART*
debug_uart_from_node(const void *fdt, int node)
{
	int len;
	const void *prop;
	phys_addr_t regs;
	int32 clock = 0;
	int32 speed = 0;
	DebugUART *uart = NULL;

	if (node < 0 || fdt == NULL)
		return NULL;

	// determine the MMIO address
	regs = fdt_get_device_reg(fdt, node, false);

	if (regs == 0) {
		TRACE("%s: FDT UART regs not found!\n", __func__);
		return NULL;
	}

	TRACE("serial: checking '%s', node %d @ %" B_PRIxPHYSADDR "\n",
		name, node, regs);

	// get the UART clock rate
	prop = fdt_getprop(fdt, node, "clock-frequency", &len);
	if (prop && len == 4) {
		clock = fdt32_to_cpu(*(uint32_t *)prop);
		TRACE("serial: clock %ld\n", clock);
	}

	// get current speed (XXX: not yet passed over)
	prop = fdt_getprop(fdt, node, "current-speed", &len);
	if (prop && len == 4) {
		speed = fdt32_to_cpu(*(uint32_t *)prop);
		TRACE("serial: speed %ld\n", speed);
	}

	// fdt_node_check_compatible returns 0 on match.
	if (fdt_node_check_compatible(fdt, node, "ns16550a") == 0
		|| fdt_node_check_compatible(fdt, node, "ns16550") == 0
		|| fdt_node_check_compatible(fdt, node, "snps,dw-apb-uart") == 0) {
		TRACE("serial: Found 8250 serial UART!\n");
		uart = arch_get_uart_8250(regs, clock);
	#if defined(__arm__)
	} else if (fdt_node_check_compatible(fdt, node, "ti,omap3-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,omap4-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,omap5-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,am3352-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,am4372-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,dra742-uart") == 0) {
		// TODO: ti,am* and ti,dr* have some special quirks.
		TRACE("serial: Found omap 8250 serial UART!\n");
		uart = arch_get_uart_8250_omap(regs, clock);
	} else if (fdt_node_check_compatible(fdt, node, "arm,pl011") == 0
		|| fdt_node_check_compatible(fdt, node, "arm,primecell") == 0) {
		TRACE("serial: Found pl011 serial UART!\n");
		uart = arch_get_uart_pl011(regs, clock);
	#endif
	}
	return uart;
}


DebugUART*
debug_uart_from_fdt(const void *fdt)
{
	int chosen_node;
	int node;
	int len;
	const char *name;
	const void *prop;
	DebugUART *uart = NULL;

	if (fdt == NULL) {
		TRACE("%s: No FDT found!\n", __func__);
		return NULL;
	}

	chosen_node = fdt_path_offset(fdt, "/chosen");
	if (chosen_node >= 0) {
		prop = fdt_getprop(fdt, chosen_node, "stdout-path", &len);
		if (prop && len > 0) {
			node = fdt_path_offset(fdt, (const char*)prop);
			uart = debug_uart_from_node(fdt, node);
		}
		if (uart == NULL) {
			prop = fdt_getprop(fdt, chosen_node, "linux,stdout-path", &len);
			if (prop && len > 0) {
				node = fdt_path_offset(fdt, (const char*)prop);
				uart = debug_uart_from_node(fdt, node);
			}
		}

		if (uart == NULL) {
			// From what i've seen, stdout is generally an alias.
			// we could check for "/..." in the prop, but not sure
			// it's needed. If we *did* check for a prop starting
			// with / we could make all three of these "the same"
			prop = fdt_getprop(fdt, chosen_node, "stdout", &len);
			if (prop && len > 0) {
				name = fdt_get_alias(fdt, (const char*)prop);
				if (name != NULL) {
					node = fdt_path_offset(fdt, name);
					uart = debug_uart_from_node(fdt, node);
				}
			}
		}

		// Whoo-hoo! Bail.
		if (uart != NULL)
			return uart;
	}

	// If we didn't find a /chosen serial device, lets search for some common aliases
	char aliases[][8] = {
		"serial",
		"serial0",
		"uart",
		"uart0",
		"serial1",
		"serial2",
		"serial3",
		"uart1",
		"uart2",
		"uart3"
	};

	// For each known common serial alias, check it out and see if we have the
	// needed driver for it. uart0 seems most common.
	for (int index = 0; index < sizeof(aliases[0]) / sizeof(aliases); index++) {
		name = fdt_get_alias(fdt, aliases[index]);
		if (name == NULL)
			continue;

		node = fdt_path_offset(fdt, name);
		if (node < 0) {
			TRACE("%s: FDT node not found!\n", __func__);
			continue;
		}
		uart = debug_uart_from_node(fdt, node);

		// We found a valid serial device. bail.
		if (uart != NULL)
			break;
	}

	// It would be nice if we had *some* communication mechanism here if uart is still
	// NULL to warn the user that we couldn't find a serial port.

	return uart;
}
