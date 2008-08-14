/*
 * Copyright 2007, Ithamar R. Adema. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>
#include <device_manager.h>
#include <bus/IDE.h>
#include <ide_adapter.h>


#define DRIVER_PRETTY_NAME	"Legacy SATA"
#define CONTROLLER_NAME		DRIVER_PRETTY_NAME
#define CONTROLLER_MODULE_NAME	"busses/ide/legacy_sata/driver_v1"
#define CHANNEL_MODULE_NAME	"busses/ide/legacy_sata/channel/v1"

#define TRACE(a...)		dprintf(DRIVER_PRETTY_NAME ": " a)
#define FLOW(a...)		dprintf(DRIVER_PRETTY_NAME ": " a)

#define PCI_vendor_VIA		0x1106
#define PCI_device_VIA6420	0x3149
#define PCI_device_VIA6421	0x3249
#define PCI_device_VIA8237A	0x0591

#define PCI_vendor_ALI		0x10b9
#define PCI_device_ALI5289	0x5289
#define PCI_device_ALI5287	0x5287
#define PCI_device_ALI5281	0x5281

#define PCI_vendor_NVIDIA	0x10de
#define PCI_device_NF2PROS1	0x008e
#define PCI_device_NF3PROS1	0x00e3
#define PCI_device_NF3PROS2	0x00ee
#define PCI_device_MCP4S1	0x0036
#define PCI_device_MCP4S2	0x003e
#define PCI_device_CK804S1	0x0054
#define PCI_device_CK804S2	0x0055
#define PCI_device_MCP51S1	0x0266
#define PCI_device_MCP51S2	0x0267
#define PCI_device_MCP55S1	0x037e
#define PCI_device_MCP55S2	0x037f
#define PCI_device_MCP61S1	0x03e7
#define PCI_device_MCP61S2	0x03f6
#define PCI_device_MCP61S3	0x03f7

#define ID(v,d) (((v)<< 16) | (d))

/* XXX: To be moved to PCI.h */
#define PCI_command_interrupt	0x400

static const char * const kChannelNames[] = {
	"Primary Channel", "Secondary Channel",
	"Tertiary Channel", "Quaternary Channel"
};

static ide_for_controller_interface* ide;
static ide_adapter_interface* ide_adapter;
static device_manager_info* dm;


static float
controller_supports(device_node *parent)
{
	uint16 vendor_id;
	uint16 device_id;
	status_t res;
	const char *bus = NULL;

	// get the bus (should be PCI)
	if (dm->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK)
		return B_ERROR;
	if (strcmp(bus, "pci") != 0) {
		return B_ERROR;
	}

	// get vendor and device ID
	if ((res=dm->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendor_id, false)) != B_OK
		|| (res=dm->get_attr_uint16(parent, B_DEVICE_ID, &device_id, false)) != B_OK) {
		return res;
	}

	switch (ID(vendor_id, device_id)) {
		/* VIA SATA chipsets */
		case ID(PCI_vendor_VIA, PCI_device_VIA6420):
		case ID(PCI_vendor_VIA, PCI_device_VIA6421):
		case ID(PCI_vendor_VIA, PCI_device_VIA8237A):
			break;

		/* ALI SATA chipsets */
		case ID(PCI_vendor_ALI, PCI_device_ALI5281):
		case ID(PCI_vendor_ALI, PCI_device_ALI5287):
		case ID(PCI_vendor_ALI, PCI_device_ALI5289):
			break;

		/* NVidia NForce chipsets */
		case ID(PCI_vendor_NVIDIA, PCI_device_NF2PROS1):
		case ID(PCI_vendor_NVIDIA, PCI_device_NF3PROS1):
		case ID(PCI_vendor_NVIDIA, PCI_device_NF3PROS2):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP4S1):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP4S2):
		case ID(PCI_vendor_NVIDIA, PCI_device_CK804S1):
		case ID(PCI_vendor_NVIDIA, PCI_device_CK804S2):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP51S1):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP51S2):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP55S1):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP55S2):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP61S1):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP61S2):
		case ID(PCI_vendor_NVIDIA, PCI_device_MCP61S3):
			break;

		default:
			return 0.0f;
	}

	TRACE("controller found! vendor 0x%04x, device 0x%04x\n", vendor_id, device_id);

	return 0.8f;
}


