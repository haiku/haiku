/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	IDE ISA controller driver

	This is a testing-only driver. In reality, you want to use
	the IDE PCI controller driver, but at least under Bochs, there's not
	much choice as PCI support is very limited there.
*/

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <bus/IDE.h>
#include <bus/ISA.h>
#include <device_manager.h>
#include <blkman.h>

#define debug_level_flow 4
#define debug_level_error 4
#define debug_level_info 4

#define DEBUG_MSG_PREFIX "IDE ISA -- "

#include "wrapper.h"

#define IDE_ISA_MODULE_NAME "busses/ide/ide_isa/"ISA_DEVICE_TYPE_NAME

// private node item:
// io address of command block
#define IDE_ISA_COMMAND_BLOCK_BASE "ide_isa/command_block_base"
// io address of control block
#define IDE_ISA_CONTROL_BLOCK_BASE "ide_isa/control_block_base"
// interrupt number
#define IDE_ISA_INTNUM "ide_isa/irq"

//isa2_module_info *isa;
ide_for_controller_interface *ide;
device_manager_info *pnp;


// info about one channel
typedef struct channel_info {
	isa2_module_info	*isa;
	uint16	command_block_base;	// io address command block
	uint16	control_block_base; // io address control block
	int		intnum;			// interrupt number	
	
	uint32	lost;			// != 0 if device got removed, i.e. if it must not
							// be accessed anymore
	
	ide_channel ide_channel;
	pnp_node_handle	node;
} channel_info;


static int
write_command_block_regs(channel_info *channel, ide_task_file *tf, ide_reg_mask mask)
{
	int i;
	uint16 ioaddr = channel->command_block_base;
	
	if( channel->lost )
		return B_ERROR;

	for( i = 0; i < 7; i++ ) {
		if( ((1 << (i-7)) & mask) != 0 ) {
			SHOW_FLOW( 3, "%x->HI(%x)", tf->raw.r[i + 7], i );
			channel->isa->write_io_8( ioaddr + 1 + i, tf->raw.r[i + 7] );
		}
		
		if( ((1 << i) & mask) != 0 ) {
			SHOW_FLOW( 3, "%x->LO(%x)", tf->raw.r[i], i );
			channel->isa->write_io_8( ioaddr + 1 + i, tf->raw.r[i] );
		}
	}
	
	return B_OK;
}


static status_t
read_command_block_regs(channel_info *channel, ide_task_file *tf, ide_reg_mask mask)
{
	int i;
	uint16 ioaddr = channel->command_block_base;

	if( channel->lost )
		return B_ERROR;

	for( i = 0; i < 7; i++ ) {
		if( ((1 << i) & mask) != 0 ) {
			tf->raw.r[i] = channel->isa->read_io_8( ioaddr + 1 + i );
			SHOW_FLOW( 3, "%x: %x", i, (int)tf->raw.r[i] );
		}
	}
	
	return B_OK;
}


static uint8
get_altstatus(channel_info *channel)
{
	uint16 altstatusaddr = channel->control_block_base;

	if (channel->lost)
		return B_ERROR;

	return channel->isa->read_io_8(altstatusaddr);
}


static status_t
write_device_control(channel_info *channel, uint8 val)
{
	uint16 device_control_addr = channel->control_block_base;

	SHOW_FLOW( 3, "%x", (int)val );

	if (channel->lost)
		return B_ERROR;

	channel->isa->write_io_8(device_control_addr, val);

	return B_OK;
}


static status_t
write_pio_16(channel_info *channel, uint16 *data, int count, bool force_16bit)
{
	uint16 ioaddr = channel->command_block_base;
	
	if( channel->lost )
		return B_ERROR;

	// Bochs doesn't support 32 bit accesses; 
	// no real performance impact as this driver is for Bochs only anyway
	force_16bit = true;

	if( (count & 1) != 0 || force_16bit ) {
		for( ; count > 0; --count )
			channel->isa->write_io_16( ioaddr, *(data++) );
			
	} else {
		uint32 *cur_data = (uint32 *)data;
		
		for( ; count > 0; count -= 2 )
			channel->isa->write_io_32( ioaddr, *(cur_data++) );
	}
	
	return B_OK;
}


