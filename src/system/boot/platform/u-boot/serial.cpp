/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2012, Alexander von Gluck, kallisti5@unixzen.com
 * Distributed under the terms of the MIT License.
 */


#include "serial.h"

#include <debug_uart_8250.h>
#include <board_config.h>
#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>
#include <new>
#include <string.h>

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};


DebugUART* gUART;

static int32 sSerialEnabled = 0;
static char sBuffer[16384];
static uint32 sBufferPosition;


static void
serial_putc(char c)
{
	gUART->PutChar(c);
}


extern "C" void
serial_puts(const char* string, size_t size)
{
	if (sSerialEnabled <= 0)
		return;

	if (sBufferPosition + size < sizeof(sBuffer)) {
		memcpy(sBuffer + sBufferPosition, string, size);
		sBufferPosition += size;
	}

	while (size-- != 0) {
		char c = string[0];

		if (c == '\n') {
			serial_putc('\r');
			serial_putc('\n');
		} else if (c != '\r')
			serial_putc(c);

		string++;
	}
}


extern "C" void
serial_disable(void)
{
	sSerialEnabled = 0;
}


extern "C" void
serial_enable(void)
{
	/* should already be initialized by U-Boot */
	/*
	gUART->InitEarly();
	gUART->InitPort(9600);
	*/
	sSerialEnabled++;
}


extern "C" void
serial_cleanup(void)
{
	if (sSerialEnabled <= 0)
		return;

	gKernelArgs.debug_output = kernel_args_malloc(sBufferPosition);
	if (gKernelArgs.debug_output != NULL) {
		memcpy(gKernelArgs.debug_output, sBuffer, sBufferPosition);
		gKernelArgs.debug_size = sBufferPosition;
	}
}


static void
serial_init_fdt(const void *fdt)
{
	const char *name;
	const char *type;
	int node;
	int len;
	phys_addr_t regs;
	int32 clock = 0;
	int32 speed = 0;
	const void *prop;

	if (fdt == NULL)
		return;

	name = fdt_get_alias(fdt, "serial");
	if (name == NULL)
		name = fdt_get_alias(fdt, "serial0");
	if (name == NULL)
		name = fdt_get_alias(fdt, "serial1");
	// TODO: else use /chosen linux,stdout-path
	if (name == NULL)
		return;

	node = fdt_path_offset(fdt, name);
	//dprintf("serial: using '%s', node %d\n", name, node);
	if (node < 0)
		return;

	type = (const char *)fdt_getprop(fdt, node, "device_type", &len);
	//dprintf("serial: type: '%s'\n", type);
	if (type == NULL || strcmp(type, "serial"))
		return;

	// determine the MMIO address
	// TODO: ppc640 use 64bit addressing, but U-Boot seems to map it below 4G,
	// and the FDT is not very clear. libfdt is also getting 64bit addr support.
	// so FIXME someday.
	prop = fdt_getprop(fdt, node, "virtual-reg", &len);
	if (prop && len == 4) {
		regs = fdt32_to_cpu(*(uint32_t *)prop);
		//dprintf("serial: virtual-reg 0x%08llx\n", (int64)regs);
	} else {
		prop = fdt_getprop(fdt, node, "reg", &len);
		if (prop && len >= 4) {
			regs = fdt32_to_cpu(*(uint32_t *)prop);
			//dprintf("serial: reg 0x%08llx\n", (int64)regs);
		} else
			return;
	}

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
		gUART = arch_get_uart_8250(regs, clock);
		//dprintf("serial: using 8250\n");
		return;
	}

}


extern "C" void
serial_init(const void *fdt)
{
	// first try with hints from the FDT
	serial_init_fdt(fdt);

#ifdef BOARD_UART_DEBUG
	// fallback to hardcoded board UART
	if (gUART == NULL)
		gUART = arch_get_uart_8250(BOARD_UART_DEBUG, BOARD_UART_CLOCK);
#endif

	if (gUART == NULL)
		return;

	serial_enable();
}
