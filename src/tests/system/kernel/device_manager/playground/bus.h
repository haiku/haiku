/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BUS_H
#define BUS_H


#include "device_manager.h"


struct bus_info {
	uint16	vendor_id;
	uint16	device_id;
};

struct bus_for_driver_module_info {
	driver_module_info info;

	status_t (*get_bus_info)(void* cookie, bus_info* info);
};

// Note: this file is also used by the device manager test to control the bus
// driver

#define BUS_MODULE_NAME "bus_managers/sample_bus/driver_v1"
#define BUS_FOR_DRIVER_NAME "bus_managers/sample_bus/device/driver_v1"
#define BUS_NAME "mybus"

extern void bus_trigger_device_removed(device_node* node);
extern void bus_trigger_device_added(device_node* node);

#endif	// BUS_H
