/*
 * Copyright 2012, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

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


extern "C" void dump_fdt(const void *fdt);

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


