/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>
#include <device_manager.h>
#include <bus/IDE.h>
#include <bus/ide/ide_adapter.h>
#include <block_io.h>

#define TRACE(a...) dprintf("si-3112 " a)
#define FLOW(a...)	dprintf("si-3112 " a)


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

struct channel_data;

typedef struct controller_data {
	pci_device_module_info *pci;
	pci_device device;

	device_node_handle node;
	
	struct channel_data *channel[4]; // XXX only for interrupt workaround
	int channel_count; // XXX only for interrupt workaround

	uint32 asic_index;
	uint32 int_num;

	area_id mmio_area;
	uint32 mmio_addr;
	
	uint32 lost;			// != 0 if device got removed, i.e. if it must not
							// be accessed anymore
} controller_data;


typedef struct channel_data {

	pci_device_module_info *pci;
	device_node_handle	node;
	pci_device 			device;
	ide_channel			ide_channel;

	volatile uint8 *	task_file;
	volatile uint8 *	control_block;
	volatile uint8 *	command_block;
	volatile uint8 *	dev_ctrl;
	volatile uint8 *	bm_status_reg;
	volatile uint8 *	bm_command_reg;
	volatile uint32 *	bm_prdt_address;

	area_id				prd_area;
	prd_entry *			prdt;
	void *				prdt_phys;
	uint32 				dma_active;
	uint32 				lost;
	
} channel_data;


static ide_for_controller_interface *	ide;
static ide_adapter_interface *			ide_adapter;
static device_manager_info *			dm;

static status_t device_control_write(void *channel_cookie, uint8 val);
static int32 handle_interrupt(void *arg);

