/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DRIVERS_BUS_FDT_H
#define _DRIVERS_BUS_FDT_H


#include <device_manager.h>


struct fdt_bus;
struct fdt_device;

struct fdt_bus_module_info {
	driver_module_info info;
	device_node* (*node_by_phandle)(fdt_bus* bus, int phandle);
};

struct fdt_device_module_info{
	driver_module_info info;
	device_node* (*get_bus)(fdt_device* dev);
	const char* (*get_name)(fdt_device* dev);
	const void* (*get_prop)(fdt_device* dev, const char* name, int* len);
	bool (*get_reg)(fdt_device* dev, uint32 ord, uint64* regs, uint64* len);
	bool (*get_interrupt)(fdt_device* dev, uint32 ord,
		device_node** interruptController, uint64* interrupt);
};


#endif // _DRIVERS_BUS_FDT_H
