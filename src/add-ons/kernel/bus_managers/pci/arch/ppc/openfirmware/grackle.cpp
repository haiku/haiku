/*
 * Copyright 2010 Andreas Färber <andreas.faerber@web.de>
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <KernelExport.h>

#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>
#include <platform/openfirmware/pci.h>

#include "pci_controller.h"
#include "pci_io.h"
#include "pci_openfirmware_priv.h"


//#define TRACE_GRACKLE

#ifdef TRACE_GRACKLE
#define TRACE(fmt...) dprintf(fmt)
#else
#define TRACE(fmt...) ;
#endif


struct grackle_range {
	uint32_t	pci_hi;
	uint32_t	pci_mid;
	uint32_t	pci_lo;
	uint32_t	host;
	uint32_t	size_hi;
	uint32_t	size_lo;
};


struct grackle_host_bridge {
	int						device_node;
	addr_t					address_registers;
	addr_t					data_registers;
	uint32					bus;
	struct grackle_range	ranges[6];
	int						range_count;
};


#define out8rb(address, value)	ppc_out8((vuint8*)(address), value)
#define out16rb(address, value)	ppc_out16_reverse((vuint16*)(address), value)
#define out32rb(address, value)	ppc_out32_reverse((vuint32*)(address), value)
#define in8rb(address)			ppc_in8((const vuint8*)(address))
#define in16rb(address)			ppc_in16_reverse((const vuint16*)(address))
#define in32rb(address)			ppc_in32_reverse((const vuint32*)(address))


static status_t
grackle_read_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size, uint32 *value)
{
	grackle_host_bridge *bridge = (grackle_host_bridge*)cookie;
	TRACE("grackle_read_pci_config(bus=%u, dev=%u, func=%u, offset=%u, "
		"size=%u)\n", (int)bus, (int)device, (int)function, (int)offset,
		(int)size);

	out32rb(bridge->address_registers, (1 << 31)
		| (bus << 16) | ((device & 0x1f) << 11) | ((function & 0x7) << 8)
		| (offset & 0xfc));

	addr_t dataAddress = bridge->data_registers + (offset & 0x3);
	switch (size) {
		case 1:
			*value = in8rb(dataAddress);
			break;
		case 2:
			*value = in16rb(dataAddress);
			break;
		case 4:
			*value = in32rb(dataAddress);
			break;
		default:
			*value = 0xffffffff;
			break;
	}

	out32rb(bridge->address_registers, 0);

	return B_OK;
}


static status_t
grackle_write_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size, uint32 value)
{
	grackle_host_bridge *bridge = (grackle_host_bridge*)cookie;
	TRACE("grackle_write_pci_config(bus=%u, dev=%u, func=%u, offset=%u, "
		"size=%u, value=%lu)\n", (int)bus, (int)device, (int)function,
		(int)offset, (int)size, value);

	out32rb(bridge->address_registers, (1 << 31)
		| (bus << 16) | ((device & 0x1f) << 11) | ((function & 0x7) << 8)
		| (offset & 0xfc));

	addr_t dataAddress = bridge->data_registers + (offset & 0x3);
	switch (size) {
		case 1:
			out8rb(dataAddress, (uint8)value);
			break;
		case 2:
			out16rb(dataAddress, (uint16)value);
			break;
		case 4:
			out32rb(dataAddress, value);
			break;
	}

	out32rb(bridge->address_registers, 0);

	return B_OK;
}


static status_t
grackle_get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 32;
	return B_OK;
}


static status_t
grackle_read_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 *irq)
{
	return B_ERROR;
}


static status_t
grackle_write_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 irq)
{
	return B_ERROR;
}


static pci_controller sGracklePCIController = {
	grackle_read_pci_config,
	grackle_write_pci_config,
	grackle_get_max_bus_devices,
	grackle_read_pci_irq,
	grackle_write_pci_irq,
};


status_t
ppc_openfirmware_probe_grackle(int deviceNode,
	const StringArrayPropertyValue &compatibleValue)
{
	if (!compatibleValue.ContainsElement("grackle"))
		return B_ERROR;

	uint32_t busrange[2];
	if (of_getprop(deviceNode, "bus-range", busrange, sizeof(busrange)) != 8)
		return B_ERROR;

	grackle_host_bridge *bridge =
		(grackle_host_bridge*)malloc(sizeof(grackle_host_bridge));
	if (bridge == NULL)
		return B_NO_MEMORY;

	bridge->device_node = deviceNode;
	bridge->address_registers = 0xfec00000;
	bridge->data_registers = 0xfee00000;
	bridge->bus = busrange[0];

	memset(bridge->ranges, 0, sizeof(bridge->ranges));
	int bytesRead = of_getprop(deviceNode, "ranges", bridge->ranges,
	    sizeof(bridge->ranges));
	if (bytesRead < 0) {
		dprintf("ppc_openfirmware_probe_grackle: Could not get ranges.\n");
		free(bridge);
		return B_ERROR;
	}
	bridge->range_count = bytesRead / sizeof(grackle_range);

	grackle_range *ioRange = NULL;
	grackle_range *memoryRanges[2];
	int memoryRangeCount = 0;

	for (int i = 0; i < bridge->range_count; i++) {
		grackle_range *range = bridge->ranges + i;
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
		dprintf("ppc_openfirmware_probe_grackle: Can't find io range.\n");
		free(bridge);
		return B_ERROR;
	}

	if (memoryRangeCount == 0) {
		dprintf("ppc_openfirmware_probe_grackle: Can't find mem ranges.\n");
		free(bridge);
		return B_ERROR;
	}

	status_t error = pci_controller_add(&sGracklePCIController, bridge);
	if (error != B_OK)
		free(bridge);
	return error;
}