static status_t
read_pio_16(channel_info *channel, uint16 *data, int count, bool force_16bit)
{
	uint16 ioaddr = channel->command_block_base;
	
	if( channel->lost )
		return B_ERROR;

	force_16bit = true;

	if( (count & 1) != 0 || force_16bit ) {
		for( ; count > 0; --count )
			*(data++) = channel->isa->read_io_16( ioaddr  );
			
	} else {
		uint32 *cur_data = (uint32 *)data;
		
		for( ; count > 0; count -= 2 )
			*(cur_data++) = channel->isa->read_io_32( ioaddr );
	}
	
	return B_OK;
}


static int32
inthand(void *arg)
{
	channel_info *channel = (channel_info *)arg;
	uint8 status;

	SHOW_FLOW0( 3, "" );

	if (channel->lost)
		return B_UNHANDLED_INTERRUPT;

	// acknowledge IRQ
	status = channel->isa->read_io_8(channel->command_block_base + 7);

	return ide->irq_handler(channel->ide_channel, status);
}


static status_t
prepare_dma(channel_info *channel, const physical_entry *sg_list, size_t sg_list_count,
	uint32 startbyte, uint32 blocksize, size_t *numBytes, bool to_device)
{
	return B_NOT_ALLOWED;
}


static status_t
start_dma(void *channel)
{
	return B_NOT_ALLOWED;
}


static status_t
finish_dma(void *channel)
{
	return B_NOT_ALLOWED;
}


static status_t
init_channel(pnp_node_handle node, ide_channel ide_channel, channel_info **cookie)
{
	channel_info *channel;
	isa2_module_info *isa;
	void *dummy;
	uint16 command_block_base, control_block_base;
	uint8 irq;
	status_t res;

	// get device data	
	if (pnp->get_attr_uint16(node, IDE_ISA_COMMAND_BLOCK_BASE, &command_block_base, false) != B_OK
		|| pnp->get_attr_uint16(node, IDE_ISA_CONTROL_BLOCK_BASE, &control_block_base, false) != B_OK
		|| pnp->get_attr_uint8(node, IDE_ISA_INTNUM, &irq, false) != B_OK)
		return B_ERROR;

	if (pnp->load_driver(pnp->get_parent(node), NULL, (pnp_driver_info **)&isa, &dummy) != B_OK)
		return B_ERROR;

	channel = (channel_info *)malloc(sizeof(channel_info));
	if (channel == NULL) {
		res = B_NO_MEMORY;
		goto err0;
	}

	channel->isa = isa;
	channel->node = node;
	channel->lost = false;
	channel->command_block_base = command_block_base;
	channel->control_block_base = control_block_base;
	channel->intnum = irq;
	channel->ide_channel = ide_channel;

	res = install_io_interrupt_handler(channel->intnum, 
		inthand, channel, 0);

	if (res < 0) {
		SHOW_ERROR( 0, "couldn't install irq handler @%d", irq);
		goto err;
	}

	// enable interrupts so the channel is ready to run
	write_device_control(channel, ide_devctrl_bit3);

	*cookie = channel;

	return B_OK;

err:
	free(channel);
	
err0:
	pnp->unload_driver(pnp->get_parent(node));
	return res;
}


static status_t
uninit_channel(channel_info *channel)
{
	// disable IRQs
	write_device_control(channel, ide_devctrl_bit3 | ide_devctrl_nien);

	// catch spurious interrupt
	// (some controllers generate an IRQ when you _disable_ interrupts,
	//  they are delayed by less then 40 µs, so 1 ms is safe)
	snooze(1000);
	
	remove_io_interrupt_handler(channel->intnum, inthand, channel);

	pnp->unload_driver(pnp->get_parent(channel->node));

	free(channel);

	return B_OK;
}


