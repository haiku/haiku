/*
 * Copyright 2008, Michael Lotz. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <ata_adapter.h>

#define IT8211_CONTROLLER_MODULE_NAME "busses/ata/it8211/driver_v1"
#define IT8211_CHANNEL_MODULE_NAME "busses/ata/it8211/channel/v1"

#define PCI_VENDOR_ITE			0x1283
#define PCI_DEVICE_ITE_IT8211	0x8211

static ata_adapter_interface *sATAAdapter;
static device_manager_info *sDeviceManager;


static void
it8211_set_channel(void *channelCookie, ata_channel channel)
{
	sATAAdapter->set_channel((ata_adapter_channel_info *)channelCookie,
		channel);
}


static status_t
it8211_write_command_block_regs(void *channelCookie, ata_task_file *taskFile,
	ata_reg_mask registerMask)
{
	return sATAAdapter->write_command_block_regs(
		(ata_adapter_channel_info *)channelCookie, taskFile, registerMask);
}


static status_t
it8211_read_command_block_regs(void *channelCookie, ata_task_file *taskFile,
	ata_reg_mask registerMask)
{
	return sATAAdapter->read_command_block_regs(
		(ata_adapter_channel_info *)channelCookie, taskFile, registerMask);
}


static uint8
it8211_get_altstatus(void *channelCookie)
{
	return sATAAdapter->get_altstatus((ata_adapter_channel_info *)channelCookie);
}


static status_t
it8211_write_device_control(void *channelCookie, uint8 value)
{
	return sATAAdapter->write_device_control(
		(ata_adapter_channel_info *)channelCookie, value);
}


static status_t
it8211_write_pio(void *channelCookie, uint16 *data, int count, bool force16bit)
{
	return sATAAdapter->write_pio(
		(ata_adapter_channel_info *)channelCookie, data, count, force16bit);
}


static status_t
it8211_read_pio(void *channelCookie, uint16 *data, int count, bool force16bit)
{
	return sATAAdapter->read_pio(
		(ata_adapter_channel_info *)channelCookie, data, count, force16bit);
}


static status_t
it8211_prepare_dma(void *channelCookie, const physical_entry *sgList,
	size_t sgListCount, bool toDevice)
{
	return sATAAdapter->prepare_dma((ata_adapter_channel_info *)channelCookie,
		sgList, sgListCount, toDevice);
}


static status_t
it8211_start_dma(void *channelCookie)
{
	return sATAAdapter->start_dma((ata_adapter_channel_info *)channelCookie);
}


static status_t
it8211_finish_dma(void *channelCookie)
{
	return sATAAdapter->finish_dma((ata_adapter_channel_info *)channelCookie);
}


static status_t
init_channel(device_node *node, void **channelCookie)
{
	return sATAAdapter->init_channel(node,
		(ata_adapter_channel_info **)channelCookie,
		sizeof(ata_adapter_channel_info), sATAAdapter->inthand);
}


static void
uninit_channel(void *channelCookie)
{
	sATAAdapter->uninit_channel((ata_adapter_channel_info *)channelCookie);
}


static void
channel_removed(void *channelCookie)
{
	sATAAdapter->channel_removed((ata_adapter_channel_info *)channelCookie);
}


static status_t
init_controller(device_node *node, void **cookie)
{
	return sATAAdapter->init_controller(node,
		(ata_adapter_controller_info **)cookie,
		sizeof(ata_adapter_controller_info));
}


static void
uninit_controller(void *cookie)
{
	sATAAdapter->uninit_controller((ata_adapter_controller_info *)cookie);
}


static void
controller_removed(void *cookie)
{
	sATAAdapter->controller_removed((ata_adapter_controller_info *)cookie);
}


static status_t
probe_controller(device_node *parent)
{
	return sATAAdapter->probe_controller(parent,
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
	{ ATA_ADAPTER_MODULE_NAME, (module_info **)&sATAAdapter },
	{}
};


// exported interface
static ata_controller_interface sChannelInterface = {
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
