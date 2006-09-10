/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>
#include <device_manager.h>
#include <bus/ide/ide_adapter.h>
#include <block_io.h>

#define TRACE(a...) dprintf("si-3112 " a)


#define DRIVER_PRETTY_NAME		"Silicon Image SATA"
#define CONTROLLER_DRIVER		"silicon_image_3112"
#define CONTROLLER_NAME			DRIVER_PRETTY_NAME
#define CONTROLLER_MODULE_NAME	"busses/ide/silicon_image_3112/device_v1"
#define CHANNEL_MODULE_NAME		"busses/ide/silicon_image_3112/channel/v1"

enum asic_type {
	ASIC_SI3112 = 0,
	ASIC_SI3114 = 1,
};

static const char *adapter_asic_names[] = {
	"si3112",
	"si3114",
	NULL
};

static const struct {
	uint16 channel_count;
	uint32 mmio_bar_size;
} asic_data[2] = {
	{ 2, 0x200 },
	{ 4, 0x400 },
};

static const struct {
	uint16 cmd;
	uint16 ctl;
	uint16 bmdma;
	uint16 sien;
	const char *name;
} controller_channel_data[] = {
	{  0x80,  0x8A,  0x00, 0x148, "Primary Channel" },
	{  0xC0,  0xCA,  0x08, 0x1C8, "Secondary Channel" },
	{ 0x280, 0x28A, 0x200, 0x348, "Tertiary Channel" },
	{ 0x2C0, 0x2CA, 0x208, 0x3C8, "Quaternary Channel" },
};

#define SI_SYSCFG 			0x48
#define SI_BMDMA2			0x200
#define SI_INT_STEERING 	0x02
#define SI_MASK_2PORT		((1 << 22) | (1 << 23))
#define SI_MASK_4PORT		((1 << 22) | (1 << 23) | (1 << 24) | (1 << 25))


static ide_for_controller_interface *	ide;
static ide_adapter_interface *			ide_adapter;
static device_manager_info *			dm;


static status_t
controller_probe(device_node_handle parent)
{
	pci_device device;
	pci_device_module_info *pci;
	device_node_handle controller_node;
	uint32 mmio_base;
	uint16 device_id;
	uint8 int_num;
	int asic_index;
	int chan_index;
	status_t res;
	
	TRACE("controller_probe\n");

	if (dm->init_driver(parent, NULL, (driver_module_info **)&pci, (void **)&device) != B_OK)
		return B_ERROR;
		
	device_id = pci->read_pci_config(device, PCI_device_id, 2);
	int_num = pci->read_pci_config(device, PCI_interrupt_line, 1);
	mmio_base = pci->read_pci_config(device, PCI_base_registers + 20, 4);
	mmio_base &= PCI_address_io_mask;
	
	switch (device_id) {
		case 0x3112:
			asic_index = ASIC_SI3112;
			break;
		case 0x3114: 
			asic_index = ASIC_SI3114;
			break;
		default:
			TRACE("unsupported asic\n");
			goto err;
	}

	if (!mmio_base) {
		TRACE("mmio not configured\n");
		goto err;
	}

	if (int_num == 0 || int_num == 0xff) {
		TRACE("irq not configured\n");
		goto err;
	}

	{
	io_resource_handle resource_handles[2];
	io_resource resources[2] = {
		{ IO_MEM, mmio_base, asic_data[asic_index].mmio_bar_size },
		{}
	};

	if (dm->acquire_io_resources(resources, resource_handles) != B_OK) {
		TRACE("acquire_io_resources failed\n");
		goto err;
	}
	
	{

	device_attr controller_attrs[] = {
		// info about ourself and our consumer
		{ B_DRIVER_MODULE, B_STRING_TYPE, { string: CONTROLLER_DRIVER }},

		// properties of this controller for ide bus manager
		// there are always max. 2 devices 
		// (unless this is a Compact Flash Card with a built-in IDE controller,
		//  which has exactly 1 device)
		{ IDE_CONTROLLER_MAX_DEVICES_ITEM, B_UINT8_TYPE, { ui8: asic_data[asic_index].channel_count }},
		// of course we can DMA
		{ IDE_CONTROLLER_CAN_DMA_ITEM, B_UINT8_TYPE, { ui8: true }},
		// command queuing always works
		{ IDE_CONTROLLER_CAN_CQ_ITEM, B_UINT8_TYPE, { ui8: true }},
		// choose any name here
		{ IDE_CONTROLLER_CONTROLLER_NAME_ITEM, B_STRING_TYPE, { string: CONTROLLER_NAME }},

		// DMA properties
		// data must be word-aligned; 
		// warning: some controllers are more picky!
		{ B_BLOCK_DEVICE_DMA_ALIGNMENT, B_UINT32_TYPE, { ui32: 1}},
		// one S/G block must not cross 64K boundary
		{ B_BLOCK_DEVICE_DMA_BOUNDARY, B_UINT32_TYPE, { ui32: 0xffff }},
		// max size of S/G block is 16 bits with zero being 64K
		{ B_BLOCK_DEVICE_MAX_SG_BLOCK_SIZE, B_UINT32_TYPE, { ui32: 0x10000 }},
		// see definition of MAX_SG_COUNT
		{ B_BLOCK_DEVICE_MAX_SG_BLOCKS, B_UINT32_TYPE, { ui32: IDE_ADAPTER_MAX_SG_COUNT }},

		// private data to find controller
		{ "silicon_image_3112/asic_index", B_UINT32_TYPE, { ui32: asic_index }},
		{ "silicon_image_3112/int_num", B_UINT32_TYPE, { ui32: int_num }},
		{ NULL }
	};
	
	TRACE("publishing controller");

	res = dm->register_device(parent, controller_attrs, resource_handles, &controller_node);
	if (res != B_OK || controller_node == NULL) {
		TRACE("register controller failed\n");
		goto err;
	}

	}

	// publish channel nodes
	for (chan_index = 0; chan_index < asic_data[asic_index].channel_count; chan_index++) {
		device_attr channel_attrs[] = {
			// info about ourself and our consumer
			{ B_DRIVER_MODULE, B_STRING_TYPE, { string: CHANNEL_MODULE_NAME }},
			{ B_DRIVER_PRETTY_NAME, B_STRING_TYPE, { string: DRIVER_PRETTY_NAME }},
			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: controller_channel_data[chan_index].name }},
			{ B_DRIVER_FIXED_CHILD, B_STRING_TYPE, { string: IDE_FOR_CONTROLLER_MODULE_NAME }},

			// private data to identify channel
			{ IDE_CONTROLLER_CAN_DMA_ITEM, B_UINT8_TYPE, { ui8: true }},
			{ "silicon_image_3112/chan_index", B_UINT32_TYPE, { ui32: chan_index }},
			{ NULL }
		};

		device_node_handle channel_node;

		TRACE("publishing %s\n", controller_channel_data[chan_index].name );

		dm->register_device(controller_node, channel_attrs, resource_handles, &channel_node);
	}

	}

	dm->uninit_driver(parent);

	TRACE("controller_probe success\n");
	return B_OK;

