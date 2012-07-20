/* Config Manager
 * provides access to device configurations
 *
 * Copyright 2002-2004, Marcus Overhagen, marcus@overhagen.de.
 * Distributed under the terms of the MIT License.
 */


#include <config_manager.h>
#include <PCI.h>
#include <ISA.h>
#include <bus_manager.h>
#include <string.h>
#include <heap.h>
#include <KernelExport.h>


static pci_module_info *gPCI = NULL;

#define B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME "bus_managers/config_manager/bus/v1"

#define FUNCTION(x, y...) dprintf("%s" x, __FUNCTION__, y)
#define TRACE(x) dprintf x


//	Driver module API


static status_t
driver_get_next_device_info(bus_type bus, uint64 *cookie, struct device_info *info, uint32 size)
{
	FUNCTION("(bus = %d, cookie = %" B_PRId64 ")\n", bus, *cookie);
	return B_ENTRY_NOT_FOUND;
}


static status_t
driver_get_device_info_for(uint64 id, struct device_info *info, uint32 size)
{
	FUNCTION("(id = %" B_PRId64 ")\n", id);
	return B_ENTRY_NOT_FOUND;
}


static status_t
driver_get_size_of_current_configuration_for(uint64 id)
{
	FUNCTION("(id = %" B_PRId64 ")\n", id);
	return B_ENTRY_NOT_FOUND;
}


static status_t
driver_get_current_configuration_for(uint64 id, struct device_configuration *current, uint32 size)
{
	FUNCTION("(id = %" B_PRId64 ", current = %p, size = %" B_PRIu32 ")\n", id,
		current, size);
	return B_ENTRY_NOT_FOUND;
}


static status_t
driver_get_size_of_possible_configurations_for(uint64 id)
{
	FUNCTION("(id = %" B_PRId64 ")\n", id);
	return B_ENTRY_NOT_FOUND;
}


static status_t
driver_get_possible_configurations_for(uint64 id, struct possible_device_configurations *possible, uint32 size)
{
	FUNCTION("(id = %" B_PRId64 ", possible = %p, size = %" B_PRIu32 ")\n", id,
		possible, size);
	return B_ENTRY_NOT_FOUND;
}


static status_t
driver_count_resource_descriptors_of_type(const struct device_configuration *config, resource_type type)
{
	FUNCTION("(config = %p, type = %d)\n", config, type);
	return B_ENTRY_NOT_FOUND;
}


static status_t
driver_get_nth_resource_descriptor_of_type(const struct device_configuration *config, uint32 num,
	resource_type type, resource_descriptor *descr, uint32 size)
{
	FUNCTION("(config = %p, num = %" B_PRId32 ")\n", config, num);
	return B_ENTRY_NOT_FOUND;
}


static int32
driver_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE(("config_manager: driver module: init\n"));

			// ToDo: should not do this! Instead, iterate through all busses/config_manager/
			//	modules and get them
			if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPCI) != 0) {
				TRACE(("config_manager: failed to load PCI module\n"));
				return -1;
			}
			break;
		case B_MODULE_UNINIT:
			TRACE(("config_manager: driver module: uninit\n"));
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


//	#pragma mark -
//	Bus module API


static int32
bus_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			TRACE(("config_manager: bus module: init\n"));
			break;
		case B_MODULE_UNINIT:
			TRACE(("config_manager: bus module: uninit\n"));
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


struct config_manager_for_driver_module_info gDriverModuleInfo = {
	{
		B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME,
		B_KEEP_LOADED,
		driver_std_ops
	},

	&driver_get_next_device_info,
	&driver_get_device_info_for,
	&driver_get_size_of_current_configuration_for,
	&driver_get_current_configuration_for,
	&driver_get_size_of_possible_configurations_for,
	&driver_get_possible_configurations_for,
	
	&driver_count_resource_descriptors_of_type,
	&driver_get_nth_resource_descriptor_of_type,
};

struct module_info gBusModuleInfo = {
	//{
		B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME,
		B_KEEP_LOADED,
		bus_std_ops
	//},

	// ToDo: find out what's in here!
};

module_info *modules[] = {
	(module_info *)&gDriverModuleInfo,
	(module_info *)&gBusModuleInfo,
	NULL
};
