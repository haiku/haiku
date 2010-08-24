/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
/*-
 * Copyright (C) 2002 Benno Rice.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Benno Rice ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>
#include <platform/openfirmware/pci.h>

#include "pci_controller.h"
#include "pci_io.h"
#include "pci_openfirmware_priv.h"


struct uninorth_range {
	uint32	pci_hi;
	uint32	pci_mid;
	uint32	pci_lo;
	uint32	host;
	uint32	size_hi;
	uint32	size_lo;
};

struct uninorth_host_bridge {
	int						device_node;
	addr_t					address_registers;
	addr_t					data_registers;
	uint32					bus;
	struct uninorth_range	ranges[6];
	int						range_count;
};


#define out8rb(address, value)	ppc_out8((vuint8*)(address), value)
#define out16rb(address, value)	ppc_out16_reverse((vuint16*)(address), value)
#define out32rb(address, value)	ppc_out32_reverse((vuint32*)(address), value)
#define in8rb(address)			ppc_in8((const vuint8*)(address))
#define in16rb(address)			ppc_in16_reverse((const vuint16*)(address))
#define in32rb(address)			ppc_in32_reverse((const vuint32*)(address))


static int		uninorth_enable_config(struct uninorth_host_bridge *bridge,
					uint8 bus, uint8 slot, uint8 function, uint8 offset);

static status_t	uninorth_read_pci_config(void *cookie, uint8 bus, uint8 device,
					uint8 function, uint8 offset, uint8 size, uint32 *value);
static status_t	uninorth_write_pci_config(void *cookie, uint8 bus,
					uint8 device, uint8 function, uint8 offset, uint8 size,
					uint32 value);
static status_t	uninorth_get_max_bus_devices(void *cookie, int32 *count);
static status_t	uninorth_read_pci_irq(void *cookie, uint8 bus, uint8 device,
					uint8 function, uint8 pin, uint8 *irq);
static status_t	uninorth_write_pci_irq(void *cookie, uint8 bus, uint8 device,
					uint8 function, uint8 pin, uint8 irq);

static pci_controller sUniNorthPCIController = {
	uninorth_read_pci_config,
	uninorth_write_pci_config,
	uninorth_get_max_bus_devices,
	uninorth_read_pci_irq,
	uninorth_write_pci_irq,
};


static status_t
uninorth_read_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size, uint32 *value)
{
	uninorth_host_bridge *bridge = (uninorth_host_bridge*)cookie;

	addr_t caoff = bridge->data_registers + (offset & 0x07);

	if (uninorth_enable_config(bridge, bus, device, function, offset) != 0) {
		switch (size) {
			case 1:
				*value = in8rb(caoff);
				break;
			case 2:
				*value = in16rb(caoff);
				break;
			case 4:
				*value = in32rb(caoff);
				break;
			default:
				*value = 0xffffffff;
				break;
		}
	} else
		*value = 0xffffffff;

	return B_OK;
}


static status_t uninorth_write_pci_config(void *cookie, uint8 bus, uint8 device,
	uint8 function, uint8 offset, uint8 size, uint32 value)
{
	uninorth_host_bridge *bridge = (uninorth_host_bridge*)cookie;

	addr_t caoff = bridge->data_registers + (offset & 0x07);

	if (uninorth_enable_config(bridge, bus, device, function, offset)) {
		switch (size) {
			case 1:
				out8rb(caoff, (uint8)value);
				(void)in8rb(caoff);
				break;
			case 2:
				out16rb(caoff, (uint16)value);
				(void)in16rb(caoff);
				break;
			case 4:
				out32rb(caoff, value);
				(void)in32rb(caoff);
				break;
		}
	}

	return B_OK;
}


static status_t uninorth_get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 32;
	return B_OK;
}


static status_t uninorth_read_pci_irq(void *cookie, uint8 bus, uint8 device,
	uint8 function, uint8 pin, uint8 *irq)
{
	return B_ERROR;
}


static status_t uninorth_write_pci_irq(void *cookie, uint8 bus, uint8 device,
	uint8 function, uint8 pin, uint8 irq)
{
	return B_ERROR;
}


// #pragma mark -


static int
uninorth_enable_config(struct uninorth_host_bridge *bridge, uint8 bus,
	uint8 slot, uint8 function, uint8 offset)
{
//	uint32 pass;
// 	if (resource_int_value(device_get_name(sc->sc_dev),
// 	        device_get_unit(sc->sc_dev), "skipslot", &pass) == 0) {
// 		if (pass == slot)
// 			return (0);
// 	}

	uint32 cfgval;
	if (bridge->bus == bus) {
		/*
		 * No slots less than 11 on the primary bus
		 */
		if (slot < 11)
			return (0);

		cfgval = (1 << slot) | (function << 8) | (offset & 0xfc);
	} else {
		cfgval = (bus << 16) | (slot << 11) | (function << 8) |
		    (offset & 0xfc) | 1;
	}

	do {
		out32rb(bridge->address_registers, cfgval);
	} while (in32rb(bridge->address_registers) != cfgval);

	return (1);
}


