/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
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

#include <ata_adapter.h>
#include <bus/ISA.h>


//#define TRACE_IDE_ISA
#ifdef TRACE_IDE_ISA
#	define TRACE(x...) dprintf("ide_isa: " x)
#else
#	define TRACE(x...) ;
#endif


#define ATA_ISA_MODULE_NAME "busses/ata/ide_isa/driver_v1"

// private node item:
// io address of command block
#define ATA_ISA_COMMAND_BLOCK_BASE "ide_isa/command_block_base"
// io address of control block
#define ATA_ISA_CONTROL_BLOCK_BASE "ide_isa/control_block_base"
// interrupt number
#define ATA_ISA_INTNUM "ide_isa/irq"


ata_for_controller_interface *sATA;
device_manager_info *sDeviceManager;


// info about one channel
typedef struct channel_info {
	isa2_module_info	*isa;
	uint16	command_block_base;	// io address command block
	uint16	control_block_base; // io address control block
	int		intnum;			// interrupt number

	uint32	lost;			// != 0 if device got removed, i.e. if it must not
							// be accessed anymore

	ata_channel ataChannel;
	device_node *node;
} channel_info;


/*! publish node of an ata channel */
static status_t
publish_channel(device_node *parent, uint16 command_block_base,
	uint16 control_block_base, uint8 intnum, const char *name)
{
	device_attr attrs[] = {
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, { string: ATA_FOR_CONTROLLER_MODULE_NAME }},

		// properties of this controller for ata bus manager
		{ ATA_CONTROLLER_MAX_DEVICES_ITEM, B_UINT8_TYPE, { ui8: 2 }},
		{ ATA_CONTROLLER_CAN_DMA_ITEM, B_UINT8_TYPE, { ui8: 0 }},
		{ ATA_CONTROLLER_CONTROLLER_NAME_ITEM, B_STRING_TYPE, { string: name }},

		// DMA properties; the 16 bit alignment is not necessary as
		// the ata bus manager handles that very efficiently, but why
		// not use the block device manager for doing that?
		{ B_DMA_ALIGNMENT, B_UINT32_TYPE, { ui32: 1 }},
		{ B_DMA_HIGH_ADDRESS, B_UINT64_TYPE, { ui64: 0x100000000LL }},

		// private data to identify device
		{ ATA_ISA_COMMAND_BLOCK_BASE, B_UINT16_TYPE, { ui16: command_block_base }},
		{ ATA_ISA_CONTROL_BLOCK_BASE, B_UINT16_TYPE, { ui16: control_block_base }},
		{ ATA_ISA_INTNUM, B_UINT8_TYPE, { ui8: intnum }},
		{ NULL }
	};
	io_resource resources[3] = {
		{ B_IO_PORT, command_block_base, 8 },
		{ B_IO_PORT, control_block_base, 1 },
		{}
	};

	TRACE("publishing %s, resources %#x %#x %d\n",
		  name, command_block_base, control_block_base, intnum);

	return sDeviceManager->register_node(parent, ATA_ISA_MODULE_NAME, attrs, resources,
		NULL);
}


//	#pragma mark -


static void
set_channel(void *cookie, ata_channel ataChannel)
{
	channel_info *channel = cookie;
	channel->ataChannel = ataChannel;
}


static status_t
write_command_block_regs(void *channel_cookie, ata_task_file *tf,
	ata_reg_mask mask)
{
	channel_info *channel = channel_cookie;
	uint16 ioaddr = channel->command_block_base;
	int i;

	if (channel->lost)
		return B_ERROR;

	for (i = 0; i < 7; i++) {
		if (((1 << (i-7)) & mask) != 0) {
			TRACE("write_command_block_regs(): %x->HI(%x)\n",
				tf->raw.r[i + 7], i);
			channel->isa->write_io_8(ioaddr + 1 + i, tf->raw.r[i + 7]);
		}

		if (((1 << i) & mask) != 0 ) {
			TRACE("write_comamnd_block_regs(): %x->LO(%x)\n", tf->raw.r[i], i);
			channel->isa->write_io_8(ioaddr + 1 + i, tf->raw.r[i]);
		}
	}

	return B_OK;
}


static status_t
read_command_block_regs(void *channel_cookie, ata_task_file *tf,
	ata_reg_mask mask)
{
	channel_info *channel = channel_cookie;
	uint16 ioaddr = channel->command_block_base;
	int i;

	if (channel->lost)
		return B_ERROR;

	for (i = 0; i < 7; i++) {
		if (((1 << i) & mask) != 0) {
			tf->raw.r[i] = channel->isa->read_io_8(ioaddr + 1 + i);
			TRACE("read_command_block_regs(%x): %x\n", i, (int)tf->raw.r[i]);
		}
	}

	return B_OK;
}


static uint8
get_altstatus(void *channel_cookie)
{
	channel_info *channel = channel_cookie;
	uint16 altstatusaddr = channel->control_block_base;

	if (channel->lost)
		return B_ERROR;

	return channel->isa->read_io_8(altstatusaddr);
}


static status_t
write_device_control(void *channel_cookie, uint8 val)
{
	channel_info *channel = channel_cookie;
	uint16 device_control_addr = channel->control_block_base;

	TRACE("write_device_control(%x)\n", (int)val);

	if (channel->lost)
		return B_ERROR;

	channel->isa->write_io_8(device_control_addr, val);

	return B_OK;
}


static status_t
write_pio_16(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	channel_info *channel = channel_cookie;
	uint16 ioaddr = channel->command_block_base;

	if (channel->lost)
		return B_ERROR;

	// Bochs doesn't support 32 bit accesses;
	// no real performance impact as this driver is for Bochs only anyway
	force_16bit = true;

	if ((count & 1) != 0 || force_16bit) {
		for (; count > 0; --count)
			channel->isa->write_io_16(ioaddr, *(data++));
	} else {
		uint32 *cur_data = (uint32 *)data;

		for (; count > 0; count -= 2)
			channel->isa->write_io_32(ioaddr, *(cur_data++));
	}

	return B_OK;
}


