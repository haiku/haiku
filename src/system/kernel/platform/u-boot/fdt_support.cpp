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

#ifndef _BOOT_MODE
/* the bootloader has it in asm code to avoid it ending up in .bss */
void *gFDT;
#endif

extern "C" void dump_fdt(const void *fdt);
extern "C" DebugUART *debug_uart_from_fdt(const void *fdt);

//#define FDT_DUMP_NODES
//#define FDT_DUMP_PROPS
//#define FDT_DUMP_PROP_VALUES


#ifdef FDT_DUMP_NODES
static const char *sTabTab = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
#define DS "%.*s"
#define DA depth - 1, sTabTab
#endif


#ifdef FDT_DUMP_PROP_VALUES
static void dump_hex(const char *data, int32 len, int depth)
{
	char str[128];
	char astr[32];
	char *p;
	int l;
	int i;

	for (i = 0; i < len; ) {
		p = str;
		l = sizeof(str);
		for (; i < len && (p == str || (i % 16 != 0)); i++) {
			snprintf(p, l - 1, "%02x ", data[i]);
			l -= strlen(p);
			p += strlen(p);
			astr[i % 16] = isprint(data[i]) ? data[i] : '.';
			astr[i % 16] = isprint(data[i]) ? data[i] : '.';
			astr[(i % 16) + 1] = '\0';
		}
		dprintf(DS"    %-48.48s	%s\n", DA, str, astr);
	}
}
#endif


void dump_fdt(const void *fdt)
{
	int err;

	dprintf("FDT @ %p:\n", fdt);

	if (!fdt)
		return;

	err = fdt_check_header(fdt);
	if (err) {
		dprintf("fdt error: %s\n", fdt_strerror(err));
		return;
	}

	dprintf("fdt_totalsize: %d\n", fdt_totalsize(fdt));
	dprintf("fdt_off_dt_struct: %d\n", fdt_off_dt_struct(fdt));
	dprintf("fdt_off_dt_strings: %d\n", fdt_off_dt_strings(fdt));
	dprintf("fdt_off_mem_rsvmap: %d\n", fdt_off_mem_rsvmap(fdt));
	dprintf("fdt_version: %d\n", fdt_version(fdt));
	dprintf("fdt_last_comp_version: %d\n", fdt_last_comp_version(fdt));
	dprintf("fdt_boot_cpuid_phys: %d\n", fdt_boot_cpuid_phys(fdt));
	dprintf("fdt_size_dt_strings: %d\n", fdt_size_dt_strings(fdt));
	dprintf("fdt_size_dt_struct: %d\n", fdt_size_dt_struct(fdt));

#ifdef FDT_DUMP_NODES
	dprintf("fdt tree:\n");

	int node = -1;
	int depth = 0;
	while ((node = fdt_next_node(fdt, node, &depth)) >= 0) {
		dprintf(DS"node at %d: '%s'\n", DA, node,
			fdt_get_name(fdt, node, NULL));
#ifdef FDT_DUMP_PROPS
		int prop, len;
		const struct fdt_property *property;
		prop = fdt_first_property_offset(fdt, node);
		while (prop >= 0) {
			property = fdt_get_property_by_offset(fdt, prop, &len);
			if (property == NULL) {
				dprintf("getting prop at %d: %s\n", prop, fdt_strerror(len));
				break;
			}
			dprintf(DS"  prop at %d: '%s', len %d\n", DA, prop,
				fdt_string(fdt, fdt32_to_cpu(property->nameoff)),
				fdt32_to_cpu(property->len));
#ifdef FDT_DUMP_PROP_VALUES
			dump_hex(property->data, fdt32_to_cpu(property->len), depth);
#endif
			prop = fdt_next_property_offset(fdt, prop);
		}
#endif
	}
#endif
}


DebugUART *
debug_uart_from_fdt(const void *fdt)
{
	const char *name;
	const char *type;
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
	// TODO: else use /chosen linux,stdout-path
	if (name == NULL)
		return NULL;

	node = fdt_path_offset(fdt, name);
	//dprintf("serial: using '%s', node %d\n", name, node);
	if (node < 0)
		return NULL;

	type = (const char *)fdt_getprop(fdt, node, "device_type", &len);
	//dprintf("serial: type: '%s'\n", type);
	if (type == NULL || strcmp(type, "serial"))
		return NULL;

	// determine the MMIO address
	// TODO: ppc460 use 64bit addressing, but U-Boot seems to map it below 4G,
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
			return NULL;
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
		uart = arch_get_uart_8250(regs, clock);
		//dprintf("serial: using 8250\n");
		// XXX:assume speed is already set
		(void)speed;
	} else {
		// TODO: handle more UART types
		// for when we can use U-Boot's console
		panic("Unknown UART type %s", type);
	}

	return uart;
}

