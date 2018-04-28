/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#include <new>
#include <stdio.h>
#include <string.h>

#include <bus/PCI.h>
#include <PCI_x86.h>

#include <KernelExport.h>

#include "sdhci_pci.h"


#define TRACE_SDHCI
#ifdef TRACE_SDHCI
#	define TRACE(x...) dprintf("\33[33msdhci_pci:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33msdhci_pci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33msdhci_pci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define SDHCI_PCI_DEVICE_MODULE_NAME "busses/mmc/sdhci_pci/driver_v1"
#define SDHCI_PCI_MMC_BUS_MODULE_NAME "busses/mmc/sdhci_pci/device/v1"

#define SDHCI_PCI_CONTROLLER_TYPE_NAME "sdhci pci controller"

#define SLOTS_COUNT				"device/slots_count"
#define SLOT_NUMBER				"device/slot"
#define BAR_INDEX				"device/bar"

typedef struct {
	pci_device_module_info* pci;
	pci_device* device;
	addr_t base_addr;
	uint8 irq;
	sdhci_mmc_bus mmc_bus;
	area_id regs_area;
	device_node* node;
	pci_info info;
	struct registers* _regs;

} sdhci_pci_mmc_bus_info;


device_manager_info* gDeviceManager;
device_module_info* gSDHCIDeviceController;
static pci_x86_module_info* sPCIx86Module;


static void
sdhci_register_dump(uint8_t slot, struct registers* regs)
{
	TRACE("Register values for slot: %d\n", slot);
	TRACE("system_address: %d\n", regs->system_address);
	TRACE("block_size: %d\n", regs->block_size);
	TRACE("block_count: %d\n", regs->block_count);
	TRACE("argument: %d\n", regs->argument);
	TRACE("transfer_mode: %d\n", regs->transfer_mode);
	TRACE("command: %d\n", regs->command);
	TRACE("response0: %d\n", regs->response0);
	TRACE("response2: %d\n", regs->response2);
	TRACE("response4: %d\n", regs->response4);
	TRACE("response6: %d\n", regs->response6);
	TRACE("buffer_data_port: %d\n", regs->buffer_data_port);
	TRACE("present_state: %d\n", regs->present_state);
	TRACE("power_control: %d\n", regs->power_control);
	TRACE("host_control: %d\n", regs->host_control);
	TRACE("wakeup_control: %d\n", regs->wakeup_control);
	TRACE("block_gap_control: %d\n", regs->block_gap_control);
	TRACE("clock_control: %d\n", regs->clock_control);
	TRACE("software_reset: %d\n", regs->software_reset);
	TRACE("timeout_control: %d\n", regs->timeout_control);
	TRACE("interrupt_status: %d\n", regs->interrupt_status);
	TRACE("interrupt_status_enable: %d\n", regs->interrupt_status_enable);
	TRACE("interrupt_signal_enable: %d\n", regs->interrupt_signal_enable);
	TRACE("auto_cmd12_error_status: %d\n", regs->auto_cmd12_error_status);
	TRACE("capabilities: %d\n", regs->capabilities);
	TRACE("capabilities_rsvd: %d\n", regs->capabilities_rsvd);
	TRACE("max_current_capabilities: %d\n",
		regs->max_current_capabilities);
	TRACE("max_current_capabilities_rsvd: %d\n",
		regs->max_current_capabilities_rsvd);
	TRACE("slot_interrupt_status: %d\n", regs->slot_interrupt_status);
	TRACE("host_control_version %d\n", regs->host_control_version);
}


static void
sdhci_reset(struct registers* regs)
{
	// if card is not present then no point of reseting the registers
	if (!(regs->present_state & SDHCI_CARD_DETECT))
		return;

	// enabling software reset all
	regs->software_reset |= SDHCI_SOFTWARE_RESET_ALL;

	// waiting for clock and power to get off
	while (regs->clock_control != 0 && regs->power_control != 0);
}


static void
sdhci_set_clock(struct registers* regs, uint16_t base_clock_div)
{
	uint32_t clock_control = regs->clock_control;
	int base_clock = SDHCI_BASE_CLOCK_FREQ(regs->capabilities);

	TRACE("SDCLK frequency: %dMHz\n", base_clock);

	// clearing previous frequency
	clock_control &= SDHCI_CLR_FREQ_SEL;
	clock_control |= base_clock_div;

	// enabling internal clock
	clock_control |= SDHCI_INTERNAL_CLOCK_ENABLE;
	regs->clock_control = clock_control;

	// waiting till internal clock gets stable
	while (!(regs->clock_control & SDHCI_INTERNAL_CLOCK_STABLE));

	regs->clock_control |= SDHCI_SD_CLOCK_ENABLE; // enabling the SD clock
}


