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
	regs = fdt_get_device_reg(fdt, node, false);

	if (regs == 0)
		return NULL;

	TRACE(("serial: using '%s', node %d @ %" B_PRIxPHYSADDR "\n",
		name, node, regs));

	// get the UART clock rate
	prop = fdt_getprop(fdt, node, "clock-frequency", &len);
	if (prop && len == 4) {
		clock = fdt32_to_cpu(*(uint32_t *)prop);
		TRACE(("serial: clock %ld\n", clock));
	}

	// get current speed (XXX: not yet passed over)
	prop = fdt_getprop(fdt, node, "current-speed", &len);
	if (prop && len == 4) {
		speed = fdt32_to_cpu(*(uint32_t *)prop);
		TRACE(("serial: speed %ld\n", speed));
	}

	// fdt_node_check_compatible returns 0 on match.

	if (fdt_node_check_compatible(fdt, node, "ns16550a") == 0
		|| fdt_node_check_compatible(fdt, node, "ns16550") == 0) {
		TRACE(("serial: Found 8250 serial UART!\n"));
		uart = arch_get_uart_8250(regs, clock);
	#if defined(__arm__)
	} else if (fdt_node_check_compatible(fdt, node, "ti,omap3-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,omap4-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,omap5-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,am3352-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,am4372-uart") == 0
		|| fdt_node_check_compatible(fdt, node, "ti,dra742-uart") == 0) {
		// TODO: ti,am* and ti,dr* have some special quirks.
		TRACE(("serial: Found omap 8250 serial UART!\n"));
		uart = arch_get_uart_8250_omap(regs, clock);
	} else if (fdt_node_check_compatible(fdt, node, "arm,pl011") == 0
		|| fdt_node_check_compatible(fdt, node, "arm,primecell") == 0) {
		TRACE(("serial: Found pl011 serial UART!\n"));
		uart = arch_get_uart_pl011(regs, clock);
	#endif
	} else {
		// TODO: handle more UART types
		return NULL;
	}

	return uart;
}
