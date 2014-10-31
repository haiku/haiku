/*
 * Copyright 2014, Ithamar R. Adema <ithamar@upgrade-android.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _DRIVERS_BUS_FDT_H
#define _DRIVERS_BUS_FDT_H

#include <bus_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int fdt_device_node;

struct fdt_device_info {
	const char *compatible;
	status_t (*init)(struct fdt_module_info *fdt, fdt_device_node node, void *cookie);
};

struct fdt_module_info {
	bus_manager_info binfo;

	// basic call for triggering callbacks for supported devices
	// scans the whole FDT tree once and calls the info.init function
	// when a matching device is found.
	status_t (*setup_devices)(struct fdt_device_info *info, int count, void *cookie);

	// map physical "reg" range "index" of node "node", and return the virtual address in '*_address'
	// and return the area ID or error if not able to.
	area_id (*map_reg_range)(fdt_device_node node, int index, void **_address);

	// return entry "index" out of "interrupts" property for node "node", or a negative error code on failure.
	int (*get_interrupt)(fdt_device_node node, int index);
};

#define B_FDT_MODULE_NAME	"bus_managers/fdt/v1"


#ifdef __cplusplus
}
#endif

#endif // _DRIVERS_BUS_FDT_H
