/*
 * Copyright 2002-04, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Promise TX2 series IDE controller driver
*/

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <bus/ide/ide_adapter.h>

#define debug_level_flow 0
#define debug_level_error 3
#define debug_level_info 3

#define DEBUG_MSG_PREFIX "PROMISE TX2 -- "

#include "wrapper.h"

#define PROMISE_TX2_CONTROLLER_MODULE_NAME "busses/ide/promise_tx2/device_v1"
#define PROMISE_TX2_CHANNEL_MODULE_NAME "busses/ide/promise_tx2/channel/v1"


static ide_for_controller_interface *ide;
static ide_adapter_interface *ide_adapter;
static device_manager_info *pnp;


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


static int32
inthand(void *arg)
{
	ide_adapter_channel_info *channel = (ide_adapter_channel_info *)arg;
	pci_device_module_info *pci = channel->pci;
	pci_device device = channel->device;
	ide_bm_status bm_status;
	uint8 status;

	SHOW_FLOW0( 3, "" );

	if (channel->lost)
		return B_UNHANDLED_INTERRUPT;

	// the controller always tells us whether it generated the IRQ, so ask it first
	pci->write_io_8(device, channel->bus_master_base + 1, 0x0b);
	if ((pci->read_io_8(device, channel->bus_master_base + 3) & 0x20) == 0)
		return B_UNHANDLED_INTERRUPT;

	if (channel->dmaing) {
		// in DMA mode, there is a safe test
		// in PIO mode, this doesn't work
		*(uint8 *)&bm_status = pci->read_io_8( device,
			channel->bus_master_base + ide_bm_status_reg );

		if (!bm_status.interrupt)
			return B_UNHANDLED_INTERRUPT;
	}

	// acknowledge IRQ
	status = pci->read_io_8(device, channel->command_block_base + 7);

	return ide->irq_handler(channel->ide_channel, status);
}


static status_t
prepare_dma(void *channel_cookie, const physical_entry *sg_list,
	size_t sg_list_count, bool to_device)
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
	return ide_adapter->init_channel(node, ide_channel, (ide_adapter_channel_info **)channel_cookie,
		sizeof(ide_adapter_channel_info), inthand);
}


static status_t
uninit_channel(void *channel_cookie)
{
	return ide_adapter->uninit_channel((ide_adapter_channel_info *)channel_cookie);
}


static void channel_removed(device_node_handle node, void *channel_cookie)
{
	return ide_adapter->channel_removed(node, (ide_adapter_channel_info *)channel_cookie);
}