// publish node of ide channel
static status_t
publish_channel(pnp_node_handle parent, io_resource_handle *resources, uint16 command_block_base,
	uint16 control_block_base, uint8 intnum, const char *name)
{
	pnp_node_attr attrs[] = {
		// info about ourself and our consumer
		{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: IDE_ISA_MODULE_NAME }},
		{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: IDE_BUS_TYPE_NAME }},
		{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: IDE_FOR_CONTROLLER_MODULE_NAME }},
		{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: name }},
		{ PNP_DRIVER_DEVICE_IDENTIFIER, B_STRING_TYPE, { string: "ide_isa" }},
		// don't scan if loaded as we own I/O resources
		{ PNP_DRIVER_NO_LIVE_RESCAN, B_UINT8_TYPE, { ui8: 1 }},

		// properties of this controller for ide bus manager
		{ IDE_CONTROLLER_MAX_DEVICES_ITEM, B_UINT8_TYPE, { ui8: 2 }},
		{ IDE_CONTROLLER_CAN_DMA_ITEM, B_UINT8_TYPE, { ui8: 0 }},
		{ IDE_CONTROLLER_CAN_CQ_ITEM, B_UINT8_TYPE, { ui8: 1 }},
		{ IDE_CONTROLLER_CONTROLLER_NAME_ITEM, B_STRING_TYPE, { string: name }},

		// DMA properties; the 16 bit alignment is not necessary as 
		// the ide bus manager handles that very efficiently, but why 
		// not use the block device manager for doing that?
		{ BLKDEV_DMA_ALIGNMENT, B_UINT32_TYPE, { ui32: 1 }},

		// private data to identify device
		{ IDE_ISA_COMMAND_BLOCK_BASE, B_UINT16_TYPE, { ui16: command_block_base }},
		{ IDE_ISA_CONTROL_BLOCK_BASE, B_UINT16_TYPE, { ui16: control_block_base }},
		{ IDE_ISA_INTNUM, B_UINT8_TYPE, { ui8: intnum }},
		{ NULL }
	};
	pnp_node_handle node;

	SHOW_FLOW0( 2, "" );

	return pnp->register_device(parent, attrs, resources, &node);
}


// detect IDE channel
static status_t
probe_channel(pnp_node_handle parent,
	uint16 command_block_base, uint16 control_block_base, 
	int intnum, const char *name)
{
	io_resource resources[3] = {
		{ IO_PORT, command_block_base, 8 },
		{ IO_PORT, control_block_base, 1 },
		{}
	};
	io_resource_handle resource_handles[3];

	SHOW_FLOW(3, "name = \"%s\"", name);

	// we aren't upset if io-ports are in use already - only 
	// the PCI IDE driver can own them, and if it does, we exit silently
	if (pnp->acquire_io_resources(resources, resource_handles) != B_OK)
		return B_OK;

	// we assume that every modern PC has an IDE controller, so no
	// further testing is done (well - I don't really know how to detect the
	// controll, but who cares ;)	
	return publish_channel(parent, resource_handles, command_block_base,
		control_block_base, intnum, name);
}


static status_t
scan_parent(pnp_node_handle node)
{
	SHOW_FLOW0( 3, "" );

	// our parent device is the isa bus and all device drivers are Universal,
	// so the pnp_manager tries each ISA driver in turn
	probe_channel(node, 0x1f0, 0x3f6, 14, "primary IDE channel");
	probe_channel(node, 0x170, 0x376, 15, "secondary IDE channel");

	return B_OK;
}


static void
channel_removed(pnp_node_handle node, channel_info *channel)
{
	SHOW_FLOW0( 3, "" );

	if (channel != NULL)
		// disable access instantly
		atomic_or(&channel->lost, 1);

	return;
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
	{ DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};

// exported interface
ide_controller_interface isa_controller_interface = {
	{
		{
			IDE_ISA_MODULE_NAME,
			0,
			std_ops
		},

		(status_t (*)( pnp_node_handle, void *, void ** ))	init_channel,
		(status_t (*)( void * ))						uninit_channel,
		scan_parent,
		(void (*)( pnp_node_handle, void * ))			channel_removed
	},

	(status_t (*)(ide_channel_cookie,
		ide_task_file*,ide_reg_mask))					&write_command_block_regs,
	(status_t (*)(ide_channel_cookie,
		ide_task_file*,ide_reg_mask))					&read_command_block_regs,
	
	(uint8 (*)(ide_channel_cookie))						&get_altstatus,
	(status_t (*)(ide_channel_cookie,uint8))			&write_device_control,

	(status_t (*)(ide_channel_cookie,uint16*,int,bool))	&write_pio_16,
	(status_t (*)(ide_channel_cookie,uint16*,int,bool))	&read_pio_16,

	(status_t (*)(ide_channel_cookie,
		const physical_entry *,size_t,bool))			&prepare_dma,
	(status_t (*)(ide_channel_cookie))					&start_dma,
	(status_t (*)(ide_channel_cookie))					&finish_dma,
};

#if !_BUILDING_kernel && !BOOT
_EXPORT 
module_info *modules[] = {
	(module_info *)&isa_controller_interface,
	NULL
};
#endif
