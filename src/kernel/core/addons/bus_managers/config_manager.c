/* config_manager.c
 * 
 * Module that provides access to driver modules and busses
 */

#include <ktypes.h> 
#include <config_manager.h>
#include <PCI.h>
#include <ISA.h>
#include <bus_manager.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <memheap.h>
#include <KernelExport.h>

static pci_module_info *pcim = NULL;

#define B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME "bus_managers/config_manager/bus/v1"

static status_t cfdm_get_next_device_info(bus_type bus, uint64 *cookie,
                                          struct device_info *info, uint32 len)
{
	dprintf("get_next_device_info(bus = %d, cookie = %lld)\n",
	        bus, *cookie);
	return 0;
}
	
/* device_modules */
static int32 cfdm_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			dprintf( "config_manager: device modules: init\n" );
			if (get_module(B_PCI_MODULE_NAME, (module_info**)&pcim) != 0) {
				dprintf("config_manager: failed to load PCI module\n");
				return -1;
			}
			break;
		case B_MODULE_UNINIT:
			dprintf( "config_manager: device modules: uninit\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}

/* bus_modules */
static int32 cfbm_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			dprintf( "config_manager: bus modules: init\n" );
			break;
		case B_MODULE_UNINIT:
			dprintf( "config_manager: bus modules: uninit\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}

/* cfdm = configuration_manager_device_modules */
struct config_manager_for_driver_module_info cfdm = {
	{
		B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME,
		B_KEEP_LOADED,
		cfdm_std_ops
	},
	
	&cfdm_get_next_device_info, /* get_next_device_info */
	NULL, /* get_device_info_for */
	NULL, /* get_size_of_current_configuration_for */
	NULL, /* get_current_configuration_for */
	NULL, /* get_size_of_possible_configurations */
	NULL, /* get_possible_configurations_for */
	
	NULL, /* count_resource_descriptors_of_type */
	NULL, /* get_nth_resource_descriptor_of_type */
};

/* cfbm = configuration_manager_bus_modules */
struct config_manager_for_driver_module_info cfbm = {
	{
		B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME,
		B_KEEP_LOADED,
		cfbm_std_ops
	},
	
	NULL, /* get_next_device_info */
	NULL, /* get_device_info_for */
	NULL, /* get_size_of_current_configuration_for */
	NULL, /* get_current_configuration_for */
	NULL, /* get_size_of_possible_configurations */
	NULL, /* get_possible_configurations_for */
	
	NULL, /* count_resource_descriptors_of_type */
	NULL, /* get_nth_resource_descriptor_of_type */
};

module_info *modules[] = {
	(module_info*)&cfdm,
	(module_info*)&cfbm,
	NULL
};
