/*
 * Copyright 2014, Ithamar R. Adema <ithamar@upgrade-android.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <drivers/bus/FDT.h>
#include <KernelExport.h>
#include <util/kernel_cpp.h>

#include <ctype.h> // isprint
#include <stdio.h> // snprintf

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

extern void *gFDT;

static status_t fdt_setup_devices(struct fdt_device_info *info, int count, void *cookie);

static const char *sTabTab = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
#define DS "%.*s"
#define DA depth - 1, sTabTab

static void
fdt_dump_value(const char *data, int32 len, int depth)
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

static int
fdt_debug_tree(int argc, char **argv)
{
	bool dump_props = false, dump_values = false;

	if (gFDT == NULL) {
		dprintf("No fdt tree\n");
		return 0;
	}

	dprintf("fdt tree:\n");

	int node = -1;
	int depth = 0;
	while ((node = fdt_next_node(gFDT, node, &depth)) >= 0) {
		dprintf(DS"node at %d: '%s'\n", DA, node,
			fdt_get_name(gFDT, node, NULL));

		if (dump_props) {
			int prop, len;
			const struct fdt_property *property;
			prop = fdt_first_property_offset(gFDT, node);
			while (prop >= 0) {
				property = fdt_get_property_by_offset(gFDT, prop, &len);
				if (property == NULL) {
					dprintf("getting prop at %d: %s\n", prop, fdt_strerror(len));
					break;
				}
				dprintf(DS"  prop at %d: '%s', len %d\n", DA, prop,
					fdt_string(gFDT, fdt32_to_cpu(property->nameoff)),
					fdt32_to_cpu(property->len));

				if (dump_values)
					fdt_dump_value(property->data, fdt32_to_cpu(property->len), depth);

				prop = fdt_next_property_offset(gFDT, prop);
			}
		}
	}

	return 0;
}


static int32
bus_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			if (gFDT == NULL || fdt_totalsize(gFDT) <= 0)
				return B_BAD_DATA;

			add_debugger_command("fdt", &fdt_debug_tree, "Show Flattened Device Tree");
			break;

		case B_MODULE_UNINIT:
			// Nothing to free, gFDT allocation is managed by kernel
			break;

		default:
			return EINVAL;
	}

	return B_OK;
}

static int
fdt_get_interrupt(fdt_device_node node, int index)
{
	const struct fdt_property *prop;
	int lenp;

	prop = fdt_get_property(gFDT, node, "interrupts", &lenp);
	if (prop == NULL)
		return B_NAME_NOT_FOUND;

	int numRanges = lenp / sizeof(uint32);
	if (index > numRanges)
		return B_BAD_INDEX;

	return fdt32_to_cpu(((uint32*)prop->data)[index]);
}

static area_id
fdt_map_reg_range(fdt_device_node node, int index, void **_address)
{
	char name[B_OS_NAME_LENGTH] = "ingo_asked_me_to_name_this";
	const struct fdt_property *prop;
	int lenp;

	prop = fdt_get_property(gFDT, node, "reg", &lenp);
	if (prop == NULL)
		return B_NAME_NOT_FOUND;

	int numRanges = lenp / (sizeof(uint32) * 2);
	if (index > numRanges)
		return B_BAD_INDEX;

	//snprintf(name, sizeof(name), "%s_reg_%d", fdt_get_name(gFDT, node, NULL), index);

	uint32* regs = (uint32*)prop->data;
	phys_addr_t rangeStart = (phys_addr_t)fdt32_to_cpu(regs[index*2]);
	uint32 rangeSize = fdt32_to_cpu(regs[index*2+1]);
	dprintf("fdt_map_reg_range: found reg range %p/%lu\n", (void*)rangeStart, rangeSize);

	return map_physical_memory(name, rangeStart, rangeSize,
		0, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)_address);
}

static struct fdt_module_info sModuleInfo = {
	// First the bus_manager_info:
	{
		{
			"bus_managers/fdt/v1",
			B_KEEP_LOADED,		// Keep loaded, even if no driver requires it
			bus_std_ops
		},
		NULL				// the rescan function
	},

	fdt_setup_devices,
	fdt_map_reg_range,
	fdt_get_interrupt,
};


module_info *modules[] = {
	(module_info *)&sModuleInfo,
	NULL
};

static status_t
fdt_setup_devices(struct fdt_device_info *info, int count, void *cookie)
{
	int numDevs = 0;

	if (gFDT == NULL)
		return B_NOT_INITIALIZED;

	int node = -1;
	int depth = 0;
	while ((node = fdt_next_node(gFDT, node, &depth)) >= 0) {
		for (int i=0; i < count; i++) {
			if (fdt_node_check_compatible(gFDT, node, info[i].compatible) == 0) {
				status_t result = info[i].init(&sModuleInfo, node, cookie);
				if (result != B_OK) {
					// TODO handle return value from init somehow?
					dprintf("fdt: device '%s' failed to initialize!\n",
						fdt_get_name(gFDT, node, NULL));
				} else {
					++numDevs;
				}
			}
		}
	}

	return (numDevs <= 0) ? B_ENTRY_NOT_FOUND : B_OK;
}


