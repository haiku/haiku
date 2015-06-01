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

#ifdef __ARM__
#include <arch/arm/arch_uart_pl011.h>
#endif

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

#include "fdt_support.h"


// If we dprintf before the UART is initalized there will be no output


DebugUART*
debug_uart_from_fdt(const void *fdt)
{
	const char *name;
	int node;
	int len;
	phys_addr_t regs;
	int32 clock = 0;
	int32 speed = 0;
	const void *prop;
	DebugUART *uart = NULL;

	if (fdt == NULL)
		return NULL;

	name = fdt_get_alias(fdt, "serial");
	if (name == NULL)
		name = fdt_get_alias(fdt, "serial0");
	if (name == NULL)
		name = fdt_get_alias(fdt, "serial1");
	if (name == NULL)
		name = fdt_get_alias(fdt, "uart0");
	// TODO: else use /chosen linux,stdout-path
	if (name == NULL)
		return NULL;

	node = fdt_path_offset(fdt, name);

	if (node < 0)
		return NULL;

	// determine the MMIO address
	regs = fdt_get_device_reg(fdt, node);

	if (regs == 0)
		return NULL;

	dprintf("serial: using '%s', node %d @ %" B_PRIxPHYSADDR "\n",
		name, node, regs);

	// get the UART clock rate
	prop = fdt_getprop(fdt, node, "clock-frequency", &len);
	if (prop && len == 4) {
		clock = fdt32_to_cpu(*(uint32_t *)prop);
		dprintf("serial: clock %ld\n", clock);
	}

	// get current speed (XXX: not yet passed over)
	prop = fdt_getprop(fdt, node, "current-speed", &len);
	if (prop && len == 4) {
		speed = fdt32_to_cpu(*(uint32_t *)prop);
		dprintf("serial: speed %ld\n", speed);
	}

	// fdt_node_check_compatible returns 0 on match.

	if (fdt_node_check_compatible(fdt, node, "ns16550a") == 0
		|| fdt_node_check_compatible(fdt, node, "ns16550") == 0) {
		dprintf("serial: Found 8250 serial UART!\n");
		uart = arch_get_uart_8250(regs, clock);
	#ifdef __ARM__
	} else if (fdt_node_check_compatible(fdt, node, "arm,pl011") == 0
		|| fdt_node_check_compatible(fdt, node, "arm,primecell") == 0) {
		dprintf("serial: Found pl011 serial UART!\n");
		uart = arch_get_uart_pl011(regs, clock);
	#endif
	} else {
		// TODO: handle more UART types
		return NULL;
	}

	return uart;
}
