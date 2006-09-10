/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>
#include <bus/ide/ide_adapter.h>
#include <block_io.h>


#define CONTROLLER_MODULE_NAME	"busses/ide/silicon_image_3112/device_v1"
#define CHANNEL_MODULE_NAME		"busses/ide/silicon_image_3112/channel/v1"


static ide_for_controller_interface *	ide;
static ide_adapter_interface *			ide_adapter;
static device_manager_info *			dm;


static status_t
controller_probe(device_node_handle parent)
{
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
