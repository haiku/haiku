/*
 * Copyright 2018-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Ron Ben Aroya, sed4906birdie@gmail.com
 */
#include <algorithm>
#include <new>
#include <stdio.h>
#include <string.h>

#include <bus/PCI.h>
#include <ACPI.h>
#include "acpi.h"

#include <KernelExport.h>

#include "IOSchedulerSimple.h"
#include "mmc.h"
#include "sdhci.h"


//#define TRACE_SDHCI
#ifdef TRACE_SDHCI
#	define TRACE(x...) dprintf("\33[33msdhci:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33msdhci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33msdhci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define SDHCI_DEVICE_MODULE_NAME "busses/mmc/sdhci/driver_v1"
#define SDHCI_ACPI_MMC_BUS_MODULE_NAME "busses/mmc/sdhci/acpi/device/v1"

static acpi_status
sdhci_acpi_scan_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct sdhci_crs* crs = (struct sdhci_crs*)context;

	if (res->Type == ACPI_RESOURCE_TYPE_FIXED_MEMORY32) {
		crs->addr_bas = res->Data.FixedMemory32.Address;
		crs->addr_len = res->Data.FixedMemory32.AddressLength;
	} else if (res->Type == ACPI_RESOURCE_TYPE_IRQ) {
		crs->irq = res->Data.Irq.Interrupt;
		//crs->irq_triggering = res->Data.Irq.Triggering;
		//crs->irq_polarity = res->Data.Irq.Polarity;
		//crs->irq_shareable = res->Data.Irq.Shareable;
	} else if (res->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
		crs->irq = res->Data.ExtendedIrq.Interrupt;
		//crs->irq_triggering = res->Data.ExtendedIrq.Triggering;
		//crs->irq_polarity = res->Data.ExtendedIrq.Polarity;
		//crs->irq_shareable = res->Data.ExtendedIrq.Shareable;
	}

	return B_OK;
}

status_t
init_bus_acpi(device_node* node, void** bus_cookie)
{
	CALLED();

	// Get the ACPI driver and device
	acpi_device_module_info* acpi;
	acpi_device device;

	device_node* parent = gDeviceManager->get_parent_node(node);
	device_node* acpiParent = gDeviceManager->get_parent_node(parent);
	gDeviceManager->get_driver(acpiParent, (driver_module_info**)&acpi,
			(void**)&device);
	gDeviceManager->put_node(acpiParent);
	gDeviceManager->put_node(parent);

	// Ignore invalid bars
	TRACE("Register SD bus\n");

	struct sdhci_crs crs;
	if(acpi->walk_resources(device, (ACPI_STRING)"_CRS",
			sdhci_acpi_scan_parse_callback, &crs) != B_OK) {
		ERROR("Couldn't scan ACPI register set\n");
		return B_IO_ERROR;
	}

	TRACE("addr: %" B_PRIx32 " len: %" B_PRIx32 "\n", crs.addr_bas, crs.addr_len);

	if (crs.addr_bas == 0 || crs.addr_len == 0) {
		ERROR("No registers to map\n");
		return B_IO_ERROR;
	}

	// map the slot registers
	area_id	regs_area;
	struct registers* _regs;
	regs_area = map_physical_memory("sdhc_regs_map",
		crs.addr_bas, crs.addr_len, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&_regs);

	if (regs_area < B_OK) {
		ERROR("Could not map registers\n");
		return B_BAD_VALUE;
	}

	// the interrupt is shared between all busses in an SDHC controller, but
	// they each register an handler. Not a problem, we will just test the
	// interrupt registers for all busses one after the other and find no
	// interrupts on the idle busses.
	uint8_t irq = crs.irq;
	TRACE("irq interrupt line: %d\n", irq);

	SdhciBus* bus = new(std::nothrow) SdhciBus(_regs, irq, true);

	status_t status = B_NO_MEMORY;
	if (bus != NULL)
		status = bus->InitCheck();

	if (status != B_OK) {
		if (bus != NULL)
			delete bus;
		else
			delete_area(regs_area);
		return status;
	}

	// Store the created object as a cookie, allowing users of the bus to
	// locate it.
	*bus_cookie = bus;

	return status;
}