static void
sdhci_stop_clock(struct registers* regs)
{
	regs->clock_control &= SDHCI_SD_CLOCK_DISABLE;
}


static void
sdhci_set_power(struct registers* _regs)
{
	uint16_t command = _regs->command;

	if (SDHCI_VOLTAGE_SUPPORTED(_regs->capabilities))
		if (SDHCI_VOLTAGE_SUPPORTED_33(_regs->capabilities))
			_regs->power_control |= SDHCI_VOLTAGE_SUPPORT_33;
		else if (SDHCI_VOLTAGE_SUPPORTED_30(_regs->capabilities))
			_regs->power_control |= SDHCI_VOLTAGE_SUPPORT_30;
		else
			_regs->power_control |= SDHCI_VOLTAGE_SUPPORT_18;
	else
		TRACE("No voltage is supported\n");

	if (SDHCI_CARD_INSERTED(_regs->present_state) == 0) {
		TRACE("Card not inserted\n");
		return;
	}

	_regs->power_control |= SDHCI_BUS_POWER_ON;
	TRACE("Executed CMD0\n");

	command = SDHCI_RESPONSE_R1 | SDHCI_CMD_CRC_EN
		| SDHCI_CMD_INDEX_EN | SDHCI_CMD_0;
	_regs->command |= command;

	DELAY(1000);
}


static status_t
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();
	status_t status = B_OK;
	area_id	regs_area;
	volatile uint32_t* regs;
	uint8_t bar, slot;

	sdhci_pci_mmc_bus_info* bus = new(std::nothrow) sdhci_pci_mmc_bus_info;
	if (bus == NULL)
		return B_NO_MEMORY;

	pci_info* pciInfo = &bus->info;
	pci_device_module_info* pci;
	pci_device* device;

	device_node* parent = gDeviceManager->get_parent_node(node);
	device_node* pciParent = gDeviceManager->get_parent_node(parent);
	gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
	        (void**)&device);
	gDeviceManager->put_node(pciParent);
	gDeviceManager->put_node(parent);

	if (get_module(B_PCI_X86_MODULE_NAME, (module_info**)&sPCIx86Module)
	    != B_OK) {
	    sPCIx86Module = NULL;
		TRACE("PCIx86Module not loaded\n");
	}

	int msiCount = sPCIx86Module->get_msi_count(pciInfo->bus,
		pciInfo->device, pciInfo->function);

	TRACE("interrupts count: %d\n",msiCount);

	if (gDeviceManager->get_attr_uint8(node, SLOT_NUMBER, &slot, false) < B_OK
		|| gDeviceManager->get_attr_uint8(node, BAR_INDEX, &bar, false) < B_OK)
		return -1;

	bus->node = node;
	bus->pci = pci;
	bus->device = device;

	pci->get_pci_info(device, pciInfo);

	// legacy interrupt
	bus->base_addr = pciInfo->u.h0.base_registers[bar];

	// enable bus master and io
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd &= ~(PCI_command_int_disable | PCI_command_io);
	pcicmd |= PCI_command_master | PCI_command_memory;
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	TRACE("init_bus() %p node %p pci %p device %p\n", bus, node,
		bus->pci, bus->device);

	// mapping the registers by MMUIO method
	int bar_size = pciInfo->u.h0.base_register_sizes[bar];

	regs_area = map_physical_memory("sdhc_regs_map",
		pciInfo->u.h0.base_registers[bar],
		pciInfo->u.h0.base_register_sizes[bar], B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&regs);

	if (regs_area < B_OK) {
		TRACE("mapping failed");
		return B_BAD_VALUE;
	}

	bus->regs_area = regs_area;
	struct registers* _regs = (struct registers*)regs;
	bus->_regs = _regs;
	sdhci_reset(_regs);
	bus->irq = pciInfo->u.h0.interrupt_line;

	TRACE("irq interrupt line: %d\n",bus->irq);

	if (bus->irq == 0 || bus->irq == 0xff) {
		TRACE("PCI IRQ not assigned\n");
		if (sPCIx86Module != NULL) {
			put_module(B_PCI_X86_MODULE_NAME);
			sPCIx86Module = NULL;
		}
		delete bus;
		return B_ERROR;
	}

	status = install_io_interrupt_handler(bus->irq,
		sdhci_generic_interrupt, bus, 0);

	if (status != B_OK) {
		TRACE("can't install interrupt handler\n");
		return status;
	}
	TRACE("interrupt handler installed\n");

	_regs->interrupt_status_enable = SDHCI_INT_CMD_CMP
		| SDHCI_INT_TRANS_CMP | SDHCI_INT_CARD_INS | SDHCI_INT_CARD_REM
		| SDHCI_INT_TIMEOUT | SDHCI_INT_CRC | SDHCI_INT_INDEX
		| SDHCI_INT_BUS_POWER | SDHCI_INT_END_BIT;
	_regs->interrupt_signal_enable =  SDHCI_INT_CMD_CMP
		| SDHCI_INT_TRANS_CMP | SDHCI_INT_CARD_INS | SDHCI_INT_CARD_REM
		| SDHCI_INT_TIMEOUT | SDHCI_INT_CRC | SDHCI_INT_INDEX
		| SDHCI_INT_BUS_POWER | SDHCI_INT_END_BIT;

	sdhci_register_dump(slot, _regs);
	sdhci_set_clock(_regs, SDHCI_BASE_CLOCK_DIV_128);
	sdhci_set_power(_regs);
	sdhci_register_dump(slot, _regs);

	*bus_cookie = bus;
	return status;
}


