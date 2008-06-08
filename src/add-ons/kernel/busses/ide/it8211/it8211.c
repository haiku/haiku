/*
 * Copyright 2008, Michael Lotz. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <ide_adapter.h>

#define IT8211_CONTROLLER_MODULE_NAME "busses/ide/it8211/driver_v1"
#define IT8211_CHANNEL_MODULE_NAME "busses/ide/it8211/channel/v1"

#define PCI_VENDOR_ITE			0x1283
#define PCI_DEVICE_ITE_IT8211	0x8211

static ide_adapter_interface *sIDEAdapter;
static device_manager_info *sDeviceManager;


static void
it8211_set_channel(void *channelCookie, ide_channel channel)
{
	sIDEAdapter->set_channel((ide_adapter_channel_info *)channelCookie,
		channel);
}


static status_t
it8211_write_command_block_regs(void *channelCookie, ide_task_file *taskFile,
	ide_reg_mask registerMask)
{
	return sIDEAdapter->write_command_block_regs(
		(ide_adapter_channel_info *)channelCookie, taskFile, registerMask);
}


static status_t
it8211_read_command_block_regs(void *channelCookie, ide_task_file *taskFile,
	ide_reg_mask registerMask)
{
	return sIDEAdapter->read_command_block_regs(
		(ide_adapter_channel_info *)channelCookie, taskFile, registerMask);
}


static uint8
it8211_get_altstatus(void *channelCookie)
{
	return sIDEAdapter->get_altstatus((ide_adapter_channel_info *)channelCookie);
}


static status_t
it8211_write_device_control(void *channelCookie, uint8 value)
{
	return sIDEAdapter->write_device_control(
		(ide_adapter_channel_info *)channelCookie, value);
}


static status_t
it8211_write_pio(void *channelCookie, uint16 *data, int count, bool force16bit)
{
	return sIDEAdapter->write_pio(
		(ide_adapter_channel_info *)channelCookie, data, count, force16bit);
}


static status_t
it8211_read_pio(void *channelCookie, uint16 *data, int count, bool force16bit)
{
	return sIDEAdapter->read_pio(
		(ide_adapter_channel_info *)channelCookie, data, count, force16bit);
}


static status_t
it8211_prepare_dma(void *channelCookie, const physical_entry *sgList,
	size_t sgListCount, bool toDevice)
{
	return sIDEAdapter->prepare_dma((ide_adapter_channel_info *)channelCookie,
		sgList, sgListCount, toDevice);
}


static status_t
it8211_start_dma(void *channelCookie)
{
	return sIDEAdapter->start_dma((ide_adapter_channel_info *)channelCookie);
}


static status_t
it8211_finish_dma(void *channelCookie)
{
	return sIDEAdapter->finish_dma((ide_adapter_channel_info *)channelCookie);
}


static status_t
init_channel(device_node *node, void **channelCookie)
{
	return sIDEAdapter->init_channel(node,
		(ide_adapter_channel_info **)channelCookie,
		sizeof(ide_adapter_channel_info), sIDEAdapter->inthand);
}


static void
uninit_channel(void *channelCookie)
{
	sIDEAdapter->uninit_channel((ide_adapter_channel_info *)channelCookie);
}


static void
channel_removed(void *channelCookie)
{
	sIDEAdapter->channel_removed((ide_adapter_channel_info *)channelCookie);
}


static status_t
init_controller(device_node *node, void **cookie)
{
	return sIDEAdapter->init_controller(node,
		(ide_adapter_controller_info **)cookie,
		sizeof(ide_adapter_controller_info));
}


static void
uninit_controller(void *cookie)
{
	sIDEAdapter->uninit_controller((ide_adapter_controller_info *)cookie);
}


static void
controller_removed(void *cookie)
{
	sIDEAdapter->controller_removed((ide_adapter_controller_info *)cookie);
}


static status_t
probe_controller(device_node *parent)
{
	return sIDEAdapter->probe_controller(parent,
		IT8211_CONTROLLER_MODULE_NAME, "it8211",
		"ITE IT8211", IT8211_CHANNEL_MODULE_NAME,
		true,		/* DMA */
		true,		/* command queuing */
		1,			/* 16 bit alignment */
		0xffff,		/* 64k boundary */
		0x10000,	/* up to 64k per scatter/gather block */
		false);		/* no compatibility mode */
}


static float
supports_device(device_node *parent)
{
	const char *bus;
	uint16 vendorID;
	uint16 deviceID;

	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendorID, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID, false) != B_OK) {
		return -1.0f;
	}

	if (strcmp(bus, "pci") != 0 || vendorID != PCI_VENDOR_ITE
		|| deviceID != PCI_DEVICE_ITE_IT8211)
		return 0.0f;

	return 1.0f;
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ IDE_ADAPTER_MODULE_NAME, (module_info **)&sIDEAdapter },
	{}
};


// exported interface
static ide_controller_interface sChannelInterface = {
	{
		{
			IT8211_CHANNEL_MODULE_NAME,
			0,
			NULL
		},

		NULL,	// supports device
		NULL,	// register device
		&init_channel,
		&uninit_channel,
		NULL,	// register child devices
		NULL,	// rescan
		&channel_removed
	},

	&it8211_set_channel,
	&it8211_write_command_block_regs,
	&it8211_read_command_block_regs,
	&it8211_get_altstatus,
	&it8211_write_device_control,
	&it8211_write_pio,
	&it8211_read_pio,
	&it8211_prepare_dma,
	&it8211_start_dma,
	&it8211_finish_dma
};


static driver_module_info sControllerInterface = {
	{
		IT8211_CONTROLLER_MODULE_NAME,
		0,
		NULL
	},

	&supports_device,
	&probe_controller,
	&init_controller,
	&uninit_controller,
	NULL,	// register child devices
	NULL,	// rescan
	&controller_removed
};


module_info *modules[] = {
	(module_info *)&sControllerInterface,
	(module_info *)&sChannelInterface,
	NULL
};
