/*
 * Copyright 2006-2009, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>

#include <ata_adapter.h>
#include <tracing.h>

#define TRACE(x...)	do { dprintf("si-3112: " x); ktrace_printf("si-3112: " x); } while (0)
//#define FLOW(x...)	ktrace_printf("si-3112: " x)
#define FLOW(x...)


#define DRIVER_PRETTY_NAME		"Silicon Image SATA"
#define CONTROLLER_NAME			DRIVER_PRETTY_NAME
#define CONTROLLER_MODULE_NAME	"busses/ata/silicon_image_3112/driver_v1"
#define CHANNEL_MODULE_NAME		"busses/ata/silicon_image_3112/channel/v1"

enum asic_type {
	ASIC_SI3112 = 0,
	ASIC_SI3114 = 1,
};

static const struct {
	uint16 channel_count;
	uint32 mmio_bar_size;
	const char *asic_name;
} kASICData[2] = {
	{ 2, 0x200, "si3112" },
	{ 4, 0x400, "si3114" },
};

static const struct {
	uint16 cmd;
	uint16 ctl;
	uint16 bmdma;
	uint16 sien;
	uint16 stat;
	const char *name;
} kControllerChannelData[] = {
	{  0x80,  0x8A,  0x00, 0x148,  0xA0, "Primary Channel" },
	{  0xC0,  0xCA,  0x08, 0x1C8,  0xE0, "Secondary Channel" },
	{ 0x280, 0x28A, 0x200, 0x348, 0x2A0, "Tertiary Channel" },
	{ 0x2C0, 0x2CA, 0x208, 0x3C8, 0x2E0, "Quaternary Channel" },
};

#define SI_SYSCFG 			0x48
#define SI_BMDMA2			0x200
#define SI_INT_STEERING 	0x02
#define SI_MASK_2PORT		((1 << 22) | (1 << 23))
#define SI_MASK_4PORT		((1 << 22) | (1 << 23) | (1 << 24) | (1 << 25))

struct channel_data;

typedef struct controller_data {
	pci_device_module_info *pci;
	pci_device *device;

	device_node *node;

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
	device_node *		node;
	pci_device *		device;
	ata_channel			ataChannel;

	volatile uint8 *	task_file;
	volatile uint8 *	control_block;
	volatile uint8 *	command_block;
	volatile uint8 *	dev_ctrl;
	volatile uint8 *	bm_status_reg;
	volatile uint8 *	bm_command_reg;
	volatile uint32 *	bm_prdt_address;

	volatile uint32 *	stat;

	area_id				prd_area;
	prd_entry *			prdt;
	phys_addr_t			prdt_phys;
	uint32 				dma_active;
	uint32 				lost;
} channel_data;


static ata_for_controller_interface* sATA;
static ata_adapter_interface* sATAAdapter;
static device_manager_info* sDeviceManager;

static status_t device_control_write(void *channel_cookie, uint8 val);
static int32 handle_interrupt(void *arg);


static float
controller_supports(device_node *parent)
{
	const char *bus;
	uint16 vendorID;
	uint16 deviceID;

	// get the bus (should be PCI)
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| strcmp(bus, "pci") != 0)
		return B_ERROR;

	// get vendor and device ID
	if (sDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendorID, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID, false) != B_OK) {
		return B_ERROR;
	}

	#define ID(v, d) (((v) << 16) | (d))
	switch (ID(vendorID, deviceID)) {
		case ID(0x1095, 0x0240):
		case ID(0x1095, 0x3112):
		case ID(0x1095, 0x3114):
		case ID(0x1095, 0x3512):
		case ID(0x1002, 0x436e): // ATI
		case ID(0x1002, 0x4379): // ATI
		case ID(0x1002, 0x437a): // ATI
			break;
		default:
			return 0.0f;
	}

	TRACE("controller found! vendor 0x%04x, device 0x%04x\n", vendorID, deviceID);

	return 0.8f;
}


static status_t
controller_probe(device_node *parent)
{
	pci_device *device;
	pci_device_module_info *pci;
	uint32 mmioBase;
	uint16 deviceID;
	uint16 vendorID;
	uint8 interruptNumber;
	int asicIndex;

	TRACE("controller_probe\n");

	sDeviceManager->get_driver(parent, (driver_module_info **)&pci, (void **)&device);

	deviceID = pci->read_pci_config(device, PCI_device_id, 2);
	vendorID = pci->read_pci_config(device, PCI_vendor_id, 2);
	interruptNumber = pci->read_pci_config(device, PCI_interrupt_line, 1);
	mmioBase = pci->read_pci_config(device, PCI_base_registers + 20, 4)
		& PCI_address_io_mask;

	switch (ID(vendorID, deviceID)) {
		case ID(0x1095, 0x0240):
		case ID(0x1095, 0x3112):
		case ID(0x1095, 0x3512):
		case ID(0x1002, 0x436e): // ATI
		case ID(0x1002, 0x4379): // ATI
		case ID(0x1002, 0x437a): // ATI
			asicIndex = ASIC_SI3112;
			break;
		case ID(0x1095, 0x3114):
			asicIndex = ASIC_SI3114;
			break;
		default:
			TRACE("unsupported asic\n");
			return B_ERROR;
	}

	if (!mmioBase) {
		TRACE("mmio not configured\n");
		return B_ERROR;
	}

	if (interruptNumber == 0 || interruptNumber == 0xff) {
		TRACE("irq not configured\n");
		return B_ERROR;
	}

	{
		io_resource resources[2] = {
			{ B_IO_MEMORY, mmioBase, kASICData[asicIndex].mmio_bar_size },
			{}
		};
		device_attr attrs[] = {
			// properties of this controller for ATA bus manager
			// there are always max. 2 devices
			// (unless this is a Compact Flash Card with a built-in ATA
			// controller, which has exactly 1 device)
			{ ATA_CONTROLLER_MAX_DEVICES_ITEM, B_UINT8_TYPE,
				{ ui8: kASICData[asicIndex].channel_count }},
			// of course we can DMA
			{ ATA_CONTROLLER_CAN_DMA_ITEM, B_UINT8_TYPE, { ui8: true }},
			// choose any name here
			{ ATA_CONTROLLER_CONTROLLER_NAME_ITEM, B_STRING_TYPE,
				{ string: CONTROLLER_NAME }},

			// DMA properties
			// data must be word-aligned;
			// warning: some controllers are more picky!
			{ B_DMA_ALIGNMENT, B_UINT32_TYPE, { ui32: 1}},
			// one S/G block must not cross 64K boundary
			{ B_DMA_BOUNDARY, B_UINT32_TYPE, { ui32: 0xffff }},
			// max size of S/G block is 16 bits with zero being 64K
			{ B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE, { ui32: 0x10000 }},
			{ B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE,
				{ ui32: ATA_ADAPTER_MAX_SG_COUNT }},
			{ B_DMA_HIGH_ADDRESS, B_UINT64_TYPE, { ui64: 0x100000000LL }},

			// private data to find controller
			{ "silicon_image_3112/asic_index", B_UINT32_TYPE,
				{ ui32: asicIndex }},
			{ "silicon_image_3112/mmio_base", B_UINT32_TYPE,
				{ ui32: mmioBase }},
			{ "silicon_image_3112/int_num", B_UINT32_TYPE,
				{ ui32: interruptNumber }},
			{ NULL }
		};

		TRACE("publishing controller\n");

		return sDeviceManager->register_node(parent, CONTROLLER_MODULE_NAME, attrs,
			resources, NULL);
	}
}


static status_t
controller_init(device_node *node, void **_controllerCookie)
{
	controller_data *controller;
	device_node *parent;
	pci_device_module_info *pci;
	pci_device *device;
	uint32 asicIndex;
	uint32 mmioBase;
	uint32 mmioAddr;
	area_id mmioArea;
	uint32 interruptNumber;
	status_t res;
	uint32 temp;
	int i;

	TRACE("controller_init\n");

	if (sDeviceManager->get_attr_uint32(node, "silicon_image_3112/asic_index", &asicIndex, false) != B_OK)
		return B_ERROR;
	if (sDeviceManager->get_attr_uint32(node, "silicon_image_3112/mmio_base", &mmioBase, false) != B_OK)
		return B_ERROR;
	if (sDeviceManager->get_attr_uint32(node, "silicon_image_3112/int_num", &interruptNumber, false) != B_OK)
		return B_ERROR;

	controller = malloc(sizeof(controller_data));
	if (!controller)
		return B_NO_MEMORY;

	FLOW("controller %p\n", controller);

	mmioArea = map_physical_memory("Silicon Image SATA regs", mmioBase,
		kASICData[asicIndex].mmio_bar_size, B_ANY_KERNEL_ADDRESS, 0,
		(void **)&mmioAddr);
	if (mmioArea < B_OK) {
		TRACE("controller_init: mapping memory failed\n");
		free(controller);
		return B_ERROR;
	}

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&pci, (void **)&device);
	sDeviceManager->put_node(parent);

	TRACE("asic index %ld\n", asicIndex);
	TRACE("asic name %s\n", kASICData[asicIndex].asic_name);
	TRACE("int num %ld\n", interruptNumber);
	TRACE("mmio addr %p\n", (void *)mmioAddr);

	controller->pci = pci;
	controller->device = device;
	controller->node = node;
	controller->asic_index = asicIndex;
	controller->int_num = interruptNumber;
	controller->mmio_area = mmioArea;
	controller->mmio_addr = mmioAddr;
	controller->lost = 0;

	controller->channel_count = kASICData[asicIndex].channel_count;
	for (i = 0; i < kASICData[asicIndex].channel_count; i++)
		controller->channel[i] = 0;

	if (asicIndex == ASIC_SI3114) {
		// I put a magic spell on you
		temp = *(volatile uint32 *)(mmioAddr + SI_BMDMA2);
		*(volatile uint32 *)(mmioAddr + SI_BMDMA2) = temp | SI_INT_STEERING;
		*(volatile uint32 *)(mmioAddr + SI_BMDMA2); // flush
	}

	// disable all sata phy interrupts
	for (i = 0; i < kASICData[asicIndex].channel_count; i++)
		*(volatile uint32 *)(mmioAddr + kControllerChannelData[i].sien) = 0;
	*(volatile uint32 *)(mmioAddr + kControllerChannelData[0].sien); // flush

	// install interrupt handler
	res = install_io_interrupt_handler(interruptNumber, handle_interrupt,
		controller, 0);
	if (res < B_OK) {
		TRACE("controller_init: installing interrupt handler failed\n");
		delete_area(mmioArea);
		free(controller);
		return res;
	}

	// unmask interrupts
	temp = *(volatile uint32 *)(mmioAddr + SI_SYSCFG);
	temp &= (asicIndex == ASIC_SI3114) ? (~SI_MASK_4PORT) : (~SI_MASK_2PORT);
	*(volatile uint32 *)(mmioAddr + SI_SYSCFG) = temp;
	*(volatile uint32 *)(mmioAddr + SI_SYSCFG); // flush

	*_controllerCookie = controller;

	TRACE("controller_init success\n");
	return B_OK;
}


static void
controller_uninit(void *controllerCookie)
{
	controller_data *controller = controllerCookie;
	uint32 temp;

	TRACE("controller_uninit enter\n");

	// disable interrupts
	temp = *(volatile uint32 *)(controller->mmio_addr + SI_SYSCFG);
	temp |= (controller->asic_index == ASIC_SI3114) ? SI_MASK_4PORT : SI_MASK_2PORT;
	*(volatile uint32 *)(controller->mmio_addr + SI_SYSCFG) = temp;
	*(volatile uint32 *)(controller->mmio_addr + SI_SYSCFG); // flush

	remove_io_interrupt_handler(controller->int_num, handle_interrupt,
		controller);

	delete_area(controller->mmio_area);
	free(controller);

	TRACE("controller_uninit leave\n");
}


static status_t
controller_register_channels(void *cookie)
{
	controller_data *controller = cookie;
	int index;

	// publish channel nodes
	for (index = 0; index < kASICData[controller->asic_index].channel_count;
			index++) {
		device_attr attrs[] = {
			{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: DRIVER_PRETTY_NAME }},
//			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: kControllerChannelData[channelIndex].name }},
			{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, { string: ATA_FOR_CONTROLLER_MODULE_NAME }},

			// private data to identify channel
			{ ATA_CONTROLLER_CAN_DMA_ITEM, B_UINT8_TYPE, { ui8: true }},
			{ "silicon_image_3112/chan_index", B_UINT32_TYPE, { ui32: index }},
			{ NULL }
		};

		TRACE("publishing %s\n", kControllerChannelData[index].name);

		sDeviceManager->register_node(controller->node, CHANNEL_MODULE_NAME, attrs,
			NULL, NULL);
	}

	return B_OK;
}


static void
controller_removed(void *controllerCookie)
{
	controller_data *controller = controllerCookie;
	controller->lost = 1;
}


//	#pragma mark -


static status_t
channel_init(device_node *node, void **_channelCookie)
{
	controller_data *controller;
	device_node *parent;
	channel_data *channel;
	physical_entry entry;
	size_t prdtSize;
	uint32 channelIndex;

	TRACE("channel_init enter\n");

	channel = malloc(sizeof(channel_data));
	if (!channel)
		return B_NO_MEMORY;

	if (sDeviceManager->get_attr_uint32(node, "silicon_image_3112/chan_index", &channelIndex, false) != B_OK)
		goto err;

#if 0
	if (1 /* debug */){
		uint8 bus, device, function;
		uint16 vendorID, deviceID;
		sDeviceManager->get_attr_uint8(node, PCI_DEVICE_BUS_ITEM, &bus, true);
		sDeviceManager->get_attr_uint8(node, PCI_DEVICE_DEVICE_ITEM, &device, true);
		sDeviceManager->get_attr_uint8(node, PCI_DEVICE_FUNCTION_ITEM, &function, true);
		sDeviceManager->get_attr_uint16(node, PCI_DEVICE_VENDOR_ID_ITEM, &vendorID, true);
		sDeviceManager->get_attr_uint16(node, PCI_DEVICE_DEVICE_ID_ITEM, &deviceID, true);
		TRACE("bus %3d, device %2d, function %2d: vendor %04x, device %04x\n",
			bus, device, function, vendorID, deviceID);
	}
