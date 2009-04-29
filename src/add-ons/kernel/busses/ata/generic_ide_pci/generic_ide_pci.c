/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*!	Generic PCI bus mastering IDE driver. */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <ide_adapter.h>

#define GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME "busses/ide/generic_ide_pci/driver_v1"
#define GENERIC_IDE_PCI_CHANNEL_MODULE_NAME "busses/ide/generic_ide_pci/channel/v1"

#define IDE_PCI_CONTROLLER_TYPE_NAME "ide pci controller"

ide_for_controller_interface *ide;
static ide_adapter_interface *ide_adapter;
device_manager_info *pnp;


static void
set_channel(void *cookie, ide_channel channel)
{
	ide_adapter->set_channel((ide_adapter_channel_info *)cookie, channel);
}


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
init_channel(device_node *node, void **channel_cookie)
{
	return ide_adapter->init_channel(node,
		(ide_adapter_channel_info **)channel_cookie,
		sizeof(ide_adapter_channel_info), ide_adapter->inthand);
}


static void
uninit_channel(void *channel_cookie)
{
	ide_adapter->uninit_channel((ide_adapter_channel_info *)channel_cookie);
}


static void
channel_removed(void *channel_cookie)
{
	ide_adapter->channel_removed((ide_adapter_channel_info *)channel_cookie);
}


static status_t
init_controller(device_node *node, ide_adapter_controller_info **cookie)
{
	return ide_adapter->init_controller(node, cookie,
		sizeof( ide_adapter_controller_info));
}


static void
uninit_controller(ide_adapter_controller_info *controller)
{
	ide_adapter->uninit_controller(controller);
}


static void
controller_removed(ide_adapter_controller_info *controller)
{
	return ide_adapter->controller_removed(controller);
}


static status_t
probe_controller(device_node *parent)
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
supports_device(device_node *parent)
{
	const char *bus;
	uint16 baseClass, subClass;

	// make sure parent is an PCI IDE mass storage host adapter device node
	if (pnp->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| pnp->get_attr_uint16(parent, B_DEVICE_TYPE, &baseClass, false) != B_OK
		|| pnp->get_attr_uint16(parent, B_DEVICE_SUB_TYPE, &subClass, false) != B_OK)
		return -1;

	if (strcmp(bus, "pci") || baseClass != PCI_mass_storage)
		return 0.0f;

	if (subClass == PCI_ide)
		return 0.3f;

	// vendor 105a: Promise Technology, Inc.; device 4d69: 20269 (Ultra133TX2)
	// has subClass set to PCI_mass_storage_other, and there are others as well.
	if (subClass == PCI_mass_storage_other)
		return 0.3f;

	return 0.0f;
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
			NULL
		},

		NULL,	// supports device
		NULL,	// register device
		init_channel,
		uninit_channel,
		NULL,	// register child devices
		NULL,	// rescan
		channel_removed,
	},

	&set_channel,

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
		NULL
	},

	supports_device,
	probe_controller,
	(status_t (*)(device_node *, void **))	init_controller,
	(void (*)(void *))						uninit_controller,
	NULL,	// register child devices
	NULL,	// rescan
	(void (*)(void *))						controller_removed,
};

module_info *modules[] = {
	(module_info *)&controller_interface,
	(module_info *)&channel_interface,
	NULL
};