static float
controller_supports(device_node_handle parent, bool *_noConnection)
{
	char *bus;
	uint16 vendorID;
	uint16 deviceID;
	
	FLOW("controller_supports\n");

	// get the bus (should be PCI)
	if (dm->get_attr_string(parent, B_DRIVER_BUS, &bus, false)
			!= B_OK) {
		return B_ERROR;
	}
	
	// get vendor and device ID
	if (dm->get_attr_uint16(parent, PCI_DEVICE_VENDOR_ID_ITEM,
			&vendorID, false) != B_OK
		|| dm->get_attr_uint16(parent, PCI_DEVICE_DEVICE_ID_ITEM,
			&deviceID, false) != B_OK) {
		free(bus);
		return B_ERROR;
	}
	
	FLOW("controller_supports: checking 0x%04x 0x%04x\n", vendorID, deviceID);

	// check, whether bus, vendor and device ID match
	if (strcmp(bus, "pci") != 0
		|| (vendorID != 0x1095)
		|| (deviceID != 0x3112 && deviceID != 3114)) {
		free(bus);
		return 0.0;
	}

	TRACE("controller_supports success\n");

	free(bus);
	return 0.6;
}


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
		{ "silicon_image_3112/mmio_base", B_UINT32_TYPE, { ui32: mmio_base }},
		{ "silicon_image_3112/int_num", B_UINT32_TYPE, { ui32: int_num }},
		{ NULL }
	};
	
	TRACE("publishing controller\n");

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
controller_init(device_node_handle node, void *user_cookie, void **controller_cookie)
{
	controller_data *controller;
	pci_device_module_info *pci;
	pci_device device;
	uint32 asic_index;
	uint32 mmio_base;
	uint32 mmio_addr;
	area_id mmio_area;
	uint32 int_num;
	status_t res;
	uint32 temp;
	int i;
	
	TRACE("controller_init\n");
	
	if (dm->get_attr_uint32(node, "silicon_image_3112/asic_index", &asic_index, false) != B_OK)
		return B_ERROR;
	if (dm->get_attr_uint32(node, "silicon_image_3112/mmio_base", &mmio_base, false) != B_OK)
		return B_ERROR;
	if (dm->get_attr_uint32(node, "silicon_image_3112/int_num", &int_num, false) != B_OK)
		return B_ERROR;

	controller = malloc(sizeof(controller_data));
	if (!controller)
		return B_NO_MEMORY;

	FLOW("controller %p\n", controller);	

	mmio_area = map_physical_memory("Silicon Image SATA regs", (void *)mmio_base, 
									asic_data[asic_index].mmio_bar_size, 
									B_ANY_KERNEL_ADDRESS, 0, (void **)&mmio_addr);
	if (mmio_area < B_OK) {
		TRACE("controller_init: mapping memory failed\n");
		free(controller);
		return B_ERROR;
	}


	if (dm->init_driver(dm->get_parent(node), NULL, (driver_module_info **)&pci, (void **)&device) < B_OK) {
		TRACE("controller_init: init parent failed\n");
		delete_area(mmio_area);
		free(controller);
		return B_ERROR;
	}
	
	TRACE("asic %ld\n", asic_index);
	TRACE("int_num %ld\n", int_num);
	TRACE("mmio_addr %p\n", (void *)mmio_addr);

	controller->pci        = pci;
	controller->device     = device;
	controller->node       = node;
	controller->asic_index = asic_index;
	controller->int_num    = int_num;
	controller->mmio_area  = mmio_area;
	controller->mmio_addr  = mmio_addr;
	controller->lost       = 0;

	controller->channel_count = asic_data[asic_index].channel_count;
	for (i = 0; i < asic_data[asic_index].channel_count; i++)
		controller->channel[i] = 0;

	if (asic_index == ASIC_SI3114) {
		// I put a magic spell on you
		temp = *(volatile uint32 *)mmio_addr + SI_BMDMA2;
		*(volatile uint32 *)(mmio_addr + SI_BMDMA2) = temp | SI_INT_STEERING;
		*(volatile uint32 *)(mmio_addr + SI_BMDMA2); // flush
	}

	temp = *(volatile uint32 *)mmio_addr + SI_SYSCFG;
	temp &= (asic_index == ASIC_SI3114) ? ~SI_MASK_4PORT : ~SI_MASK_2PORT;
	*(volatile uint32 *)(mmio_addr + SI_SYSCFG) = temp;
	*(volatile uint32 *)(mmio_addr + SI_SYSCFG); // flush
	
	// disable interrupts
	for (i = 0; i < asic_data[asic_index].channel_count; i++)
		*(volatile uint32 *)(mmio_addr + controller_channel_data[i].sien) = 0;
	*(volatile uint32 *)(mmio_addr + controller_channel_data[0].sien); // flush
	
	// install interrupt handler
	res = install_io_interrupt_handler(int_num, handle_interrupt, controller, 0);
	if (res <  B_OK) {
		TRACE("controller_init: installing interrupt handler failed\n");
		dm->uninit_driver(dm->get_parent(node));
		delete_area(mmio_area);
		free(controller);
	}
	
	*controller_cookie = controller;

	TRACE("controller_init success\n");
	return B_OK;
}


static status_t
controller_uninit(void *controller_cookie)
{
	controller_data *controller = controller_cookie;
	int i;

	TRACE("controller_uninit enter\n");

	// disable interrupts
	for (i = 0; i < asic_data[controller->asic_index].channel_count; i++)
		*(volatile uint32 *)(controller->mmio_addr + controller_channel_data[i].sien) = 0;
	*(volatile uint32 *)(controller->mmio_addr + controller_channel_data[0].sien); // flush

	remove_io_interrupt_handler(controller->int_num, handle_interrupt, controller);

	dm->uninit_driver(dm->get_parent(controller->node));	
	delete_area(controller->mmio_area);
	free(controller);

	TRACE("controller_uninit leave\n");
	return B_OK;
}


static void
controller_removed(device_node_handle node, void *controller_cookie)
{
	controller_data *controller = controller_cookie;
	controller->lost = 1;
}


static void
controller_get_paths(const char ***_bus, const char ***_device)
{
	static const char *kBus[] = { "pci", NULL };
	static const char *kDevice[] = { "drivers/dev/disk/sata", NULL };

	*_bus = kBus;
	*_device = kDevice;
}