static status_t
controller_probe(device_node *parent)
{
	device_node *controller_node;
	device_node *channels[4];
	uint16 command_block_base[4];
	uint16 control_block_base[4];
	pci_device_module_info *pci;
	uint8 num_channels = 2;
	uint32 bus_master_base;
	pci_device *device = NULL;
	uint16 device_id;
	uint16 vendor_id;
	uint8 int_num;
	status_t res;
	uint8 index;

	TRACE("controller_probe\n");

	dm->get_driver(parent, (driver_module_info **)&pci, (void **)&device);

	device_id = pci->read_pci_config(device, PCI_device_id, 2);
	vendor_id = pci->read_pci_config(device, PCI_vendor_id, 2);
	int_num = pci->read_pci_config(device, PCI_interrupt_line, 1);
	bus_master_base = pci->read_pci_config(device, PCI_base_registers + 16, 4);

	/* Default PCI assigments */
	command_block_base[0] = pci->read_pci_config(device, PCI_base_registers + 0, 4);
	control_block_base[0] = pci->read_pci_config(device, PCI_base_registers + 4, 4);
	command_block_base[1] = pci->read_pci_config(device, PCI_base_registers + 8, 4);
	control_block_base[1] = pci->read_pci_config(device, PCI_base_registers + 12, 4);

	/* enable PCI interrupt */
	pci->write_pci_config(device, PCI_command, 2,
		pci->read_pci_config(device, PCI_command, 2) & ~PCI_command_interrupt);

	if (vendor_id == PCI_vendor_NVIDIA) {
		/* enable control access */
		pci->write_pci_config(device, 0x50, 1,
			pci->read_pci_config(device, 0x50, 1) | 0x04);
	}

	switch (ID(vendor_id, device_id)) {
		case ID(PCI_vendor_VIA,PCI_device_VIA6421):
			/* newer SATA chips has resources in one BAR for each channel */
			num_channels = 4;
			command_block_base[0] = pci->read_pci_config(device, PCI_base_registers + 0, 4);
			control_block_base[0] = command_block_base[0] + 8;
			command_block_base[1] = pci->read_pci_config(device, PCI_base_registers + 4, 4);
			control_block_base[1] = command_block_base[1] + 8;
			command_block_base[2] = pci->read_pci_config(device, PCI_base_registers + 8, 4);
			control_block_base[2] = command_block_base[2] + 8;
			command_block_base[3] = pci->read_pci_config(device, PCI_base_registers + 12, 4);
			control_block_base[3] = command_block_base[3] + 8;
			break;

		case ID(PCI_vendor_ALI, PCI_device_ALI5287):
			num_channels = 4;
			command_block_base[2] = pci->read_pci_config(device, PCI_base_registers + 0, 4) + 8;
			control_block_base[2] = pci->read_pci_config(device, PCI_base_registers + 4, 4) + 4;
			command_block_base[3] = pci->read_pci_config(device, PCI_base_registers + 8, 4) + 8;
			control_block_base[3] = pci->read_pci_config(device, PCI_base_registers + 12, 4) + 4;
			break;
	}

	bus_master_base &= PCI_address_io_mask;
	for (index = 0; index < num_channels; index++) {
		command_block_base[index] &= PCI_address_io_mask;
		control_block_base[index] &= PCI_address_io_mask;
	}

	res = ide_adapter->detect_controller(pci, device, parent, bus_master_base,
		CONTROLLER_MODULE_NAME, "" /* XXX: unused: controller_driver_type*/, CONTROLLER_NAME, true,
		true, 1, 0xffff, 0x10000, &controller_node);
	// don't register if controller is already registered!
	// (happens during rescan; registering new channels would kick out old channels)
	if (res != B_OK)
		goto err;
	if (controller_node == NULL) {
		res = B_IO_ERROR;
		goto err;
	}

	// ignore errors during registration of channels - could be a simple rescan collision

	for (index = 0; index < num_channels; index++) {
		res = ide_adapter->detect_channel(pci, device, controller_node, CHANNEL_MODULE_NAME,
			true, command_block_base[index], control_block_base[index], bus_master_base,
			int_num, index, kChannelNames[index], &channels[index], false);

		dprintf("%s: %s\n", kChannelNames[index], strerror(res));
	}


	TRACE("controller_probe success\n");
	return B_OK;

err:
	TRACE("controller_probe failed (%s)\n", strerror(res));
	return res;
}