void
sdhci_error_interrupt_recovery(struct registers* _regs)
{
	_regs->interrupt_signal_enable &= ~(SDHCI_INT_CMD_CMP
		| SDHCI_INT_TRANS_CMP | SDHCI_INT_CARD_INS | SDHCI_INT_CARD_REM);

	if (_regs->interrupt_status & 7) {
		_regs->software_reset |= 1 << 1;
		while (_regs->command);
	}

	int16_t erorr_status = _regs->interrupt_status;
	_regs->interrupt_status &= ~(erorr_status);
}


int32
sdhci_generic_interrupt(void* data)
{
	TRACE("interrupt function called\n");
	sdhci_pci_mmc_bus_info* bus = (sdhci_pci_mmc_bus_info*)data;

	uint16_t intmask, card_present;

	intmask = bus->_regs->slot_interrupt_status;

	if (intmask == 0 || intmask == 0xffffffff) {
		TRACE("invalid command interrupt\n");

		return B_UNHANDLED_INTERRUPT;
	}

	// handling card presence interrupt
	if (intmask & (SDHCI_INT_CARD_INS | SDHCI_INT_CARD_REM)) {
		card_present = ((intmask & SDHCI_INT_CARD_INS) != 0);
		bus->_regs->interrupt_status_enable &= ~(SDHCI_INT_CARD_INS
			| SDHCI_INT_CARD_REM);
		bus->_regs->interrupt_signal_enable &= ~(SDHCI_INT_CARD_INS
			| SDHCI_INT_CARD_REM);

		bus->_regs->interrupt_status_enable |= card_present
		 	? SDHCI_INT_CARD_REM : SDHCI_INT_CARD_INS;
		bus->_regs->interrupt_signal_enable |= card_present
			? SDHCI_INT_CARD_REM : SDHCI_INT_CARD_INS;

		bus->_regs->interrupt_status |= (intmask &
			(SDHCI_INT_CARD_INS | SDHCI_INT_CARD_REM));
		TRACE("Card presence interrupt handled\n");

		return B_HANDLED_INTERRUPT;
	}

	// handling command interrupt
	if (intmask & SDHCI_INT_CMD_MASK) {
		TRACE("interrupt status error: %d\n", bus->_regs->interrupt_status);
		bus->_regs->interrupt_status |= (intmask & SDHCI_INT_CMD_MASK);
		TRACE("Command interrupt handled\n");

		return B_HANDLED_INTERRUPT;
	}

	// handling bus power interrupt
	if (intmask & SDHCI_INT_BUS_POWER) {
		bus->_regs->interrupt_status |= SDHCI_INT_BUS_POWER;
		TRACE("card is consuming too much power\n");

		return B_HANDLED_INTERRUPT;
	}

	intmask &= ~(SDHCI_INT_BUS_POWER | SDHCI_INT_CARD_INS
		|SDHCI_INT_CARD_REM | SDHCI_INT_CMD_MASK);

}