static status_t
channel_init(device_node_handle node, void *user_cookie, void **channel_cookie)
{
	ide_channel ide_channel = user_cookie;
	controller_data *controller;
	channel_data *channel;
	physical_entry pe[1];
	size_t prdt_size;
	uint32 chan_index;
	
	TRACE("channel_init enter\n");
	
	channel = malloc(sizeof(channel_data));
	if (!channel)
		return B_NO_MEMORY;

	TRACE("channel %p\n", channel);	

	if (dm->get_attr_uint32(node, "silicon_image_3112/chan_index", &chan_index, false) != B_OK)
		return B_ERROR;

	TRACE("channel_index %ld\n", chan_index);
	TRACE("channel name: %s\n", controller_channel_data[chan_index].name);
		
	if (dm->init_driver(dm->get_parent(node), NULL, NULL, (void **)&controller) != B_OK)
		return B_ERROR;
		
	TRACE("controller %p\n", controller);	
	TRACE("mmio_addr %p\n", (void *)controller->mmio_addr);

	// PRDT must be contiguous, dword-aligned and must not cross 64K boundary 
	prdt_size = (IDE_ADAPTER_MAX_SG_COUNT * sizeof(prd_entry) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);
	channel->prd_area = create_area("prd", (void **)&channel->prdt, B_ANY_KERNEL_ADDRESS, prdt_size, B_FULL_LOCK | B_CONTIGUOUS, 0);
	if (channel->prd_area < B_OK) {
		TRACE("creating prd_area failed\n");
		goto err2;
	}

	get_memory_map(channel->prdt, prdt_size, pe, 1);
	channel->prdt_phys = (void *)(uint32)pe[0].address;

	channel->pci = controller->pci;
	channel->device = controller->device;
	channel->node = node;
	channel->ide_channel = ide_channel;

	channel->task_file = (volatile uint8 *)(controller->mmio_addr + controller_channel_data[chan_index].cmd + 1);
	channel->control_block = (volatile uint8 *)(controller->mmio_addr + controller_channel_data[chan_index].ctl);
	channel->command_block = (volatile uint8 *)(controller->mmio_addr + controller_channel_data[chan_index].cmd);
	channel->dev_ctrl = (volatile uint8 *)(controller->mmio_addr + controller_channel_data[chan_index].ctl);
	channel->bm_prdt_address = (volatile uint32 *)(controller->mmio_addr + controller_channel_data[chan_index].bmdma + ide_bm_prdt_address);
	channel->bm_status_reg = (volatile uint8 *)(controller->mmio_addr + controller_channel_data[chan_index].bmdma + ide_bm_status_reg);
	channel->bm_command_reg = (volatile uint8 *)(controller->mmio_addr + controller_channel_data[chan_index].bmdma + ide_bm_command_reg);

	channel->lost = 0;
	channel->dma_active = 0;
	
	controller->channel[chan_index] = channel;

	// enable interrupts so the channel is ready to run
	device_control_write(channel, ide_devctrl_bit3);

	*channel_cookie = channel;

	TRACE("channel_init leave\n");
	return B_OK;

err3:
	delete_area(channel->prd_area);
err2:
	dm->uninit_driver(dm->get_parent(node));
err:
	free(channel);
	return B_ERROR;
}


static status_t
channel_uninit(void *channel_cookie)
{
	channel_data *channel = channel_cookie;

	TRACE("channel_uninit enter\n");

	// disable IRQs
	device_control_write(channel, ide_devctrl_bit3 | ide_devctrl_nien);

	// catch spurious interrupt
	// (some controllers generate an IRQ when you _disable_ interrupts,
	//  they are delayed by less then 40 µs, so 1 ms is safe)
	snooze(1000);

	dm->uninit_driver(dm->get_parent(channel->node));

	delete_area(channel->prd_area);
	free(channel);

	TRACE("channel_uninit leave\n");
	return B_OK;
}


static void
channel_removed(device_node_handle node, void *channel_cookie)
{
	channel_data *channel = channel_cookie;
	channel->lost = 1;
}


