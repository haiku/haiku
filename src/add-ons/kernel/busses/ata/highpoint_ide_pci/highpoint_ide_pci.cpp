/*
 * Copyright 2017, Alexander Coers. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "highpoint_ata.h"

#include <ata_adapter.h>
#include <malloc.h>
#include <KernelExport.h>

#include <stdlib.h>
#include <string.h>


#define TRACE(a...)			dprintf("Highpoint-IDE: " a)

//#define TRACE_EXT_HIGHPOINT
#ifdef TRACE_EXT_HIGHPOINT
#define TRACE_EXT(a...)		dprintf("Highpoint-IDE (ext): " a)
#else
#define TRACE_EXT(a...)
#endif


#define HIGHPOINT_IDE_PCI_CONTROLLER_MODULE_NAME "busses/ata/highpoint_ide_pci/driver_v1"
#define HIGHPOINT_IDE_PCI_CHANNEL_MODULE_NAME    "busses/ata/highpoint_ide_pci/channel/v1"


static ata_for_controller_interface *sATA;
static ata_adapter_interface *sATAAdapter;
static device_manager_info *sDeviceManager;

struct HPT_controller_info *HPT_info;


// #pragma mark - helper functions


static void
set_channel(void *cookie, ata_channel channel)
{
	TRACE_EXT("set_channel()\n");

	sATAAdapter->set_channel((ata_adapter_channel_info *)cookie, channel);
}


static status_t
write_command_block_regs(void *channel_cookie, ata_task_file *tf,
	ata_reg_mask mask)
{
	TRACE_EXT("write_command_block_regs()\n");

	return sATAAdapter->write_command_block_regs(
		(ata_adapter_channel_info *)channel_cookie, tf, mask);
}


static status_t
read_command_block_regs(void *channel_cookie, ata_task_file *tf,
	ata_reg_mask mask)
{
	TRACE_EXT("read_command_block_regs()\n");

	return sATAAdapter->read_command_block_regs(
		(ata_adapter_channel_info *)channel_cookie, tf, mask);
}


static uint8
get_altstatus(void *channel_cookie)
{
	TRACE_EXT("get_altstatus()\n");

	return sATAAdapter->get_altstatus(
		(ata_adapter_channel_info *)channel_cookie);
}


static status_t
write_device_control(void *channel_cookie, uint8 val)
{
	TRACE_EXT("write_device_control()\n");

	return sATAAdapter->write_device_control(
		(ata_adapter_channel_info *)channel_cookie, val);
}


static status_t
write_pio(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	TRACE_EXT("write_pio()\n");

	return sATAAdapter->write_pio((ata_adapter_channel_info *)channel_cookie,
		data, count, force_16bit);
}


static status_t
read_pio(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	TRACE_EXT("read_pio()\n");

	return sATAAdapter->read_pio((ata_adapter_channel_info *)channel_cookie,
		data, count, force_16bit);
}


static status_t
prepare_dma(void *channel_cookie, const physical_entry *sg_list,
	size_t sg_list_count, bool to_device)
{
	TRACE_EXT("prepare_dma()\n");

	return sATAAdapter->prepare_dma((ata_adapter_channel_info *)channel_cookie,
		sg_list, sg_list_count, to_device);
}


static status_t
start_dma(void *channel_cookie)
{
	TRACE_EXT("start_dma()\n");

	return sATAAdapter->start_dma((ata_adapter_channel_info *)channel_cookie);
}


static status_t
finish_dma(void *channel_cookie)
{
	TRACE_EXT("finish_dma()\n");

	return sATAAdapter->finish_dma((ata_adapter_channel_info *)channel_cookie);
}


static status_t
init_channel(device_node *node, void **channel_cookie)
{
	status_t result;
	uint8 channel_index;

	TRACE("init_channel(): node: %p, cookie: %p\n", node, channel_cookie);

#if 0 /* debug */
	uint8 bus = 0, device = 0, function = 0;
	uint16 vendorID=0xffff, deviceID=0xffff;
	sDeviceManager->get_attr_uint16(node, B_DEVICE_VENDOR_ID, &vendorID, true);
	sDeviceManager->get_attr_uint16(node, B_DEVICE_ID, &deviceID, true);
	TRACE("init_channel():init_channel(): bus %3d, device %2d, function %2d: vendor %04x, device %04x\n",
		bus, device, function, vendorID, deviceID);
	if (vendorID != ATA_HIGHPOINT_ID) {
		TRACE ("unsupported! Vendor ID: %x\n",vendorID);
		return B_ERROR;
	}