static status_t
read_pio_16(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	channel_info *channel = channel_cookie;
	uint16 ioaddr = channel->command_block_base;

	if (channel->lost)
		return B_ERROR;

	force_16bit = true;

	if ((count & 1) != 0 || force_16bit) {
		for (; count > 0; --count)
			*(data++) = channel->isa->read_io_16(ioaddr);
	} else {
		uint32 *cur_data = (uint32 *)data;

		for (; count > 0; count -= 2)
			*(cur_data++) = channel->isa->read_io_32(ioaddr);
	}

	return B_OK;
}


static int32
inthand(void *arg)
{
	channel_info *channel = (channel_info *)arg;
	uint8 status;

	TRACE("interrupt handler()\n");

	if (channel->lost)
		return B_UNHANDLED_INTERRUPT;

	// acknowledge IRQ
	status = channel->isa->read_io_8(channel->command_block_base + 7);

	return sATA->interrupt_handler(channel->ataChannel, status);
}


static status_t
prepare_dma(void *channel_cookie, const physical_entry *sg_list,
	size_t sg_list_count, bool write)
{
	return B_NOT_ALLOWED;
}


static status_t
start_dma(void *channel_cookie)
{
	return B_NOT_ALLOWED;
}


static status_t
finish_dma(void *channel_cookie)
{
	return B_NOT_ALLOWED;
}


//	#pragma mark -


static float
supports_device(device_node *parent)
{
	const char *bus;

	// make sure parent is really the ISA bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return B_ERROR;

	if (strcmp(bus, "isa"))
		return 0.0;

	// we assume that every modern PC has an IDE controller, so no
	// further testing is done (well - I don't really know how to detect the
	// controller, but who cares ;)

	return 0.6;
}


static status_t
init_channel(device_node *node, void **_cookie)
{
	channel_info *channel;
	device_node *parent;
	isa2_module_info *isa;
	uint16 command_block_base, control_block_base;
	uint8 irq;
	status_t res;

	TRACE("ISA-IDE: channel init\n");

	// get device data
	if (sDeviceManager->get_attr_uint16(node, ATA_ISA_COMMAND_BLOCK_BASE, &command_block_base, false) != B_OK
		|| sDeviceManager->get_attr_uint16(node, ATA_ISA_CONTROL_BLOCK_BASE, &control_block_base, false) != B_OK
		|| sDeviceManager->get_attr_uint8(node, ATA_ISA_INTNUM, &irq, false) != B_OK)
		return B_ERROR;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&isa, NULL);
	sDeviceManager->put_node(parent);

	channel = (channel_info *)malloc(sizeof(channel_info));
	if (channel == NULL)
		return B_NO_MEMORY;

	TRACE("ISA-IDE: channel init, resources %#x %#x %d\n",
		  command_block_base, control_block_base, irq);

	channel->isa = isa;
	channel->node = node;
	channel->lost = false;
	channel->command_block_base = command_block_base;
	channel->control_block_base = control_block_base;
	channel->intnum = irq;
	channel->ataChannel = NULL;

	res = install_io_interrupt_handler(channel->intnum,
		inthand, channel, 0);

	if (res < 0) {
		TRACE("ISA-IDE: couldn't install irq handler for int %d\n", irq);
		goto err;
	}

	// enable interrupts so the channel is ready to run
	write_device_control(channel, ATA_DEVICE_CONTROL_BIT3);

	*_cookie = channel;
	return B_OK;

err:
	free(channel);
	return res;
}


static void
uninit_channel(void *channel_cookie)
{
	channel_info *channel = channel_cookie;

	TRACE("ISA-IDE: channel uninit\n");

	// disable IRQs
	write_device_control(channel, ATA_DEVICE_CONTROL_BIT3 | ATA_DEVICE_CONTROL_DISABLE_INTS);

	// catch spurious interrupt
	// (some controllers generate an IRQ when you _disable_ interrupts,
	//  they are delayed by less then 40 µs, so 1 ms is safe)
	snooze(1000);

	remove_io_interrupt_handler(channel->intnum, inthand, channel);

	free(channel);
}


static status_t
register_device(device_node *parent)
{
	status_t primaryStatus;
	status_t secondaryStatus;
	TRACE("register_device()\n");

	// our parent device is the isa bus and all device drivers are Universal,
	// so the pnp_manager tries each ISA driver in turn
	primaryStatus = publish_channel(parent, 0x1f0, 0x3f6, 14,
		"primary IDE channel");
	secondaryStatus = publish_channel(parent, 0x170, 0x376, 15,
		"secondary IDE channel");

	if (primaryStatus == B_OK || secondaryStatus == B_OK)
		return B_OK;

	return primaryStatus;
}


static void
channel_removed(void *cookie)
{
	channel_info *channel = cookie;
	TRACE("channel_removed()\n");

	// disable access instantly
	atomic_or(&channel->lost, 1);
}


module_dependency module_dependencies[] = {
	{ ATA_FOR_CONTROLLER_MODULE_NAME, (module_info **)&sATA },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};

// exported interface
static ata_controller_interface sISAControllerInterface = {
	{
		{
			ATA_ISA_MODULE_NAME,
			0,
			NULL
		},

		supports_device,
		register_device,
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

	&write_pio_16,
	&read_pio_16,

	&prepare_dma,
	&start_dma,
	&finish_dma,
};

module_info *modules[] = {
	(module_info *)&sISAControllerInterface,
	NULL
};
