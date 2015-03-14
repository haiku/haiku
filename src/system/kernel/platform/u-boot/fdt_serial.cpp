/*
 * Copyright 2012, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <ByteOrder.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/cdefs.h>

#include <arch/generic/debug_uart.h>
#include <arch/generic/debug_uart_8250.h>

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

#include "fdt_support.h"


extern "C" DebugUART *debug_uart_from_fdt(const void *fdt);


DebugUART *
debug_uart_from_fdt(const void *fdt)
{
	const char *name;
	//const char *type;
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
	//dprintf("serial: using '%s', node %d\n", name, node);
	if (node < 0)
		return NULL;

	// determine the MMIO address
	regs = fdt_get_device_reg(node);

	if (regs == 0)
		return NULL;

	// get the UART clock rate
	prop = fdt_getprop(fdt, node, "clock-frequency", &len);
	if (prop && len == 4) {
		clock = fdt32_to_cpu(*(uint32_t *)prop);
		//dprintf("serial: clock %ld\n", clock);
	}

	// get current speed (XXX: not yet passed over)
	prop = fdt_getprop(fdt, node, "current-speed", &len);
	if (prop && len == 4) {
		speed = fdt32_to_cpu(*(uint32_t *)prop);
		//dprintf("serial: speed %ld\n", speed);
	}

	if (fdt_node_check_compatible(fdt, node, "ns16550a") == 1
		|| fdt_node_check_compatible(fdt, node, "ns16550") == 1) {
		uart = arch_get_uart_8250(regs, clock);
		//dprintf("serial: using 8250\n");
		// XXX:assume speed is already set
		(void)speed;
	} else {
		// TODO: handle more UART types
		return NULL;
	}

	return uart;
}