static status_t
controller_init(device_node *node, void **controller_cookie)
{
	return ide_adapter->init_controller(
		node, (ide_adapter_controller_info**)controller_cookie,
		sizeof(ide_adapter_controller_info));
}


static void
controller_uninit(void *controller_cookie)
{
	ide_adapter->uninit_controller(controller_cookie);
}


static void
controller_removed(void *controller_cookie)
{
	ide_adapter->controller_removed(
		(ide_adapter_controller_info*)controller_cookie);
}


static status_t
channel_init(device_node *node, void **channel_cookie)
{
	return ide_adapter->init_channel(node,
		(ide_adapter_channel_info**)channel_cookie,
		sizeof(ide_adapter_channel_info),
		ide_adapter->inthand);
}


static void
channel_uninit(void *channel_cookie)
{
	ide_adapter->uninit_channel(channel_cookie);
}


static void
channel_removed(void *channel_cookie)
{
	ide_adapter->channel_removed(channel_cookie);
}


static void
channel_set(void *cookie, ide_channel channel)
{
	ide_adapter->set_channel((ide_adapter_channel_info *)cookie, channel);
}


static status_t
task_file_write(void *channel_cookie, ide_task_file *tf, ide_reg_mask mask)
{
	return ide_adapter->write_command_block_regs(channel_cookie,tf,mask);
}


static status_t
task_file_read(void *channel_cookie, ide_task_file *tf, ide_reg_mask mask)
{
	return ide_adapter->read_command_block_regs(channel_cookie,tf,mask);
}


static uint8
altstatus_read(void *channel_cookie)
{
	return ide_adapter->get_altstatus(channel_cookie);
}


static status_t
device_control_write(void *channel_cookie, uint8 val)
{
	return ide_adapter->write_device_control(channel_cookie,val);
}


static status_t
pio_write(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	return ide_adapter->write_pio(channel_cookie,data,count,force_16bit);
}


static status_t
pio_read(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	return ide_adapter->read_pio(channel_cookie,data,count,force_16bit);
}


static status_t
dma_prepare(void *channel_cookie, const physical_entry *sg_list, size_t sg_list_count, bool write)
{
	return ide_adapter->prepare_dma(channel_cookie,sg_list,sg_list_count,write);
}


static status_t
dma_start(void *channel_cookie)
{
	return ide_adapter->start_dma(channel_cookie);
}


static status_t
dma_finish(void *channel_cookie)
{
	return ide_adapter->finish_dma(channel_cookie);
}


module_dependency module_dependencies[] = {
	{ IDE_FOR_CONTROLLER_MODULE_NAME, (module_info **)&ide },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&dm },
	{ IDE_ADAPTER_MODULE_NAME, (module_info **)&ide_adapter },
	{}
};

static ide_controller_interface sChannelInterface = {
	{
		{
			CHANNEL_MODULE_NAME,
			0,
			NULL
		},

		NULL,
		NULL,
		channel_init,
		channel_uninit,
		NULL,
		NULL,
		channel_removed,
	},

	channel_set,

	task_file_write,
	task_file_read,

	altstatus_read,
	device_control_write,

	pio_write,
	pio_read,

	dma_prepare,
	dma_start,
	dma_finish,
};

static driver_module_info sControllerInterface = {
	{
		CONTROLLER_MODULE_NAME,
		0,
		NULL
	},

	.init_driver		= &controller_init,
	.uninit_driver		= &controller_uninit,
	.supports_device	= &controller_supports,
	.register_device	= &controller_probe,
	.device_removed		= &controller_removed,
};

module_info *modules[] = {
	(module_info *)&sControllerInterface,
	(module_info *)&sChannelInterface,
	NULL
};
