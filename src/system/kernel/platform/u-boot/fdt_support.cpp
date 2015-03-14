/*
 * Copyright 2012, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

#include "fdt_support.h"

#include <KernelExport.h>
#include <ByteOrder.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/cdefs.h>

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

//#define TRACE_FDT
#ifdef TRACE_FDT
#   define TRACE(x) dprintf x
#else
#   define TRACE(x) ;
#endif

//#define FDT_DUMP_NODES
//#define FDT_DUMP_PROPS
//#define FDT_DUMP_PROP_VALUES


#ifdef FDT_DUMP_NODES
static const char *sTabTab = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
#define DS "%.*s"
#define DA depth - 1, sTabTab
#endif


#ifndef _BOOT_MODE
/* the bootloader has it in asm code to avoid it ending up in .bss */
void *gFDT;
#else
extern void *gFDT;
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


status_t
fdt_get_cell_count(int32 pathOffset, int32 &addressCells, int32 &sizeCells)
{
	// It would be nice if libfdt provided this.

	// Memory base addresses are provided in 32 or 64 bit flavors
	// #address-cells and #size-cells matches the number of 32-bit 'cells'
	// representing the length of the base address and size fields

	// TODO: assert !gFDT || !pathOffset?

	int len;
	if (!pathOffset) {
		TRACE(("%s: Invalid FDT pathOffset provided!\n", __func__));
		return B_ERROR;
	}

	const void *prop;
	prop = fdt_getprop(gFDT, pathOffset, "#address-cells", &len);
	if (prop && len == sizeof(uint32))
		addressCells = fdt32_to_cpu(*(uint32_t *)prop);
	prop = fdt_getprop(gFDT, pathOffset, "#size-cells", &len);
	if (prop && len == sizeof(uint32))
		sizeCells = fdt32_to_cpu(*(uint32_t *)prop);

	// NOTE : Cells over 2 is possible in theory... 
	if (addressCells > 2 || sizeCells > 2) {
		panic("%s: Unsupported FDT cell count detected.\n"
			"Address Cells: %" B_PRId32 "; Size Cells: %" B_PRId32
			" (CPU > 64bit?).\n", __func__, addressCells, sizeCells);
		return B_ERROR;
	}

	return B_OK;
}