err:
	dm->uninit_driver(parent);	
	TRACE("controller_probe failed\n");
	return B_ERROR;
}



static status_t 
controller_init(device_node_handle node, void *user_cookie, void **_cookie)
{
	return B_ERROR;
}


static status_t
controller_uninit(void *cookie)
{
	return B_ERROR;
}


static void
controller_removed(device_node_handle node, void *cookie)
{
}


static status_t
channel_init(device_node_handle node, void *user_cookie, void **_cookie)
{
	return B_ERROR;
}


static status_t
channel_uninit(void *cookie)
{
	return B_ERROR;
}


static void
channel_removed(device_node_handle node, void *cookie)
{
}


static status_t
task_file_write(ide_channel_cookie channel, ide_task_file *tf, ide_reg_mask mask)
{
	return B_ERROR;
}


static status_t
task_file_read(ide_channel_cookie channel, ide_task_file *tf, ide_reg_mask mask)
{
	return B_ERROR;
}


static uint8
altstatus_read(ide_channel_cookie channel)
{
	return 0xff;
}


static status_t 
device_control_write(ide_channel_cookie channel, uint8 val)
{
	return B_ERROR;
}


static status_t
pio_write(ide_channel_cookie channel, uint16 *data, int count, bool force_16bit)
{
	return B_ERROR;
}

static status_t
pio_read(ide_channel_cookie channel, uint16 *data, int count, bool force_16bit)
{
	return B_ERROR;
}


static status_t
dma_prepare(ide_channel_cookie channel, const physical_entry *sg_list, size_t sg_list_count, bool write)
{
	return B_ERROR;
}


static status_t
dma_start(ide_channel_cookie channel)
{
	return B_ERROR;
}


static status_t
dma_finish(ide_channel_cookie channel)
{
	return B_ERROR;
}


static int32
handle_interrupt(void *arg)
{
	ide_adapter_channel_info *channel = (ide_adapter_channel_info *)arg;
	pci_device_module_info *pci = channel->pci;
	pci_device device = channel->device;

	return B_UNHANDLED_INTERRUPT;
}


static status_t
std_ops(int32 op, ...)
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
	{ IDE_FOR_CONTROLLER_MODULE_NAME,	(module_info **)&ide },
	{ B_DEVICE_MANAGER_MODULE_NAME,		(module_info **)&dm },
	{ IDE_ADAPTER_MODULE_NAME,			(module_info **)&ide_adapter },
	{}
};


static ide_controller_interface channel_interface = {
	{
		{
			CHANNEL_MODULE_NAME,
			0,
			std_ops
		},
		
		.supports_device						= NULL,
		.register_device						= NULL,
		.init_driver							= &channel_init,
		.uninit_driver							= &channel_uninit,
		.device_removed							= &channel_removed,
		.device_cleanup							= NULL,
		.get_supported_paths					= NULL,
	},

	.write_command_block_regs					= &task_file_write,
	.read_command_block_regs					= &task_file_read,
	.get_altstatus								= &altstatus_read,
	.write_device_control						= &device_control_write,
	.write_pio									= &pio_write,
	.read_pio									= &pio_read,
	.prepare_dma								= &dma_prepare,
	.start_dma									= &dma_start,
	.finish_dma									= &dma_finish,
};


static driver_module_info controller_interface = {
	{
		CONTROLLER_MODULE_NAME,
		0,
		std_ops
	},

	.supports_device							= NULL,
	.register_device							= &controller_probe,
	.init_driver								= &controller_init,
	.uninit_driver								= &controller_uninit,
	.device_removed								= &controller_removed,
	.device_cleanup								= NULL,
	.get_supported_paths						= NULL,
};

module_info *modules[] = {
	(module_info *)&controller_interface,
	(module_info *)&channel_interface,
	NULL
};