static status_t
init_controller(device_node_handle node, void *user_cookie,
	ide_adapter_controller_info **cookie)
{
	return ide_adapter->init_controller(node, user_cookie, cookie,
		sizeof(ide_adapter_controller_info));
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


// publish node of ide controller

static status_t
publish_controller(device_node_handle parent, uint16 bus_master_base, uint8 intnum,
	io_resource_handle *resources, device_node_handle *node)
{
	device_attr attrs[] = {
		// info about ourself and our consumer
		{ B_DRIVER_MODULE, B_STRING_TYPE, { string: PROMISE_TX2_CONTROLLER_MODULE_NAME }},

		// properties of this controller for ide bus manager
		// there are always max. 2 devices
		{ IDE_CONTROLLER_MAX_DEVICES_ITEM, B_UINT8_TYPE, { ui8: 2 }},
		// of course we can DMA
		{ IDE_CONTROLLER_CAN_DMA_ITEM, B_UINT8_TYPE, { ui8: 1 }},
		// command queuing always works
		{ IDE_CONTROLLER_CAN_CQ_ITEM, B_UINT8_TYPE, { ui8: 1 }},
		// choose any name here
		{ IDE_CONTROLLER_CONTROLLER_NAME_ITEM, B_STRING_TYPE, { string: "Promise TX2" }},

		// DMA properties
		// some say it must be dword-aligned, others that it can be byte-aligned;
		// stay on the safe side
		{ B_DMA_ALIGNMENT, B_UINT32_TYPE, { ui32: 3 }},
		// one S/G block must not cross 64K boundary
		{ B_DMA_BOUNDARY, B_UINT32_TYPE, { ui32: 0xffff }},
		// size of S/G block is 16 bits with zero being 64K
		{ B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE, { ui32: 0x10000 }},
		{ B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE,
			{ ui32: IDE_ADAPTER_MAX_SG_COUNT }},
		{ B_DMA_HIGH_ADDRESS, B_UINT64_TYPE, { ui64: 0x100000000LL }},

		// private data to find controller
		{ IDE_ADAPTER_BUS_MASTER_BASE, B_UINT16_TYPE, { ui16: bus_master_base }},
		// store interrupt in controller node
		{ IDE_ADAPTER_INTNUM, B_UINT8_TYPE, { ui8: intnum }},
		{ NULL }
	};

	SHOW_FLOW0(2, "");

	return pnp->register_device(parent, attrs, resources, node);
}


// detect pure IDE controller, i.e. without channels

static status_t
detect_controller(pci_device_module_info *pci, pci_device pci_device,
	device_node_handle parent, uint16 bus_master_base, int8 intnum,
	device_node_handle *node)
{
	io_resource_handle resource_handles[2];

	SHOW_FLOW0(3, "");

	if ((bus_master_base & PCI_address_space) != 1)
		return B_OK;

	bus_master_base &= ~PCI_address_space;

	{
		io_resource resources[2] = {
			{ IO_PORT, bus_master_base, 16 },
			{}
		};

		if (pnp->acquire_io_resources(resources, resource_handles) != B_OK)
			return B_ERROR;
	}

	return publish_controller(parent, bus_master_base, intnum, resource_handles, node);
}


static status_t
probe_controller(device_node_handle parent)
{
	pci_device_module_info *pci;
	pci_device device;
	uint16 command_block_base[2];
	uint16 control_block_base[2];
	uint16 bus_master_base;
	device_node_handle controller_node, channels[2];
	uint8 intnum;
	status_t res;

	SHOW_FLOW0(3, "");

	if (pnp->init_driver(parent, NULL, (driver_module_info **)&pci, (void **)&device) != B_OK)
		return B_ERROR;

	command_block_base[0] = pci->read_pci_config(device, PCI_base_registers, 4);
	control_block_base[0] = pci->read_pci_config(device, PCI_base_registers + 4, 4);
	command_block_base[1] = pci->read_pci_config(device, PCI_base_registers + 8, 4);
	control_block_base[1] = pci->read_pci_config(device, PCI_base_registers + 12, 4);
	bus_master_base = pci->read_pci_config(device, PCI_base_registers + 16, 4);
	intnum = pci->read_pci_config(device, PCI_interrupt_line, 1);

	command_block_base[0] &= PCI_address_io_mask;
	control_block_base[0] &= PCI_address_io_mask;
	command_block_base[1] &= PCI_address_io_mask;
	control_block_base[1] &= PCI_address_io_mask;
	bus_master_base &= PCI_address_io_mask;

	res = detect_controller(pci, device, parent, bus_master_base, intnum, &controller_node);
	if (res != B_OK || controller_node == NULL)
		goto err;

	ide_adapter->detect_channel(pci, device, controller_node,
		PROMISE_TX2_CHANNEL_MODULE_NAME, true,
		command_block_base[0], control_block_base[0], bus_master_base, intnum,
		0, "Primary Channel", &channels[0], false);

	ide_adapter->detect_channel(pci, device, controller_node,
		PROMISE_TX2_CHANNEL_MODULE_NAME, true,
		command_block_base[1], control_block_base[1], bus_master_base, intnum,
		1, "Secondary Channel", &channels[1], false);

	pnp->uninit_driver(parent);

	return B_OK;

err:
	pnp->uninit_driver(parent);
	return res;
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
	{ IDE_FOR_CONTROLLER_MODULE_NAME, (module_info **)&ide },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{ IDE_ADAPTER_MODULE_NAME, (module_info **)&ide_adapter },
	{}
};


// exported interface
static ide_controller_interface channel_interface = {
	{
		{
			PROMISE_TX2_CHANNEL_MODULE_NAME,
			0,
			std_ops
		},

		NULL,	// supported devices
		NULL,
		(status_t (*)( device_node_handle , void *, void ** ))	init_channel,
		uninit_channel,
		channel_removed
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
		PROMISE_TX2_CONTROLLER_MODULE_NAME,
		0,
		std_ops
	},

	NULL,
	probe_controller,
	(status_t (*)(device_node_handle, void *, void **))	init_controller,
	(status_t (*)(void *))								uninit_controller,
	(void (*)(device_node_handle, void *))				controller_removed
};

module_info *modules[] = {
	(module_info *)&controller_interface,
	(module_info *)&channel_interface,
	NULL
};