static void
uninit_bus(void* bus_cookie)
{
	sdhci_pci_mmc_bus_info* bus = (sdhci_pci_mmc_bus_info*)bus_cookie;
	delete bus;
}


static void
bus_removed(void* bus_cookie)
{
	return;
}


static status_t
register_child_devices(void* cookie)
{
	CALLED();
	device_node* node = (device_node*)cookie;
	device_node* parent = gDeviceManager->get_parent_node(node);
	pci_device_module_info* pci;
	pci_device* device;
	uint8 slots_count, bar, slotsInfo;

	gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
		(void**)&device);
	uint16 pciSubDeviceId = pci->read_pci_config(device, PCI_subsystem_id, 2);
	slotsInfo = pci->read_pci_config(device, SDHCI_PCI_SLOT_INFO, 1);
	bar = SDHCI_PCI_SLOT_INFO_FIRST_BASE_INDEX(slotsInfo);
	slots_count = SDHCI_PCI_SLOTS(slotsInfo);

	char prettyName[25];

	if (slots_count > 6 || bar > 5) {
		TRACE("Invalid slots count: %d or BAR count: %d \n", slots_count, bar);
		return B_BAD_VALUE;
	}

	for (uint8_t slot = 0; slot <= slots_count; slot++) {

		bar = bar + slot;
		sprintf(prettyName, "SDHC bus %" B_PRIu16 " slot %"
			B_PRIu8, pciSubDeviceId, slot);
		device_attr attrs[] = {
			// properties of this controller for SDHCI bus manager
			{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
				{ string: prettyName }},
			{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
				{string: SDHCI_BUS_CONTROLLER_MODULE_NAME}},
			{SDHCI_DEVICE_TYPE_ITEM, B_UINT16_TYPE,
				{ ui16: pciSubDeviceId}},
			{B_DEVICE_BUS, B_STRING_TYPE,{string: "mmc"}},
			{SLOT_NUMBER, B_UINT8_TYPE,
				{ ui8: slot}},
			{BAR_INDEX, B_UINT8_TYPE,
				{ ui8: bar}},
			{ NULL }
		};
		if (gDeviceManager->register_node(node, SDHCI_PCI_MMC_BUS_MODULE_NAME,
				attrs, NULL, &node) != B_OK)
			return B_BAD_VALUE;
	}
	return B_OK;
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	*device_cookie = node;
	return B_OK;
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "SDHC PCI controller"}},
		{}
	};

	return gDeviceManager->register_node(parent, SDHCI_PCI_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
	uint16 type, subType;
	uint8 pciSubDeviceId;

	// make sure parent is a PCI SDHCI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		!= B_OK || gDeviceManager->get_attr_uint16(parent, B_DEVICE_SUB_TYPE,
		&subType, false) < B_OK || gDeviceManager->get_attr_uint16(parent,
		B_DEVICE_TYPE, &type, false) < B_OK)
		return -1;

	if (strcmp(bus, "pci") != 0)
		return 0.0f;

	if (type == PCI_base_peripheral) {
		if (subType != PCI_sd_host)
			return 0.0f;

		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);
		pciSubDeviceId = pci->read_pci_config(device, PCI_revision, 1);
		TRACE("SDHCI Device found! Subtype: 0x%04x, type: 0x%04x\n",
			subType, type);
		return 0.8f;
	}

	return 0.0f;
}


module_dependency module_dependencies[] = {
	{ SDHCI_BUS_CONTROLLER_MODULE_NAME, (module_info**)&gSDHCIDeviceController},
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};


static sdhci_mmc_bus_interface gSDHCIPCIDeviceModule = {
	{
		{
			SDHCI_PCI_MMC_BUS_MODULE_NAME,
			0,
			NULL
		},
		NULL,	// supports device
		NULL,	// register device
		init_bus,
		uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		bus_removed,
	}
};


static driver_module_info sSDHCIDevice = {
	{
		SDHCI_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},
	supports_device,
	register_device,
	init_device,
	NULL,	// uninit
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};


module_info* modules[] = {
	(module_info* )&sSDHCIDevice,
	(module_info* )&gSDHCIPCIDeviceModule,
	NULL
};
