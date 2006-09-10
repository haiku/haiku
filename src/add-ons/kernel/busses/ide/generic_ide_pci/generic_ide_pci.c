/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Generic IDE PCI controller driver

	Generic PCI bus mastering IDE driver.
*/

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <bus/ide/ide_adapter.h>

#define debug_level_flow 0
#define debug_level_error 3
#define debug_level_info 3

#define DEBUG_MSG_PREFIX "GENERIC IDE PCI -- "

#include "wrapper.h"

#define GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME "busses/ide/generic_ide_pci/device_v1"
#define GENERIC_IDE_PCI_CHANNEL_MODULE_NAME "busses/ide/generic_ide_pci/channel/v1"

#define IDE_PCI_CONTROLLER_TYPE_NAME "ide pci controller"

ide_for_controller_interface *ide;
static ide_adapter_interface *ide_adapter;
device_manager_info *pnp;


static status_t
write_command_block_regs(void *channel_cookie, ide_task_file *tf, ide_reg_mask mask)
{
	return ide_adapter->write_command_block_regs((ide_adapter_channel_info *)channel_cookie, tf, mask);
}


static status_t
read_command_block_regs(void *channel_cookie, ide_task_file *tf, ide_reg_mask mask)
{
	return ide_adapter->read_command_block_regs((ide_adapter_channel_info *)channel_cookie, tf, mask);
}


static uint8
get_altstatus(void *channel_cookie)
{
	return ide_adapter->get_altstatus((ide_adapter_channel_info *)channel_cookie);
}


static status_t
write_device_control(void *channel_cookie, uint8 val)
{
	return ide_adapter->write_device_control((ide_adapter_channel_info *)channel_cookie, val);
}


static status_t
write_pio(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	return ide_adapter->write_pio((ide_adapter_channel_info *)channel_cookie, data, count, force_16bit);
}


static status_t
read_pio(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	return ide_adapter->read_pio((ide_adapter_channel_info *)channel_cookie, data, count, force_16bit);
}


static status_t
prepare_dma(void *channel_cookie, 
	const physical_entry *sg_list, size_t sg_list_count,
	bool to_device)
{
	return ide_adapter->prepare_dma((ide_adapter_channel_info *)channel_cookie, sg_list, sg_list_count, to_device);
}


static status_t
start_dma(void *channel_cookie)
{
	return ide_adapter->start_dma((ide_adapter_channel_info *)channel_cookie);
}


static status_t
finish_dma(void *channel_cookie)
{
	return ide_adapter->finish_dma((ide_adapter_channel_info *)channel_cookie);
}


static status_t
init_channel(device_node_handle node, ide_channel ide_channel, 
	void **channel_cookie)
{
	return ide_adapter->init_channel(node, ide_channel, 
		(ide_adapter_channel_info **)channel_cookie, 
		sizeof(ide_adapter_channel_info), ide_adapter->inthand);
}


static status_t
uninit_channel(void *channel_cookie)
{
	return ide_adapter->uninit_channel((ide_adapter_channel_info *)channel_cookie);
}


static void
channel_removed(device_node_handle node, void *channel_cookie)
{
	return ide_adapter->channel_removed(node, (ide_adapter_channel_info *)channel_cookie);
}


static status_t
init_controller(device_node_handle node, void *user_cookie, 
	ide_adapter_controller_info **cookie)
{
	return ide_adapter->init_controller(node, user_cookie, cookie, 
		sizeof( ide_adapter_controller_info));
}


static status_t
uninit_controller(ide_adapter_controller_info *controller)
{
	return ide_adapter->uninit_controller(controller);
}


static void
controller_removed(device_node_handle node, ide_adapter_controller_info *controller)
{
	return ide_adapter->controller_removed(node, controller);
}


static status_t
probe_controller(device_node_handle parent)
{
	return ide_adapter->probe_controller(parent,
		GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME, "generic_ide_pci",
		"Generic IDE PCI Controller", 
		GENERIC_IDE_PCI_CHANNEL_MODULE_NAME,
		true,
		true,					// assume that command queuing works
		1,						// assume 16 bit alignment is enough
		0xffff,					// boundary is on 64k according to spec
		0x10000,				// up to 64k per S/G block according to spec
		true);					// by default, compatibility mode is used
}


static float
supports_device(device_node_handle parent, bool *_noConnection)
{
	char *bus;
	uint8 baseClass, subClass;

	// make sure parent is an PCI IDE mass storage host adapter device node
	if (pnp->get_attr_string(parent, B_DRIVER_BUS, &bus, false) != B_OK
		|| pnp->get_attr_uint8(parent, PCI_DEVICE_BASE_CLASS_ID_ITEM, &baseClass, false) != B_OK
		|| pnp->get_attr_uint8(parent, PCI_DEVICE_SUB_CLASS_ID_ITEM, &subClass, false) != B_OK)
		return B_ERROR;

	if (strcmp(bus, "pci") || baseClass != PCI_mass_storage || subClass != PCI_ide) {
		free(bus);
		return 0.0;
	}

	free(bus);
	return 0.3;
}


static void
get_paths(const char ***_bus, const char ***_device)
{
	static const char *kBus[] = { "pci", NULL };
	static const char *kDevice[] = { "drivers/dev/disk/ide", NULL };

	*_bus = kBus;
	*_device = kDevice;
}


static status_t
module_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{ IDE_FOR_CONTROLLER_MODULE_NAME, (module_info **)&ide },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{ IDE_ADAPTER_MODULE_NAME, (module_info **)&ide_adapter },
	{}
};


// exported interface
static ide_controller_interface channel_interface = {
	{
		{
			GENERIC_IDE_PCI_CHANNEL_MODULE_NAME,
			0,
			module_std_ops
		},

		NULL,	// supports device
		NULL,	// register device
		(status_t (*)(device_node_handle, void *, void **))init_channel,
		uninit_channel,
		channel_removed,
		NULL,	// cleanup
		NULL,	// get_paths
	},

	&write_command_block_regs,
	&read_command_block_regs,

	&get_altstatus,
	&write_device_control,

	&write_pio,
	&read_pio,

	&prepare_dma,
	&start_dma,
	&finish_dma,
};


static driver_module_info controller_interface = {
	{
		GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME,
		0,
		module_std_ops
	},

	supports_device,
	probe_controller,
	(status_t (*)(device_node_handle, void *, void **))	init_controller,
	(status_t (*)(void *))								uninit_controller,
	(void (*)(device_node_handle, void *))				controller_removed,
	NULL,	// cleanup
	get_paths,
};

module_info *modules[] = {
	(module_info *)&controller_interface,
	(module_info *)&channel_interface,
	NULL
};
