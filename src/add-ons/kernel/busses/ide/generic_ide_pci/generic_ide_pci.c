/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
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

#define GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME "busses/ide/generic_ide_pci/" PCI_DEVICE_TYPE_NAME
#define GENERIC_IDE_PCI_CHANNEL_MODULE_NAME "busses/ide/generic_ide_pci/channel/v1"

#define IDE_PCI_CONTROLLER_TYPE_NAME "ide pci controller"

ide_for_controller_interface *ide;
static ide_adapter_interface *ide_adapter;
device_manager_info *pnp;


static int write_command_block_regs( ide_adapter_channel_info *channel, ide_task_file *tf, 
									 ide_reg_mask mask )
{
	return ide_adapter->write_command_block_regs( channel, tf, mask );
}

static status_t read_command_block_regs( ide_adapter_channel_info *channel, ide_task_file *tf, 
									ide_reg_mask mask )
{
	return ide_adapter->read_command_block_regs( channel, tf, mask );
}

static uint8 get_altstatus( ide_adapter_channel_info *channel)
{
	return ide_adapter->get_altstatus( channel );
}

static status_t write_device_control( ide_adapter_channel_info *channel, uint8 val )
{
	return ide_adapter->write_device_control( channel, val );
}

static status_t write_pio( ide_adapter_channel_info *channel, uint16 *data, int count, 
	bool force_16bit )
{
	return ide_adapter->write_pio( channel, data, count, force_16bit );
}

static status_t read_pio( ide_adapter_channel_info *channel, uint16 *data, int count, 
	bool force_16bit )
{
	return ide_adapter->read_pio( channel, data, count, force_16bit );
}

static status_t prepare_dma( ide_adapter_channel_info *channel, 
	const physical_entry *sg_list, size_t sg_list_count,
	bool to_device )
{
	return ide_adapter->prepare_dma( channel, sg_list, sg_list_count, to_device );
}

static status_t start_dma( ide_adapter_channel_info *channel )
{
	return ide_adapter->start_dma( channel );
}


static status_t finish_dma( ide_adapter_channel_info *channel )
{
	return ide_adapter->finish_dma( channel );
}


static status_t	init_channel( pnp_node_handle node, ide_channel ide_channel, 
	ide_adapter_channel_info **cookie )
{
	return ide_adapter->init_channel( node, ide_channel, cookie, 
		sizeof( ide_adapter_channel_info ), ide_adapter->inthand );
}


static status_t uninit_channel( ide_adapter_channel_info *channel )
{
	return ide_adapter->uninit_channel( channel );
}


static void channel_removed( pnp_node_handle node, ide_adapter_channel_info *channel )
{
	return ide_adapter->channel_removed( node, channel );
}


static status_t	init_controller( pnp_node_handle node, void *user_cookie, 
	ide_adapter_controller_info **cookie )
{
	return ide_adapter->init_controller( node, user_cookie, cookie, 
		sizeof( ide_adapter_controller_info ));
}


static status_t uninit_controller( ide_adapter_controller_info *controller )
{
	return ide_adapter->uninit_controller( controller );
}


static void controller_removed( 
	pnp_node_handle node, ide_adapter_controller_info *controller )
{
	return ide_adapter->controller_removed( node, controller );
}


static status_t probe_controller( pnp_node_handle parent )
{
	return ide_adapter->probe_controller( parent,
		GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME, "generic_ide_pci",
		"Generic IDE PCI Controller", 
		GENERIC_IDE_PCI_CHANNEL_MODULE_NAME,
		true,
		true,					// assume that command queuing works
		1,						// assume 16 bit alignment is enough
		0xffff,					// boundary is on 64k according to spec
		0x10000,				// up to 64k per S/G block according to spec
		true					// by default, compatibility mode is used
	);
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
	{ DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
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

		(status_t (*)( pnp_node_handle , void *, void ** ))	init_channel,
		(status_t (*)( void * ))						uninit_channel,
		NULL,
		(void (*)( pnp_node_handle , void * ))			channel_removed
	},

	(status_t (*)(ide_channel_cookie,
		ide_task_file*,ide_reg_mask))					&write_command_block_regs,
	(status_t (*)(ide_channel_cookie,
		ide_task_file*,ide_reg_mask))					&read_command_block_regs,

	(uint8 (*)(ide_channel_cookie))						&get_altstatus,
	(status_t (*)(ide_channel_cookie,uint8))			&write_device_control,

	(status_t (*)(ide_channel_cookie,uint16*,int,bool))	&write_pio,
	(status_t (*)(ide_channel_cookie,uint16*,int,bool))	&read_pio,

	(status_t (*)(ide_channel_cookie,
		const physical_entry *,size_t,bool))			&prepare_dma,
	(status_t (*)(ide_channel_cookie))					&start_dma,
	(status_t (*)(ide_channel_cookie))					&finish_dma,
};


static pnp_driver_info controller_interface = {
	{
		GENERIC_IDE_PCI_CONTROLLER_MODULE_NAME,
		0,
		module_std_ops
	},

	(status_t (*)( pnp_node_handle, void *, void ** ))	init_controller,
	(status_t (*)( void * ))						uninit_controller,
	probe_controller,
	(void (*)( pnp_node_handle, void * ))			controller_removed
};

#if !_BUILDING_kernel && !BOOT
_EXPORT 
module_info *modules[] = {
	(module_info *)&controller_interface,
	(module_info *)&channel_interface,
	NULL
};
#endif