#endif

	result = sATAAdapter->init_channel(node,
		(ata_adapter_channel_info **)channel_cookie,
		sizeof(ata_adapter_channel_info), sATAAdapter->inthand);
	// from here we have valid channel...
	ata_adapter_channel_info *channel=NULL;
	channel = (ata_adapter_channel_info *)*channel_cookie;

	if (sDeviceManager->get_attr_uint8(node, ATA_ADAPTER_CHANNEL_INDEX,
			&channel_index, false) != B_OK)  {
		TRACE("init_channel(): channel not set, strange!\n");
		return B_ERROR;
	}

	TRACE("init_channel(): channel command %x, control %x, result: %d \n", channel->command_block_base,channel->control_block_base, (int)result);
	TRACE("init_channel(): index #%d done. HPT_info deviceID: %04x, config option %x, revision %2d, function %2d \n", channel_index, HPT_info->deviceID, HPT_info->configOption, HPT_info->revisionID, HPT_info->function);
	return result;
}


static void
uninit_channel(void *channel_cookie)
{
	TRACE("uninit_channel()\n");

	sATAAdapter->uninit_channel((ata_adapter_channel_info *)channel_cookie);
}


static void
channel_removed(void *channel_cookie)
{
	TRACE("channel_removed()\n");

	sATAAdapter->channel_removed((ata_adapter_channel_info *)channel_cookie);
}


static status_t
init_controller(device_node *node, ata_adapter_controller_info **cookie)
{
	pci_device_module_info *pci;
	pci_device *pci_device;
	pci_info info;
	uint16 devID = 0xffff;
	uint8 revisionID = 0xff;
	uint8 function = 0xff;

	status_t result;

	// we need some our info structure here
	HPT_info = (struct HPT_controller_info *) malloc(sizeof(HPT_controller_info));
	if (HPT_info == NULL)
		return B_NO_MEMORY;

	result = sATAAdapter->init_controller(node, cookie,
		sizeof(ata_adapter_controller_info));

	if (result == B_OK) {
		// get device info
		device_node *parent = sDeviceManager->get_parent_node(node);
		sDeviceManager->get_driver(parent, (driver_module_info **)&pci, (void **)&pci_device);
		// read registers
		pci->get_pci_info(pci_device,&info);
		devID = info.device_id;
		revisionID = info.revision;
		function = info.function;

		HPT_info->deviceID = devID;
		HPT_info->revisionID = revisionID;
		HPT_info->function = function;

    	TRACE("init_controller(): found: device: %x, revision: %x, function: %x\n",devID,revisionID,function);

		// setting different config options
		if (devID == ATA_HPT366) {
			switch (revisionID) {
			case 0:
				HPT_info->configOption = CFG_HPT366_OLD;
				HPT_info->maxDMA = ATA_ULTRA_DMA4;
				break;
			case 1:
			case 2:
				HPT_info->configOption = CFG_HPT366;
				HPT_info->maxDMA = ATA_ULTRA_DMA4;
				break;
			case 3:
				HPT_info->configOption = CFG_HPT370;
				HPT_info->maxDMA = ATA_ULTRA_DMA5;
				break;
			case 5:
				HPT_info->configOption = CFG_HPT372;
				HPT_info->maxDMA = ATA_ULTRA_DMA6;
				break;
			default:
				HPT_info->configOption = CFG_HPTUnknown;
				HPT_info->maxDMA = ATA_ULTRA_DMA0;
			}
		} else {
			if (devID == ATA_HPT374) {
				HPT_info->configOption = CFG_HPT374;
				HPT_info->maxDMA = ATA_ULTRA_DMA6;
			} else {
				// all other versions use this config
				HPT_info->configOption = CFG_HPT372;
				HPT_info->maxDMA = ATA_ULTRA_DMA6;
			}
		}

		if (HPT_info->configOption == CFG_HPT366_OLD) {
			/* disable interrupt prediction */
			pci->write_pci_config(pci_device, 0x51, 1, (pci->read_pci_config(pci_device, 0x51, 1) & ~0x80));
			TRACE("Highpoint-ATA: old revision found.\n");
	    } else {
			/* disable interrupt prediction */
			pci->write_pci_config(pci_device, 0x51, 1, (pci->read_pci_config(pci_device, 0x51, 1) & ~0x03));
			pci->write_pci_config(pci_device, 0x55, 1, (pci->read_pci_config(pci_device, 0x55, 1) & ~0x03));

			/* enable interrupts */
			pci->write_pci_config(pci_device, 0x5a, 1, (pci->read_pci_config(pci_device, 0x5a, 1) & ~0x10));

			/* set clocks etc */
			if (HPT_info->configOption < CFG_HPT372) {
    			pci->write_pci_config(pci_device, 0x5b, 1, 0x22);
	    	} else {
    			pci->write_pci_config(pci_device, 0x5b, 1,
		     		(pci->read_pci_config(pci_device, 0x5b, 1) & 0x01) | 0x20);
	    	}
		}
	}
	return result;
}


