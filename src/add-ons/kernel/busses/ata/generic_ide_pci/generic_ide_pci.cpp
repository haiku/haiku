/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*!	Generic PCI bus mastering IDE driver. */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <ata_adapter.h>

#define GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME "busses/ata/generic_ide_pci/driver_v1"
#define GENERIC_IDE_PCI_CHANNEL_MODULE_NAME "busses/ata/generic_ide_pci/channel/v1"

#define ATA_PCI_CONTROLLER_TYPE_NAME "ata pci controller"

ata_for_controller_interface *sATA;
static ata_adapter_interface *sATAAdapter;
device_manager_info *sDeviceManager;


static void
set_channel(void *cookie, ata_channel channel)
{
	sATAAdapter->set_channel((ata_adapter_channel_info *)cookie, channel);
}


static status_t
write_command_block_regs(void *channel_cookie, ata_task_file *tf, ata_reg_mask mask)
{
	return sATAAdapter->write_command_block_regs((ata_adapter_channel_info *)channel_cookie, tf, mask);
}


static status_t
read_command_block_regs(void *channel_cookie, ata_task_file *tf, ata_reg_mask mask)
{
	return sATAAdapter->read_command_block_regs((ata_adapter_channel_info *)channel_cookie, tf, mask);
}


static uint8
get_altstatus(void *channel_cookie)
{
	return sATAAdapter->get_altstatus((ata_adapter_channel_info *)channel_cookie);
}


static status_t
write_device_control(void *channel_cookie, uint8 val)
{
	return sATAAdapter->write_device_control((ata_adapter_channel_info *)channel_cookie, val);
}


static status_t
write_pio(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	return sATAAdapter->write_pio((ata_adapter_channel_info *)channel_cookie, data, count, force_16bit);
}


static status_t
read_pio(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	return sATAAdapter->read_pio((ata_adapter_channel_info *)channel_cookie, data, count, force_16bit);
}


static status_t
prepare_dma(void *channel_cookie,
	const physical_entry *sg_list, size_t sg_list_count,
	bool to_device)
{
	return sATAAdapter->prepare_dma((ata_adapter_channel_info *)channel_cookie, sg_list, sg_list_count, to_device);
}


static status_t
start_dma(void *channel_cookie)
{
	return sATAAdapter->start_dma((ata_adapter_channel_info *)channel_cookie);
}


static status_t
finish_dma(void *channel_cookie)
{
	return sATAAdapter->finish_dma((ata_adapter_channel_info *)channel_cookie);
}


static status_t
init_channel(device_node *node, void **channel_cookie)
{
	return sATAAdapter->init_channel(node,
		(ata_adapter_channel_info **)channel_cookie,
		sizeof(ata_adapter_channel_info), sATAAdapter->inthand);
}


static void
uninit_channel(void *channel_cookie)
{
	sATAAdapter->uninit_channel((ata_adapter_channel_info *)channel_cookie);
}


static void
channel_removed(void *channel_cookie)
{
	sATAAdapter->channel_removed((ata_adapter_channel_info *)channel_cookie);
}


static status_t
init_controller(device_node *node, ata_adapter_controller_info **cookie)
{
	return sATAAdapter->init_controller(node, cookie,
		sizeof( ata_adapter_controller_info));
}


static void
uninit_controller(ata_adapter_controller_info *controller)
{
	sATAAdapter->uninit_controller(controller);
}


static void
controller_removed(ata_adapter_controller_info *controller)
{
	return sATAAdapter->controller_removed(controller);
}


static status_t
probe_controller(device_node *parent)
{
	return sATAAdapter->probe_controller(parent,
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
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_TYPE, &baseClass, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_SUB_TYPE, &subClass, false) != B_OK)
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
	{ ATA_FOR_CONTROLLER_MODULE_NAME, (module_info **)&sATA },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ ATA_ADAPTER_MODULE_NAME, (module_info **)&sATAAdapter },
	{}
};


// exported interface
static ata_controller_interface channel_interface = {
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