static status_t
task_file_write(void *channel_cookie, ide_task_file *tf, ide_reg_mask mask)
{
	channel_data *channel = channel_cookie;
	int i;

	FLOW("task_file_write\n");
	
	if (channel->lost)
		return B_ERROR;

	for (i = 0; i < 7; i++) {
		if( ((1 << (i+7)) & mask) != 0 ) {
			FLOW("%x->HI(%x)\n", tf->raw.r[i + 7], i );
			channel->task_file[i] = tf->raw.r[i + 7];
		}
		
		if (((1 << i) & mask) != 0) {
			FLOW("%x->LO(%x)\n", tf->raw.r[i], i );
			channel->task_file[i] = tf->raw.r[i];
		}
	}
	*channel->dev_ctrl; // read altstatus to flush
	
	return B_OK;
}


static status_t
task_file_read(void *channel_cookie, ide_task_file *tf, ide_reg_mask mask)
{
	channel_data *channel = channel_cookie;
	int i;

	FLOW("task_file_read\n");

	if (channel->lost)
		return B_ERROR;

	for (i = 0; i < 7; i++) {
		if (((1 << i) & mask) != 0) {
			tf->raw.r[i] = channel->task_file[i];
			FLOW("%x: %x\n", i, (int)tf->raw.r[i] );
		}
	}
	
	return B_OK;
}


static uint8
altstatus_read(void *channel_cookie)
{
	channel_data *channel = channel_cookie;

	FLOW("altstatus_read\n");

	if (channel->lost)
		return 0x01; // Error bit

	return *channel->dev_ctrl;
}


static status_t 
device_control_write(void *channel_cookie, uint8 val)
{
	channel_data *channel = channel_cookie;

	FLOW("device_control_write %x\n", val);
	
	if (channel->lost)
		return B_ERROR;

	*channel->dev_ctrl = val;
	*channel->dev_ctrl; // read altstatus to flush
	
	return B_OK;
}


static status_t
pio_write(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	channel_data *channel = channel_cookie;
	if (channel->lost)
		return B_ERROR;

	FLOW("pio_write force_16bit = %d, (count & 1) = %d\n", force_16bit, (count & 1));

	// The data port is only 8 bit wide in the command register block.

	if ((count & 1) != 0 || force_16bit) {
		volatile uint16 * base = (volatile uint16 *)channel->command_block;
		for ( ; count > 0; --count)
			*base = *(data++);
	} else {
		volatile uint32 * base = (volatile uint32 *)channel->command_block;
		uint32 *cur_data = (uint32 *)data;
		
		for ( ; count > 0; count -= 2 )
			*base = *(cur_data++);
	}
	*channel->dev_ctrl; // read altstatus to flush
	
	return B_OK;
}


static status_t
pio_read(void *channel_cookie, uint16 *data, int count, bool force_16bit)
{
	channel_data *channel = channel_cookie;
	if (channel->lost)
		return B_ERROR;

	FLOW("pio_read force_16bit = %d, (count & 1) = %d\n", force_16bit, (count & 1));
	
	// The data port is only 8 bit wide in the command register block.
	// We are memory mapped and read using 16 or 32 bit access from this 8 bit location.

	if ((count & 1) != 0 || force_16bit) {
		volatile uint16 * base = (volatile uint16 *)channel->command_block;
		for ( ; count > 0; --count)
			*(data++) = *base;
	} else {
		volatile uint32 * base = (volatile uint32 *)channel->command_block;
		uint32 *cur_data = (uint32 *)data;
		
		for ( ; count > 0; count -= 2 )
			*(cur_data++) = *base;
	}
	
	return B_OK;
}