#endif

	TRACE("channel_index %ld\n", channelIndex);
	TRACE("channel name: %s\n", kControllerChannelData[channelIndex].name);

	TRACE("channel %p\n", channel);

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, NULL, (void **)&controller);
	sDeviceManager->put_node(parent);

	TRACE("controller %p\n", controller);
	TRACE("mmio_addr %p\n", (void *)controller->mmio_addr);

	// PRDT must be contiguous, dword-aligned and must not cross 64K boundary
// TODO: Where's the handling for the 64 K boundary? create_area_etc() can be
// used.
	prdtSize = (ATA_ADAPTER_MAX_SG_COUNT * sizeof(prd_entry) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);
	channel->prd_area = create_area("prd", (void **)&channel->prdt,
		B_ANY_KERNEL_ADDRESS, prdtSize, B_32_BIT_CONTIGUOUS, 0);
	if (channel->prd_area < B_OK) {
		TRACE("creating prd_area failed\n");
		goto err;
	}

	get_memory_map(channel->prdt, prdtSize, &entry, 1);
	channel->prdt_phys = entry.address;

	channel->pci = controller->pci;
	channel->device = controller->device;
	channel->node = node;
	channel->ataChannel = NULL;

	channel->task_file = (volatile uint8 *)(controller->mmio_addr + kControllerChannelData[channelIndex].cmd + 1);
	channel->control_block = (volatile uint8 *)(controller->mmio_addr + kControllerChannelData[channelIndex].ctl);
	channel->command_block = (volatile uint8 *)(controller->mmio_addr + kControllerChannelData[channelIndex].cmd);
	channel->dev_ctrl = (volatile uint8 *)(controller->mmio_addr + kControllerChannelData[channelIndex].ctl);
	channel->bm_prdt_address = (volatile uint32 *)(controller->mmio_addr + kControllerChannelData[channelIndex].bmdma + ATA_BM_PRDT_ADDRESS);
	channel->bm_status_reg = (volatile uint8 *)(controller->mmio_addr + kControllerChannelData[channelIndex].bmdma + ATA_BM_STATUS_REG);
	channel->bm_command_reg = (volatile uint8 *)(controller->mmio_addr + kControllerChannelData[channelIndex].bmdma + ATA_BM_COMMAND_REG);
	channel->stat = (volatile uint32 *)(controller->mmio_addr + kControllerChannelData[channelIndex].stat);

	channel->lost = 0;
	channel->dma_active = 0;

	controller->channel[channelIndex] = channel;

	// disable interrupts
	device_control_write(channel, ATA_DEVICE_CONTROL_BIT3
		| ATA_DEVICE_CONTROL_DISABLE_INTS);

	*_channelCookie = channel;

	TRACE("channel_init leave\n");
	return B_OK;