static void
uninit_controller(ata_adapter_controller_info *controller)
{
	TRACE("uninit_controller()\n");
	free(HPT_info);
	sATAAdapter->uninit_controller(controller);
}


static void
controller_removed(ata_adapter_controller_info *controller)
{
	TRACE("controller_removed()\n");

	return sATAAdapter->controller_removed(controller);
}


static status_t
probe_controller(device_node *parent)
{
	status_t result;
	result = sATAAdapter->probe_controller(parent,
		HIGHPOINT_IDE_PCI_CONTROLLER_MODULE_NAME, "highpoint_ide_pci",
		"Highpoint IDE PCI Controller",
		HIGHPOINT_IDE_PCI_CHANNEL_MODULE_NAME,
		true,
		true,					// assume that command queuing works
		1,						// assume 16 bit alignment is enough
		0xffff,					// boundary is on 64k according to spec
		0x10000,				// up to 64k per S/G block according to spec
		false);					// by default, compatibility mode is used, not for HPT!

	TRACE("probe_controller(): probe result: %x\n",(int)result);
	return result;
}


static float
supports_device(device_node *parent)
{
	TRACE("supports_device()\n");

	const char *bus;
	uint16 baseClass, subClass;
	uint16 vendorID;
	uint16 deviceID;
	float result = -1.0f;

	// make sure parent is an PCI IDE mass storage host adapter device node
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_TYPE, &baseClass, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_SUB_TYPE, &subClass, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendorID, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID, false) != B_OK)
		return -1.0f;

	// No PCI bus OR no mass storage OR no Highpoint device = bail out
	if (vendorID != ATA_HIGHPOINT_ID) {
		TRACE("supports_device(): unsupported device: vendor ID: %x, deviceID: %x\n",vendorID, deviceID);
		return 0.0f;
	}

	// check if mass storage controller
	if ((subClass == PCI_ide) || (subClass == PCI_mass_storage_other)) {
		switch (deviceID) {
 		case ATA_HPT302:
 			// UDMA 6
 		case ATA_HPT366:
 			// UDMA 4, Rev 0 & 2
 			// UDMA 5, Rev 3
 		 	// UDMA 6, Rev 5
 		case ATA_HPT371:
 		case ATA_HPT372:
 		case ATA_HPT374:
 			// UDMA 6
			result = 1.0f;
			break;
		default:
			// device not supported, should be covered by generic ata driver
			result = 0.0f;
		}
	}
	TRACE("supports_device(): supporting device Vendor ID: %x, deviceID: %x, result %f\n", vendorID, deviceID, result);
	return result;
}

// #pragma mark - module dependencies

module_dependency module_dependencies[] = {
	{ ATA_FOR_CONTROLLER_MODULE_NAME, (module_info **)&sATA },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ ATA_ADAPTER_MODULE_NAME, (module_info **)&sATAAdapter },
	{}
};


static ata_controller_interface channel_interface = {
	{
		{
			HIGHPOINT_IDE_PCI_CHANNEL_MODULE_NAME,
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
		HIGHPOINT_IDE_PCI_CONTROLLER_MODULE_NAME,
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