static status_t
dma_prepare(void *channel_cookie, const physical_entry *sg_list, size_t sg_list_count, bool write)
{
	channel_data *channel = channel_cookie;
	pci_device_module_info *pci = channel->pci;
	pci_device device = channel->device;
	prd_entry *prd = channel->prdt;
	ide_bm_command command;
	ide_bm_status status;
	uint32 temp;
	int i;
	
	FLOW("dma_prepare enter\n");
	
	for (i = sg_list_count - 1, prd = channel->prdt; i >= 0; --i, ++prd, ++sg_list ) {
		prd->address = B_HOST_TO_LENDIAN_INT32(pci->ram_address(device, sg_list->address));
		// 0 means 64K - this is done automatically be discarding upper 16 bits
		prd->count = B_HOST_TO_LENDIAN_INT16((uint16)sg_list->size);
		prd->EOT = i == 0;
		
		FLOW("%x, %x, %d\n", (int)prd->address, prd->count, prd->EOT );
	}

	// XXX move this to chan init?
	temp = (*channel->bm_prdt_address) & 3;
	temp |= B_HOST_TO_LENDIAN_INT32(pci->ram_address(device, (void *)channel->prdt_phys)) & ~3;
	*channel->bm_prdt_address = temp;

	*channel->dev_ctrl; // read altstatus to flush

	// reset interrupt and error signal
	*(uint8 *)&status = *channel->bm_status_reg;
	status.interrupt = 1;
	status.error = 1;
	*channel->bm_status_reg = *(uint8 *)&status;

	*channel->dev_ctrl; // read altstatus to flush
		
	// set data direction
	*(uint8 *)&command = *channel->bm_command_reg;
	command.from_device = !write;
	*channel->bm_command_reg = *(uint8 *)&command;

	*channel->dev_ctrl; // read altstatus to flush

	FLOW("dma_prepare leave\n");

	return B_OK;
}


static status_t
dma_start(void *channel_cookie)
{
	channel_data *channel = channel_cookie;
	ide_bm_command command;

	FLOW("dma_start enter\n");
	
	*(uint8 *)&command = *channel->bm_command_reg;

	command.start_stop = 1;
	channel->dma_active = true;

	*channel->bm_command_reg = *(uint8 *)&command;

	*channel->dev_ctrl; // read altstatus to flush
	
	FLOW("dma_start leave\n");
	
	return B_OK;
}


static status_t
dma_finish(void *channel_cookie)
{
	channel_data *channel = channel_cookie;
	ide_bm_command command;
	ide_bm_status status, new_status;

	FLOW("dma_finish enter\n");

	*(uint8 *)&command = *channel->bm_command_reg;

	command.start_stop = 0;
	channel->dma_active = false;
	
	*channel->bm_command_reg = *(uint8 *)&command;

	*(uint8 *)&status = *channel->bm_status_reg;

	new_status = status;	
	new_status.interrupt = 1;
	new_status.error = 1;

	*channel->bm_status_reg = *(uint8 *)&new_status;

	*channel->dev_ctrl; // read altstatus to flush
	
	if (status.error) {
		FLOW("dma_finish: failed\n");
		return B_ERROR;
	}

	if (!status.interrupt) {
		if (status.active) {
			FLOW("dma_finish: transfer aborted\n");
			return B_ERROR;
		} else {
			FLOW("dma_finish: buffer underrun\n");
			return B_DEV_DATA_UNDERRUN;
		}
	} else {
		if (status.active) {
			FLOW("dma_finish: buffer too large\n");
			return B_DEV_DATA_OVERRUN;
		} else {
			FLOW("dma_finish leave\n");
			return B_OK;
		}
	}
}


static int32
handle_interrupt(void *arg)
{
	controller_data *controller = arg;
	ide_bm_status bm_status;
	uint8 status;
	int i;

	FLOW("handle_interrupt\n");

	// XXX This is bogus
	for (i = 0; i <  controller->channel_count; i++) {
		channel_data *channel = controller->channel[i];
		if (!channel || channel->lost)
			continue;

		if (channel->dma_active) {
			*(uint8 *)&bm_status = *channel->bm_status_reg;
			if (!bm_status.interrupt)
				continue;
		}

		// acknowledge IRQ
		status = *(channel->command_block + 7);
		return ide->irq_handler(channel->ide_channel, status);
	}

	return B_UNHANDLED_INTERRUPT;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE("module init\n");
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE("module uninit\n");
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

	.supports_device							= &controller_supports,
	.register_device							= &controller_probe,
	.init_driver								= &controller_init,
	.uninit_driver								= &controller_uninit,
	.device_removed								= &controller_removed,
	.device_cleanup								= NULL,
	.get_supported_paths							= &controller_get_paths,
};

module_info *modules[] = {
	(module_info *)&controller_interface,
	(module_info *)&channel_interface,
	NULL
};