status_t
register_child_devices_acpi(void* cookie)
{
	CALLED();
	SdhciDevice* context = (SdhciDevice*)cookie;
	device_node* parent = gDeviceManager->get_parent_node(context->fNode);
	acpi_device_module_info* acpi;
	acpi_device* device;

	gDeviceManager->get_driver(parent, (driver_module_info**)&acpi,
		(void**)&device);

	TRACE("register_child_devices\n");

	char prettyName[25];

	sprintf(prettyName, "SDHC bus");
	device_attr attrs[] = {
		// properties of this controller for mmc bus manager
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = prettyName } },
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{.string = MMC_BUS_MODULE_NAME} },
		{ B_DEVICE_BUS, B_STRING_TYPE, {.string = "mmc"} },

		// DMA properties
		// The high alignment is to force access only to complete sectors
		// These constraints could be removed by using ADMA which allows
		// use of the full 64bit address space and can do scatter-gather.
		{ B_DMA_ALIGNMENT, B_UINT32_TYPE, { .ui32 = 511 }},
		{ B_DMA_HIGH_ADDRESS, B_UINT64_TYPE, { .ui64 = 0x100000000LL }},
		{ B_DMA_BOUNDARY, B_UINT32_TYPE, { .ui32 = (1 << 19) - 1 }},
		{ B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE, { .ui32 = 1 }},
		{ B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE, { .ui32 = (1 << 10) - 1 }},

		// private data to identify device
		{ NULL }
	};
	device_node* node;
	if (gDeviceManager->register_node(context->fNode,
			SDHCI_ACPI_MMC_BUS_MODULE_NAME, attrs, NULL,
			&node) != B_OK)
		return B_BAD_VALUE;
	return B_OK;
}

float
supports_device_acpi(device_node* parent)
{
	const char* hid;
	const char* uid;
	uint32 type;

	if (gDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM, &type, false)
		|| type != ACPI_TYPE_DEVICE) {
		return 0.0f;
	}

	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid, false)) {
		TRACE("No hid attribute\n");
		return 0.0f;
	}

	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_UID_ITEM, &uid, false)) {
		TRACE("No uid attribute\n");
		return 0.0f;
	}

	TRACE("supports_device(hid:%s uid:%s)\n", hid, uid);

	if (!(strcmp(hid, "80860F14") == 0
			||	strcmp(hid, "80860F16") == 0
			||	strcmp(hid, "80865ACA") == 0
			||	strcmp(hid, "80865AD0") == 0
			||	strcmp(hid, "INT33C6") == 0
			||	strcmp(hid, "INT3436") == 0
			||	strcmp(hid, "INT344D") == 0
			||	strcmp(hid, "INT33BB") == 0
			||	strcmp(hid, "NXP0003") == 0
			||	strcmp(hid, "RKCP0D40") == 0
			||	strcmp(hid, "PNP0D40") == 0))
		return 0.0f;

	acpi_device_module_info* acpi;
	acpi_device* device;
	gDeviceManager->get_driver(parent, (driver_module_info**)&acpi,
		(void**)&device);
	TRACE("SDHCI Device found! hid: %s, uid: %s\n", hid, uid);

	return 0.8f;
}

// Device node registered for each SD slot. It implements the MMC operations so
// the bus manager can use it to communicate with SD cards.
mmc_bus_interface gSDHCIACPIDeviceModule = {
	.info = {
		.info = {
			.name = SDHCI_ACPI_MMC_BUS_MODULE_NAME,
		},

		.init_driver = init_bus_acpi,
		.uninit_driver = uninit_bus,
		.device_removed = bus_removed,
	},

	.set_clock = set_clock,
	.execute_command = execute_command,
	.do_io = do_io,
	.set_scan_semaphore = set_scan_semaphore,
	.set_bus_width = set_bus_width,
};