// #pragma mark -

status_t
ppc_openfirmware_probe_uninorth(int deviceNode,
	const StringArrayPropertyValue &compatibleValue)
{
	if (!compatibleValue.ContainsElement("uni-north"))
		return B_ERROR;

	uint32 reg[2];
	if (of_getprop(deviceNode, "reg", reg, sizeof(reg)) < 8)
		return B_ERROR;

	uint32 busrange[2];
	if (of_getprop(deviceNode, "bus-range", busrange, sizeof(busrange)) != 8)
		return B_ERROR;

	uninorth_host_bridge *bridge
		= (uninorth_host_bridge*)malloc(sizeof(uninorth_host_bridge));
	if (!bridge)
		return B_NO_MEMORY;

	bridge->device_node = deviceNode;
	bridge->address_registers = reg[0] + 0x800000;
	bridge->data_registers = reg[0] + 0xc00000;
	bridge->bus = busrange[0];
// TODO: Check whether address and data registers have already been mapped by
// the Open Firmware. If not, map them ourselves.

	memset(bridge->ranges, 0, sizeof(bridge->ranges));
	int bytesRead = of_getprop(deviceNode, "ranges", bridge->ranges,
	    sizeof(bridge->ranges));

	if (bytesRead < 0) {
		dprintf("ppc_openfirmware_probe_uninorth: Could not get ranges.\n");
		free(bridge);
		return B_ERROR;
	}
	bridge->range_count = bytesRead / sizeof(uninorth_range);

	uninorth_range *ioRange = NULL;
	uninorth_range *memoryRanges[2];
	int memoryRangeCount = 0;

	for (int i = 0; i < bridge->range_count; i++) {
		uninorth_range *range = bridge->ranges + i;
		switch (range->pci_hi & OFW_PCI_PHYS_HI_SPACEMASK) {
			case OFW_PCI_PHYS_HI_SPACE_CONFIG:
				break;
			case OFW_PCI_PHYS_HI_SPACE_IO:
				ioRange = range;
				break;
			case OFW_PCI_PHYS_HI_SPACE_MEM32:
				memoryRanges[memoryRangeCount++] = range;
				break;
			case OFW_PCI_PHYS_HI_SPACE_MEM64:
				break;
		}
	}

	if (ioRange == NULL) {
		dprintf("ppc_openfirmware_probe_uninorth: Can't find io range.\n");
		free(bridge);
		return B_ERROR;
	}

	if (memoryRangeCount == 0) {
		dprintf("ppc_openfirmware_probe_uninorth: Can't find mem ranges.\n");
		free(bridge);
		return B_ERROR;
	}

	status_t error = pci_controller_add(&sUniNorthPCIController, bridge);
	if (error != B_OK)
		free(bridge);
	return error;
}