err:
	free(channel);
	return B_ERROR;
}


static void
channel_uninit(void *channelCookie)
{
	channel_data *channel = channelCookie;

	TRACE("channel_uninit enter\n");

	// disable interrupts
	device_control_write(channel, ATA_DEVICE_CONTROL_BIT3
		| ATA_DEVICE_CONTROL_DISABLE_INTS);

	delete_area(channel->prd_area);
	free(channel);

	TRACE("channel_uninit leave\n");
}


static void
channel_removed(void *channelCookie)
{
	channel_data *channel = channelCookie;
	channel->lost = 1;
}


static void
set_channel(void *channelCookie, ata_channel ataChannel)
{
	channel_data *channel = channelCookie;
	channel->ataChannel = ataChannel;
}


static status_t
task_file_write(void *channelCookie, ata_task_file *tf, ata_reg_mask mask)
{
	channel_data *channel = channelCookie;
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
task_file_read(void *channelCookie, ata_task_file *tf, ata_reg_mask mask)
{
	channel_data *channel = channelCookie;
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
altstatus_read(void *channelCookie)
{
	channel_data *channel = channelCookie;

	FLOW("altstatus_read\n");

	if (channel->lost)
		return 0x01; // Error bit

	return *channel->dev_ctrl;
}


static status_t
device_control_write(void *channelCookie, uint8 val)
{
	channel_data *channel = channelCookie;

	FLOW("device_control_write 0x%x\n", val);

	if (channel->lost)
		return B_ERROR;

	*channel->dev_ctrl = val;
	*channel->dev_ctrl; // read altstatus to flush

	return B_OK;
}


static status_t
pio_write(void *channelCookie, uint16 *data, int count, bool force_16bit)
{
	channel_data *channel = channelCookie;
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
pio_read(void *channelCookie, uint16 *data, int count, bool force_16bit)
{
	channel_data *channel = channelCookie;
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
dma_prepare(void *channelCookie, const physical_entry *sg_list,
	size_t sg_list_count, bool write)
{
	channel_data *channel = channelCookie;
	pci_device_module_info *pci = channel->pci;
	pci_device *device = channel->device;
	prd_entry *prd = channel->prdt;
	uint8 command;
	uint8 status;
	uint32 temp;
	int i;

	FLOW("dma_prepare enter\n");

	for (i = sg_list_count - 1, prd = channel->prdt; i >= 0;
			--i, ++prd, ++sg_list ) {
		prd->address = B_HOST_TO_LENDIAN_INT32((uint32)pci->ram_address(device,
			(void*)(addr_t)sg_list->address));

		// 0 means 64K - this is done automatically by discarding upper 16 bits
		prd->count = B_HOST_TO_LENDIAN_INT16((uint16)sg_list->size);
		prd->EOT = i == 0;

		FLOW("%x, %x, %d\n", (int)prd->address, prd->count, prd->EOT);
	}

	// XXX move this to chan init?
	temp = (*channel->bm_prdt_address) & 3;
	temp |= B_HOST_TO_LENDIAN_INT32((uint32)pci->ram_address(device,
		(void *)(addr_t)channel->prdt_phys)) & ~3;
	*channel->bm_prdt_address = temp;

	*channel->dev_ctrl; // read altstatus to flush

	// reset interrupt and error signal
	status = *channel->bm_status_reg | ATA_BM_STATUS_INTERRUPT
		| ATA_BM_STATUS_ERROR;
	*channel->bm_status_reg = status;

	*channel->dev_ctrl; // read altstatus to flush

	// set data direction
	command = *channel->bm_command_reg;
	if (write)
		command &= ~ATA_BM_COMMAND_READ_FROM_DEVICE;
	else
		command |= ATA_BM_COMMAND_READ_FROM_DEVICE;

	*channel->bm_command_reg = command;

	*channel->dev_ctrl; // read altstatus to flush

	FLOW("dma_prepare leave\n");

	return B_OK;
}


static status_t
dma_start(void *channelCookie)
{
	channel_data *channel = channelCookie;
	uint8 command;

	FLOW("dma_start enter\n");

	command = *channel->bm_command_reg | ATA_BM_COMMAND_START_STOP;
	channel->dma_active = true;
	*channel->bm_command_reg = command;

	*channel->dev_ctrl; // read altstatus to flush

	FLOW("dma_start leave\n");

	return B_OK;
}


static status_t
dma_finish(void *channelCookie)
{
	channel_data *channel = channelCookie;
	uint8 command;
	uint8 status;

	FLOW("dma_finish enter\n");

	status = *channel->bm_status_reg;

	command = *channel->bm_command_reg;
	*channel->bm_command_reg = command & ~ATA_BM_COMMAND_START_STOP;

	channel->dma_active = false;

	*channel->bm_status_reg = status | ATA_BM_STATUS_ERROR;
	*channel->dev_ctrl; // read altstatus to flush

	if ((status & ATA_BM_STATUS_ACTIVE) != 0) {
		TRACE("dma_finish: buffer too large\n");
		return B_DEV_DATA_OVERRUN;
	}
	if ((status & ATA_BM_STATUS_ERROR) != 0) {
		FLOW("dma_finish: failed\n");
		return B_ERROR;
	}

	FLOW("dma_finish leave\n");
	return B_OK;
}


static int32
handle_interrupt(void *arg)
{
	controller_data *controller = arg;
	uint8 statusATA, statusBM;
	int32 result;
	int i;

	FLOW("handle_interrupt\n");

	result = B_UNHANDLED_INTERRUPT;

	for (i = 0; i < controller->channel_count; i++) {
		channel_data *channel = controller->channel[i];
		if (!channel || channel->lost)
			continue;

		if (!channel->dma_active) {
			// this could be a spurious interrupt, so read
			// ata status register to acknowledge
			*(channel->command_block + 7);
			continue;
		}

		statusBM = *channel->bm_status_reg;
		if (statusBM & ATA_BM_STATUS_INTERRUPT) {
			statusATA = *(channel->command_block + 7);
			*channel->bm_status_reg
				= (statusBM & 0xf8) | ATA_BM_STATUS_INTERRUPT;
			sATA->interrupt_handler(channel->ataChannel, statusATA);
			result = B_INVOKE_SCHEDULER;
		}
	}

	return result;
}


module_dependency module_dependencies[] = {
	{ ATA_FOR_CONTROLLER_MODULE_NAME,	(module_info **)&sATA },
	{ B_DEVICE_MANAGER_MODULE_NAME,		(module_info **)&sDeviceManager },
	{ ATA_ADAPTER_MODULE_NAME,			(module_info **)&sATAAdapter },
	{}
};


static ata_controller_interface sChannelInterface = {
	{
		{
			CHANNEL_MODULE_NAME,
			0,
			NULL
		},

		.supports_device			= NULL,
		.register_device			= NULL,
		.init_driver				= &channel_init,
		.uninit_driver				= &channel_uninit,
		.register_child_devices		= NULL,
		.device_removed				= &channel_removed,
	},

	.set_channel					= &set_channel,
	.write_command_block_regs		= &task_file_write,
	.read_command_block_regs		= &task_file_read,
	.get_altstatus					= &altstatus_read,
	.write_device_control			= &device_control_write,
	.write_pio						= &pio_write,
	.read_pio						= &pio_read,
	.prepare_dma					= &dma_prepare,
	.start_dma						= &dma_start,
	.finish_dma						= &dma_finish,
};


static driver_module_info sControllerInterface = {
	{
		CONTROLLER_MODULE_NAME,
		0,
		NULL
	},

	.supports_device				= &controller_supports,
	.register_device				= &controller_probe,
	.init_driver					= &controller_init,
	.uninit_driver					= &controller_uninit,
	.register_child_devices			= &controller_register_channels,
	.device_removed					= &controller_removed,
};

module_info *modules[] = {
	(module_info *)&sControllerInterface,
	(module_info *)&sChannelInterface,
	NULL
};
