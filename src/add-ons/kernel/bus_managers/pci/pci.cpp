/*
 * Copyright 2003-2008, Marcus Overhagen. All rights reserved.
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <string.h>
#include <KernelExport.h>
#define __HAIKU_PCI_BUS_MANAGER_TESTING 1
#include <PCI.h>
#include <arch/generic/msi.h>
#if defined(__i386__) || defined(__x86_64__)
#include <arch/x86/msi.h>
#endif

#include "util/kernel_cpp.h"
#include "pci_fixup.h"
#include "pci_info.h"
#include "pci_private.h"
#include "pci.h"


#define TRACE_CAP(x...) dprintf(x)
#define FLOW(x...)
//#define FLOW(x...) dprintf(x)


PCI *gPCI;


// #pragma mark bus manager exports


long
pci_get_nth_pci_info(long index, pci_info *outInfo)
{
	return gPCI->GetNthInfo(index, outInfo);
}


uint32
pci_read_config(uint8 virtualBus, uint8 device, uint8 function, uint16 offset,
	uint8 size)
{
	uint8 bus;
	uint8 domain;
	uint32 value;

	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return 0xffffffff;

	if (gPCI->ReadConfig(domain, bus, device, function, offset, size,
			&value) != B_OK)
		return 0xffffffff;

	return value;
}


void
pci_write_config(uint8 virtualBus, uint8 device, uint8 function, uint16 offset,
	uint8 size, uint32 value)
{
	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return;

	gPCI->WriteConfig(domain, bus, device, function, offset, size, value);
}


phys_addr_t
pci_ram_address(phys_addr_t childAdr)
{
	phys_addr_t hostAdr = 0;
#if defined(__i386__) || defined(__x86_64__)
	hostAdr = childAdr;
#else
	uint8 domain;
	pci_resource_range range;
	if (gPCI->LookupRange(B_IO_MEMORY, childAdr, domain, range) >= B_OK)
		hostAdr = childAdr - range.pci_address + range.host_address;
#endif
	//dprintf("pci_ram_address(%#" B_PRIx64 ") -> %#" B_PRIx64 "\n", childAdr, hostAdr);
	return hostAdr;
}


status_t
pci_find_capability(uint8 virtualBus, uint8 device, uint8 function,
	uint8 capID, uint8 *offset)
{
	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	return gPCI->FindCapability(domain, bus, device, function, capID, offset);
}


status_t
pci_find_extended_capability(uint8 virtualBus, uint8 device, uint8 function,
	uint16 capID, uint16 *offset)
{
	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	return gPCI->FindExtendedCapability(domain, bus, device, function, capID,
		offset);
}


status_t
pci_reserve_device(uchar virtualBus, uchar device, uchar function,
	const char *driverName, void *nodeCookie)
{
	status_t status;
	uint8 bus;
	uint8 domain;
	TRACE(("pci_reserve_device(%d, %d, %d, %s)\n", virtualBus, device, function,
		driverName));

	/*
	 * we add 2 nodes to the PCI devices, one with constant attributes,
	 * so adding for another driver fails, and a subnode with the
	 * driver-provided informations.
	 */

	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	//TRACE(("%s(%d [%d:%d], %d, %d, %s, %p)\n", __FUNCTION__, virtualBus,
	//	domain, bus, device, function, driverName, nodeCookie));

	device_attr matchThis[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},

		// location on PCI bus
		{B_PCI_DEVICE_DOMAIN, B_UINT8_TYPE, {.ui8 = domain}},
		{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {.ui8 = bus}},
		{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {.ui8 = device}},
		{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {.ui8 = function}},
		{NULL}
	};
	device_attr legacyAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "legacy_driver"}},
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "Legacy Driver Reservation"}},
		{NULL}
	};
	device_attr drvAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "legacy_driver"}},
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = driverName}},
		{"legacy_driver", B_STRING_TYPE, {.string = driverName}},
		{"legacy_driver_cookie", B_UINT64_TYPE, {.ui64 = (uint64)nodeCookie}},
		{NULL}
	};
	device_node *node, *legacy;

	status = B_DEVICE_NOT_FOUND;
	device_node *root_pci_node = gPCI->_GetDomainData(domain)->root_node;

	node = NULL;
	if (gDeviceManager->get_next_child_node(root_pci_node,
		matchThis, &node) < B_OK) {
		goto err1;
	}

	// common API for all legacy modules ?
	//status = legacy_driver_register(node, driverName, nodeCookie, PCI_LEGACY_DRIVER_MODULE_NAME);

	status = gDeviceManager->register_node(node, PCI_LEGACY_DRIVER_MODULE_NAME,
		legacyAttrs, NULL, &legacy);
	if (status < B_OK)
		goto err2;

	status = gDeviceManager->register_node(legacy, PCI_LEGACY_DRIVER_MODULE_NAME,
		drvAttrs, NULL, NULL);
	if (status < B_OK)
		goto err3;

	gDeviceManager->put_node(node);

	return B_OK;

err3:
	gDeviceManager->unregister_node(legacy);
err2:
	gDeviceManager->put_node(node);
err1:
	TRACE(("pci_reserve_device for driver %s failed: %s\n", driverName,
		strerror(status)));
	return status;
}


status_t
pci_unreserve_device(uchar virtualBus, uchar device, uchar function,
	const char *driverName, void *nodeCookie)
{
	status_t status;
	uint8 bus;
	uint8 domain;
	TRACE(("pci_unreserve_device(%d, %d, %d, %s)\n", virtualBus, device,
		function, driverName));

	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	//TRACE(("%s(%d [%d:%d], %d, %d, %s, %p)\n", __FUNCTION__, virtualBus,
	//	domain, bus, device, function, driverName, nodeCookie));

	device_attr matchThis[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},

		// location on PCI bus
		{B_PCI_DEVICE_DOMAIN, B_UINT8_TYPE, {.ui8 = domain}},
		{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {.ui8 = bus}},
		{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {.ui8 = device}},
		{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {.ui8 = function}},
		{NULL}
	};
	device_attr legacyAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "legacy_driver"}},
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "Legacy Driver Reservation"}},
		{NULL}
	};
	device_attr drvAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "legacy_driver"}},
		{"legacy_driver", B_STRING_TYPE, {.string = driverName}},
		{"legacy_driver_cookie", B_UINT64_TYPE, {.ui64 = (uint64)nodeCookie}},
		{NULL}
	};
	device_node *pci, *node, *legacy, *drv;

	status = B_DEVICE_NOT_FOUND;

	pci = gPCI->_GetDomainData(domain)->root_node;

	node = NULL;
	if (gDeviceManager->get_next_child_node(pci, matchThis, &node) < B_OK)
		goto err1;

	// common API for all legacy modules ?
	//status = legacy_driver_unregister(node, driverName, nodeCookie);

	legacy = NULL;
	if (gDeviceManager->get_next_child_node(node, legacyAttrs, &legacy) < B_OK)
		goto err2;

	drv = NULL;
	if (gDeviceManager->get_next_child_node(legacy, drvAttrs, &drv) < B_OK)
		goto err3;

	gDeviceManager->put_node(drv);
	status = gDeviceManager->unregister_node(drv);
	//dprintf("unreg:drv:%s\n", strerror(status));

	gDeviceManager->put_node(legacy);
	status = gDeviceManager->unregister_node(legacy);
	//dprintf("unreg:legacy:%s\n", strerror(status));
	// we'll get EBUSY here anyway...

	gDeviceManager->put_node(node);
	return B_OK;

err3:
	gDeviceManager->put_node(legacy);
err2:
	gDeviceManager->put_node(node);
err1:
	TRACE(("pci_unreserve_device for driver %s failed: %s\n", driverName,
		strerror(status)));
	return status;
}


status_t
pci_update_interrupt_line(uchar virtualBus, uchar device, uchar function,
	uchar newInterruptLineValue)
{
	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	return gPCI->UpdateInterruptLine(domain, bus, device, function,
		newInterruptLineValue);
}


status_t
pci_get_powerstate(uchar virtualBus, uint8 device, uint8 function, uint8* state)
{
	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	return gPCI->GetPowerstate(domain, bus, device, function, state);
}


status_t
pci_set_powerstate(uchar virtualBus, uint8 device, uint8 function, uint8 newState)
{
	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	return gPCI->SetPowerstate(domain, bus, device, function, newState);
}


// used by pci_info.cpp print_info_basic()
void
__pci_resolve_virtual_bus(uint8 virtualBus, uint8 *domain, uint8 *bus)
{
	if (gPCI->ResolveVirtualBus(virtualBus, domain, bus) < B_OK)
		panic("ResolveVirtualBus failed");
}


// #pragma mark kernel debugger commands


static int
display_io(int argc, char **argv)
{
	int32 displayWidth;
	int32 itemSize;
	int32 num = 1;
	int address;
	int i = 1, j;

	switch (argc) {
	case 3:
		num = atoi(argv[2]);
	case 2:
		address = strtoul(argv[1], NULL, 0);
		break;
	default:
		kprintf("usage: %s <address> [num]\n", argv[0]);
		return 0;
	}

	// build the format string
	if (strcmp(argv[0], "inb") == 0 || strcmp(argv[0], "in8") == 0) {
		itemSize = 1;
		displayWidth = 16;
	} else if (strcmp(argv[0], "ins") == 0 || strcmp(argv[0], "in16") == 0) {
		itemSize = 2;
		displayWidth = 8;
	} else if (strcmp(argv[0], "inw") == 0 || strcmp(argv[0], "in32") == 0) {
		itemSize = 4;
		displayWidth = 4;
	} else {
		kprintf("display_io called in an invalid way!\n");
		return 0;
	}

	for (i = 0; i < num; i++) {
		if ((i % displayWidth) == 0) {
			int32 displayed = min_c(displayWidth, (num-i)) * itemSize;
			if (i != 0)
				kprintf("\n");

			kprintf("[0x%" B_PRIx32 "]  ", address + i * itemSize);

			if (num > displayWidth) {
				// make sure the spacing in the last line is correct
				for (j = displayed; j < displayWidth * itemSize; j++)
					kprintf(" ");
			}
			kprintf("  ");
		}

		switch (itemSize) {
			case 1:
				kprintf(" %02" B_PRIx8, pci_read_io_8(address + i * itemSize));
				break;
			case 2:
				kprintf(" %04" B_PRIx16, pci_read_io_16(address + i * itemSize));
				break;
			case 4:
				kprintf(" %08" B_PRIx32, pci_read_io_32(address + i * itemSize));
				break;
		}
	}

	kprintf("\n");
	return 0;
}


static int
write_io(int argc, char **argv)
{
	int32 itemSize;
	uint32 value;
	int address;
	int i = 1;

	if (argc < 3) {
		kprintf("usage: %s <address> <value> [value [...]]\n", argv[0]);
		return 0;
	}

	address = strtoul(argv[1], NULL, 0);

	if (strcmp(argv[0], "outb") == 0 || strcmp(argv[0], "out8") == 0) {
		itemSize = 1;
	} else if (strcmp(argv[0], "outs") == 0 || strcmp(argv[0], "out16") == 0) {
		itemSize = 2;
	} else if (strcmp(argv[0], "outw") == 0 || strcmp(argv[0], "out32") == 0) {
		itemSize = 4;
	} else {
		kprintf("write_io called in an invalid way!\n");
		return 0;
	}

	// skip cmd name and address
	argv += 2;
	argc -= 2;

	for (i = 0; i < argc; i++) {
		value = strtoul(argv[i], NULL, 0);
		switch (itemSize) {
			case 1:
				pci_write_io_8(address + i * itemSize, value);
				break;
			case 2:
				pci_write_io_16(address + i * itemSize, value);
				break;
			case 4:
				pci_write_io_32(address + i * itemSize, value);
				break;
		}
	}

	return 0;
}


static int
pcistatus(int argc, char **argv)
{
	for (uint32 domain = 0; ; domain++) {
		domain_data *data = gPCI->_GetDomainData(domain);
		if (data == NULL)
			break;

		gPCI->ClearDeviceStatus(data->bus, true);
	}

	return 0;
}


static int
pcirefresh(int argc, char **argv)
{
	gPCI->RefreshDeviceInfo();
	pci_print_info();
	return 0;
}


// #pragma mark bus manager init/uninit


status_t
pci_init(void)
{
	gPCI = new PCI;

	add_debugger_command("inw", &display_io, "dump io words (32-bit)");
	add_debugger_command("in32", &display_io, "dump io words (32-bit)");
	add_debugger_command("ins", &display_io, "dump io shorts (16-bit)");
	add_debugger_command("in16", &display_io, "dump io shorts (16-bit)");
	add_debugger_command("inb", &display_io, "dump io bytes (8-bit)");
	add_debugger_command("in8", &display_io, "dump io bytes (8-bit)");

	add_debugger_command("outw", &write_io, "write io words (32-bit)");
	add_debugger_command("out32", &write_io, "write io words (32-bit)");
	add_debugger_command("outs", &write_io, "write io shorts (16-bit)");
	add_debugger_command("out16", &write_io, "write io shorts (16-bit)");
	add_debugger_command("outb", &write_io, "write io bytes (8-bit)");
	add_debugger_command("out8", &write_io, "write io bytes (8-bit)");

	add_debugger_command("pcistatus", &pcistatus, "dump and clear pci device status registers");
	add_debugger_command("pcirefresh", &pcirefresh, "refresh and print all pci_info");

	return B_OK;
}


void
pci_uninit(void)
{
	delete gPCI;

	remove_debugger_command("outw", &write_io);
	remove_debugger_command("out32", &write_io);
	remove_debugger_command("outs", &write_io);
	remove_debugger_command("out16", &write_io);
	remove_debugger_command("outb", &write_io);
	remove_debugger_command("out8", &write_io);

	remove_debugger_command("inw", &display_io);
	remove_debugger_command("in32", &display_io);
	remove_debugger_command("ins", &display_io);
	remove_debugger_command("in16", &display_io);
	remove_debugger_command("inb", &display_io);
	remove_debugger_command("in8", &display_io);

	remove_debugger_command("pcistatus", &pcistatus);
	remove_debugger_command("pcirefresh", &pcirefresh);
}


// #pragma mark PCI class


PCI::PCI()
	:
	fDomainCount(0),
	fBusEnumeration(false),
	fVirtualBusMap(),
	fNextVirtualBus(0)
{
	#if defined(__POWERPC__) || defined(__M68K__)
		fBusEnumeration = true;
	#endif
}


void
PCI::InitBus(PCIBus *bus)
{
	if (fBusEnumeration) {
		_EnumerateBus(bus->domain, 0);
	}

	if (1) {
		_FixupDevices(bus->domain, 0);
	}

	_DiscoverBus(bus);
	_ConfigureBridges(bus);
	ClearDeviceStatus(bus, false);
	_RefreshDeviceInfo(bus);
}


PCI::~PCI()
{
}


status_t
PCI::_CreateVirtualBus(uint8 domain, uint8 bus, uint8 *virtualBus)
{
#if defined(__i386__) || defined(__x86_64__)

	// IA32 doesn't use domains
	if (domain)
		panic("PCI::CreateVirtualBus domain != 0");
	*virtualBus = bus;
	return B_OK;

#else

	if (fNextVirtualBus > 0xff)
		panic("PCI::CreateVirtualBus: virtual bus number space exhausted");

	uint16 value = domain << 8 | bus;

	for (VirtualBusMap::Iterator it = fVirtualBusMap.Begin();
		it != fVirtualBusMap.End(); ++it) {
		if (it->Value() == value) {
			*virtualBus = it->Key();
			FLOW("PCI::CreateVirtualBus: domain %d, bus %d already in map => "
				"virtualBus %d\n", domain, bus, *virtualBus);
			return B_OK;
		}
	}

	*virtualBus = fNextVirtualBus++;

	FLOW("PCI::CreateVirtualBus: domain %d, bus %d => virtualBus %d\n", domain,
		bus, *virtualBus);

	return fVirtualBusMap.Insert(*virtualBus, value);

#endif
}


status_t
PCI::ResolveVirtualBus(uint8 virtualBus, uint8 *domain, uint8 *bus)
{
#if defined(__i386__) || defined(__x86_64__)

	// IA32 doesn't use domains
	*bus = virtualBus;
	*domain = 0;
	return B_OK;

#else

	if (virtualBus >= fNextVirtualBus)
		return B_ERROR;

	uint16 value = fVirtualBusMap.Get(virtualBus);
	*domain = value >> 8;
	*bus = value & 0xff;
	return B_OK;

#endif
}


status_t
PCI::AddController(pci_controller_module_info *controller,
	void *controllerCookie, device_node *rootNode, domain_data **domainData)
{
	if (fDomainCount == MAX_PCI_DOMAINS)
		return B_ERROR;

	uint8 domain = fDomainCount;
	domain_data& data = fDomainData[domain];

	data.controller = controller;
	data.controller_cookie = controllerCookie;
	data.root_node = rootNode;

	data.bus = new(std::nothrow) PCIBus {.domain = domain};
	if (data.bus == NULL)
		return B_NO_MEMORY;

	// initialized later to avoid call back into controller at this point
	data.max_bus_devices = -1;

	fDomainCount++;

	InitDomainData(data);
	InitBus(data.bus);
	if (data.controller->finalize != NULL)
		data.controller->finalize(data.controller_cookie);
	_RefreshDeviceInfo(data.bus);

	pci_print_info();

	*domainData = &data;
	return B_OK;
}


status_t
PCI::LookupRange(uint32 type, phys_addr_t pciAddr,
	uint8 &domain, pci_resource_range &range, uint8 **mappedAdr)
{
	for (uint8 curDomain = 0; curDomain < fDomainCount; curDomain++) {
		const auto &ranges = fDomainData[curDomain].ranges;

		for (int32 i = 0; i < ranges.Count(); i++) {
			const pci_resource_range curRange = ranges[i];
			if (curRange.type != type)
				continue;

			if (pciAddr >= curRange.pci_address
					&& pciAddr < (curRange.pci_address + curRange.size)) {
				domain = curDomain;
				range = curRange;
#if !(defined(__i386__) || defined(__x86_64__))
				if (type == B_IO_PORT && mappedAdr != NULL)
					*mappedAdr = fDomainData[curDomain].io_port_adr;
#endif
				return B_OK;
			}
		}
	}

	return B_ENTRY_NOT_FOUND;
}


void
PCI::InitDomainData(domain_data &data)
{
	int32 count;
	status_t status;

	pci_controller_module_info *ctrl = data.controller;
	void *ctrlCookie = data.controller_cookie;

	status = ctrl->get_max_bus_devices(ctrlCookie, &count);
	data.max_bus_devices = (status == B_OK) ? count : 0;

	pci_resource_range range;
	for (uint32 j = 0; ctrl->get_range(ctrlCookie, j, &range) >= B_OK; j++)
		data.ranges.Add(range);

#if !(defined(__i386__) || defined(__x86_64__))
	for (int32 i = 0; i < data.ranges.Count(); i++) {
		pci_resource_range &ioPortRange = data.ranges[i];
		if (ioPortRange.type != B_IO_PORT)
			continue;

		if (ioPortRange.size > 0) {
			data.io_port_area = map_physical_memory("PCI IO Ports",
				ioPortRange.host_address, ioPortRange.size, B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&data.io_port_adr);

			if (data.io_port_area < B_OK)
				data.io_port_adr = NULL;

			// TODO: Map other IO ports (if any.)
			break;
		}
	}
#endif
}


domain_data *
PCI::_GetDomainData(uint8 domain)
{
	if (domain >= fDomainCount)
		return NULL;

	return &fDomainData[domain];
}


inline int
PCI::_NumFunctions(uint8 domain, uint8 bus, uint8 device)
{
	uint8 type = ReadConfig(domain, bus, device,
		0, PCI_header_type, 1);
	return (type & PCI_multifunction) != 0 ? 8 : 1;
}


status_t
PCI::GetNthInfo(long index, pci_info *outInfo)
{
	long currentIndex = 0;

	for (uint32 domain = 0; domain < fDomainCount; domain++) {
		if (_GetNthInfo(fDomainData[domain].bus, &currentIndex, index, outInfo) >= B_OK)
			return B_OK;
	}

	return B_ERROR;
}


status_t
PCI::_GetNthInfo(PCIBus *bus, long *currentIndex, long wantIndex,
	pci_info *outInfo)
{
	// maps tree structure to linear indexed view
	PCIDev *dev = bus->child;
	while (dev) {
		if (*currentIndex == wantIndex) {
			*outInfo = dev->info;
			return B_OK;
		}
		*currentIndex += 1;
		if (dev->child && B_OK == _GetNthInfo(dev->child, currentIndex,
				wantIndex, outInfo))
			return B_OK;
		dev = dev->next;
	}

	return B_ERROR;
}


void
PCI::_EnumerateBus(uint8 domain, uint8 bus, uint8 *subordinateBus)
{
	TRACE(("PCI: EnumerateBus: domain %u, bus %u\n", domain, bus));

	int maxBusDevices = _GetDomainData(domain)->max_bus_devices;

	// step 1: disable all bridges on this bus
	for (int dev = 0; dev < maxBusDevices; dev++) {
		uint16 vendor_id = ReadConfig(domain, bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;

		int numFunctions = _NumFunctions(domain, bus, dev);
		for (int function = 0; function < numFunctions; function++) {
			uint16 device_id = ReadConfig(domain, bus, dev, function,
				PCI_device_id, 2);
			if (device_id == 0xffff)
				continue;

			uint8 baseClass = ReadConfig(domain, bus, dev, function,
				PCI_class_base, 1);
			uint8 subClass = ReadConfig(domain, bus, dev, function,
				PCI_class_sub, 1);
			if (baseClass != PCI_bridge || subClass != PCI_pci)
				continue;

			// skip incorrectly configured devices
			uint8 headerType = ReadConfig(domain, bus, dev, function,
				PCI_header_type, 1) & PCI_header_type_mask;
			if (headerType != PCI_header_type_PCI_to_PCI_bridge)
				continue;

			TRACE(("PCI: found PCI-PCI bridge: domain %u, bus %u, dev %u, func %u\n",
				domain, bus, dev, function));
			TRACE(("PCI: original settings: pcicmd %04" B_PRIx32 ", primary-bus "
				"%" B_PRIu32 ", secondary-bus %" B_PRIu32 ", subordinate-bus "
				"%" B_PRIu32 "\n",
				ReadConfig(domain, bus, dev, function, PCI_command, 2),
				ReadConfig(domain, bus, dev, function, PCI_primary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_secondary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_subordinate_bus, 1)));

			// disable decoding
			uint16 pcicmd;
			pcicmd = ReadConfig(domain, bus, dev, function, PCI_command, 2);
			pcicmd &= ~(PCI_command_io | PCI_command_memory
				| PCI_command_master);
			WriteConfig(domain, bus, dev, function, PCI_command, 2, pcicmd);

			// disable busses
			WriteConfig(domain, bus, dev, function, PCI_primary_bus, 1, 0);
			WriteConfig(domain, bus, dev, function, PCI_secondary_bus, 1, 0);
			WriteConfig(domain, bus, dev, function, PCI_subordinate_bus, 1, 0);

			TRACE(("PCI: disabled settings: pcicmd %04" B_PRIx32 ", primary-bus "
				"%" B_PRIu32 ", secondary-bus %" B_PRIu32 ", subordinate-bus "
				"%" B_PRIu32 "\n",
				ReadConfig(domain, bus, dev, function, PCI_command, 2),
				ReadConfig(domain, bus, dev, function, PCI_primary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_secondary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_subordinate_bus, 1)));
		}
	}

	uint8 lastUsedBusNumber = bus;

	// step 2: assign busses to all bridges, and enable them again
	for (int dev = 0; dev < maxBusDevices; dev++) {
		uint16 vendor_id = ReadConfig(domain, bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;

		int numFunctions = _NumFunctions(domain, bus, dev);
		for (int function = 0; function < numFunctions; function++) {
			uint16 deviceID = ReadConfig(domain, bus, dev, function,
				PCI_device_id, 2);
			if (deviceID == 0xffff)
				continue;

			uint8 baseClass = ReadConfig(domain, bus, dev, function,
				PCI_class_base, 1);
			uint8 subClass = ReadConfig(domain, bus, dev, function,
				PCI_class_sub, 1);
			if (baseClass != PCI_bridge || subClass != PCI_pci)
				continue;

			// skip incorrectly configured devices
			uint8 headerType = ReadConfig(domain, bus, dev, function,
				PCI_header_type, 1) & PCI_header_type_mask;
			if (headerType != PCI_header_type_PCI_to_PCI_bridge)
				continue;

			TRACE(("PCI: configuring PCI-PCI bridge: domain %u, bus %u, dev %u, func %u\n",
				domain, bus, dev, function));

			// open Scheunentor for enumerating the bus behind the bridge
			WriteConfig(domain, bus, dev, function, PCI_primary_bus, 1, bus);
			WriteConfig(domain, bus, dev, function, PCI_secondary_bus, 1,
				lastUsedBusNumber + 1);
			WriteConfig(domain, bus, dev, function, PCI_subordinate_bus, 1, 255);

			// enable decoding (too early here?)
			uint16 pcicmd;
			pcicmd = ReadConfig(domain, bus, dev, function, PCI_command, 2);
			pcicmd |= PCI_command_io | PCI_command_memory | PCI_command_master;
			WriteConfig(domain, bus, dev, function, PCI_command, 2, pcicmd);

			TRACE(("PCI: probing settings: pcicmd %04" B_PRIx32 ", primary-bus "
				"%" B_PRIu32 ", secondary-bus %" B_PRIu32 ", subordinate-bus "
				"%" B_PRIu32 "\n",
				ReadConfig(domain, bus, dev, function, PCI_command, 2),
				ReadConfig(domain, bus, dev, function, PCI_primary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_secondary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_subordinate_bus, 1)));

			// enumerate bus
			_EnumerateBus(domain, lastUsedBusNumber + 1, &lastUsedBusNumber);

			// close Scheunentor
			WriteConfig(domain, bus, dev, function, PCI_subordinate_bus, 1, lastUsedBusNumber);

			TRACE(("PCI: configured settings: pcicmd %04" B_PRIx32 ", primary-bus "
				"%" B_PRIu32 ", secondary-bus %" B_PRIu32 ", subordinate-bus "
				"%" B_PRIu32 "\n",
				ReadConfig(domain, bus, dev, function, PCI_command, 2),
				ReadConfig(domain, bus, dev, function, PCI_primary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_secondary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_subordinate_bus, 1)));
			}
	}
	if (subordinateBus)
		*subordinateBus = lastUsedBusNumber;

	TRACE(("PCI: EnumerateBus done: domain %u, bus %u, last used bus number %u\n", domain, bus, lastUsedBusNumber));
}


void
PCI::_FixupDevices(uint8 domain, uint8 bus)
{
	FLOW("PCI: FixupDevices domain %u, bus %u\n", domain, bus);

	int maxBusDevices = _GetDomainData(domain)->max_bus_devices;
	static int recursed = 0;

	if (recursed++ > 10) {
		// guard against buggy chipsets
		// XXX: is there any official limit ?
		dprintf("PCI: FixupDevices: too many recursions (buggy chipset?)\n");
		recursed--;
		return;
	}

	for (int dev = 0; dev < maxBusDevices; dev++) {
		uint16 vendorId = ReadConfig(domain, bus, dev, 0, PCI_vendor_id, 2);
		if (vendorId == 0xffff)
			continue;

		int numFunctions = _NumFunctions(domain, bus, dev);
		for (int function = 0; function < numFunctions; function++) {
			uint16 deviceId = ReadConfig(domain, bus, dev, function,
				PCI_device_id, 2);
			if (deviceId == 0xffff)
				continue;

			pci_fixup_device(this, domain, bus, dev, function);

			uint8 baseClass = ReadConfig(domain, bus, dev, function,
				PCI_class_base, 1);
			if (baseClass != PCI_bridge)
				continue;
			uint8 subClass = ReadConfig(domain, bus, dev, function,
				PCI_class_sub, 1);
			if (subClass != PCI_pci)
				continue;

			// some FIC motherboards have a buggy BIOS...
			// make sure the header type is correct for a bridge,
			uint8 headerType = ReadConfig(domain, bus, dev, function,
				PCI_header_type, 1) & PCI_header_type_mask;
			if (headerType != PCI_header_type_PCI_to_PCI_bridge) {
				dprintf("PCI: dom %u, bus %u, dev %2u, func %u, PCI bridge"
					" class but wrong header type 0x%02x, ignoring.\n",
					domain, bus, dev, function, headerType);
				continue;
			}


			int busBehindBridge = ReadConfig(domain, bus, dev, function,
				PCI_secondary_bus, 1);

			TRACE(("PCI: FixupDevices: checking bus %d behind %04x:%04x\n",
				busBehindBridge, vendorId, deviceId));
			_FixupDevices(domain, busBehindBridge);
		}
	}
	recursed--;
}


void
PCI::_ConfigureBridges(PCIBus *bus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		if (dev->info.class_base == PCI_bridge
			&& dev->info.class_sub == PCI_pci
			&& (dev->info.header_type & PCI_header_type_mask)
			== PCI_header_type_PCI_to_PCI_bridge) {
			uint16 bridgeControlOld = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_bridge_control, 2);
			uint16 bridgeControlNew = bridgeControlOld;
			// Enable: Parity Error Response, SERR, Master Abort Mode, Discard
			// Timer SERR
			// Clear: Discard Timer Status
			bridgeControlNew |= PCI_bridge_parity_error_response
				| PCI_bridge_serr | PCI_bridge_master_abort
				| PCI_bridge_discard_timer_status
				| PCI_bridge_discard_timer_serr;
			// Set discard timer to 2^15 PCI clocks
			bridgeControlNew &= ~(PCI_bridge_primary_discard_timeout
				| PCI_bridge_secondary_discard_timeout);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_bridge_control, 2, bridgeControlNew);
			bridgeControlNew = ReadConfig(dev->domain, dev->bus, dev->device,
				dev->function, PCI_bridge_control, 2);
			dprintf("PCI: dom %u, bus %u, dev %2u, func %u, changed PCI bridge"
				" control from 0x%04x to 0x%04x\n", dev->domain, dev->bus,
				dev->device, dev->function, bridgeControlOld,
				bridgeControlNew);
		}

		if (dev->child)
			_ConfigureBridges(dev->child);
	}
}


void
PCI::ClearDeviceStatus(PCIBus *bus, bool dumpStatus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		// Clear and dump PCI device status
		uint16 status = ReadConfig(dev->domain, dev->bus, dev->device,
			dev->function, PCI_status, 2);
		WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
			PCI_status, 2, status);
		if (dumpStatus) {
			kprintf("domain %u, bus %u, dev %2u, func %u, PCI device status "
				"0x%04x\n", dev->domain, dev->bus, dev->device, dev->function,
				status);
			if (status & PCI_status_parity_error_detected)
				kprintf("  Detected Parity Error\n");
			if (status & PCI_status_serr_signalled)
				kprintf("  Signalled System Error\n");
			if (status & PCI_status_master_abort_received)
				kprintf("  Received Master-Abort\n");
			if (status & PCI_status_target_abort_received)
				kprintf("  Received Target-Abort\n");
			if (status & PCI_status_target_abort_signalled)
				kprintf("  Signalled Target-Abort\n");
			if (status & PCI_status_parity_signalled)
				kprintf("  Master Data Parity Error\n");
		}

		if (dev->info.class_base == PCI_bridge
			&& dev->info.class_sub == PCI_pci) {
			// Clear and dump PCI bridge secondary status
			uint16 secondaryStatus = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_secondary_status, 2);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_secondary_status, 2, secondaryStatus);
			if (dumpStatus) {
				kprintf("domain %u, bus %u, dev %2u, func %u, PCI bridge "
					"secondary status 0x%04x\n", dev->domain, dev->bus,
					dev->device, dev->function, secondaryStatus);
				if (secondaryStatus & PCI_status_parity_error_detected)
					kprintf("  Detected Parity Error\n");
				if (secondaryStatus & PCI_status_serr_signalled)
					kprintf("  Received System Error\n");
				if (secondaryStatus & PCI_status_master_abort_received)
					kprintf("  Received Master-Abort\n");
				if (secondaryStatus & PCI_status_target_abort_received)
					kprintf("  Received Target-Abort\n");
				if (secondaryStatus & PCI_status_target_abort_signalled)
					kprintf("  Signalled Target-Abort\n");
				if (secondaryStatus & PCI_status_parity_signalled)
					kprintf("  Data Parity Reported\n");
			}

			// Clear and dump the discard-timer error bit located in bridge-control register
			uint16 bridgeControl = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_bridge_control, 2);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_bridge_control, 2, bridgeControl);
			if (dumpStatus) {
				kprintf("domain %u, bus %u, dev %2u, func %u, PCI bridge "
					"control 0x%04x\n", dev->domain, dev->bus, dev->device,
					dev->function, bridgeControl);
				if (bridgeControl & PCI_bridge_discard_timer_status) {
					kprintf("  bridge-control: Discard Timer Error\n");
				}
			}
		}

		if (dev->child)
			ClearDeviceStatus(dev->child, dumpStatus);
	}
}


void
PCI::_DiscoverBus(PCIBus *bus)
{
	FLOW("PCI: DiscoverBus, domain %u, bus %u\n", bus->domain, bus->bus);

	int maxBusDevices = _GetDomainData(bus->domain)->max_bus_devices;
	static int recursed = 0;

	if (recursed++ > 10) {
		// guard against buggy chipsets
		// XXX: is there any official limit ?
		dprintf("PCI: DiscoverBus: too many recursions (buggy chipset?)\n");
		recursed--;
		return;
	}

	for (int dev = 0; dev < maxBusDevices; dev++) {
		uint16 vendorID = ReadConfig(bus->domain, bus->bus, dev, 0,
			PCI_vendor_id, 2);
		if (vendorID == 0xffff)
			continue;

		int numFunctions = _NumFunctions(bus->domain, bus->bus, dev);
		for (int function = 0; function < numFunctions; function++)
			_DiscoverDevice(bus, dev, function);
	}

	recursed--;
}


void
PCI::_DiscoverDevice(PCIBus *bus, uint8 dev, uint8 function)
{
	FLOW("PCI: DiscoverDevice, domain %u, bus %u, dev %u, func %u\n", bus->domain, bus->bus, dev, function);

	uint16 deviceID = ReadConfig(bus->domain, bus->bus, dev, function,
		PCI_device_id, 2);
	if (deviceID == 0xffff)
		return;

	PCIDev *newDev = _CreateDevice(bus, dev, function);

	uint8 baseClass = ReadConfig(bus->domain, bus->bus, dev, function,
		PCI_class_base, 1);
	uint8 subClass = ReadConfig(bus->domain, bus->bus, dev, function,
		PCI_class_sub, 1);
	uint8 headerType = ReadConfig(bus->domain, bus->bus, dev, function,
		PCI_header_type, 1) & PCI_header_type_mask;
	if (baseClass == PCI_bridge && subClass == PCI_pci
		&& headerType == PCI_header_type_PCI_to_PCI_bridge) {
		uint8 secondaryBus = ReadConfig(bus->domain, bus->bus, dev, function,
			PCI_secondary_bus, 1);
		PCIBus *newBus = _CreateBus(newDev, bus->domain, secondaryBus);
		_DiscoverBus(newBus);
	}
}


PCIBus *
PCI::_CreateBus(PCIDev *parent, uint8 domain, uint8 bus)
{
	PCIBus *newBus = new(std::nothrow) PCIBus;
	if (newBus == NULL)
		return NULL;

	newBus->parent = parent;
	newBus->child = NULL;
	newBus->domain = domain;
	newBus->bus = bus;

	// append
	parent->child = newBus;

	return newBus;
}


PCIDev *
PCI::_CreateDevice(PCIBus *parent, uint8 device, uint8 function)
{
	FLOW("PCI: CreateDevice, domain %u, bus %u, dev %u, func %u:\n", parent->domain,
		parent->bus, device, function);

	PCIDev *newDev = new(std::nothrow) PCIDev;
	if (newDev == NULL)
		return NULL;

	newDev->next = NULL;
	newDev->parent = parent;
	newDev->child = NULL;
	newDev->domain = parent->domain;
	newDev->bus = parent->bus;
	newDev->device = device;
	newDev->function = function;
	memset(&newDev->info, 0, sizeof(newDev->info));

	_ReadBasicInfo(newDev);

	FLOW("PCI: CreateDevice, vendor 0x%04x, device 0x%04x, class_base 0x%02x, "
		"class_sub 0x%02x\n", newDev->info.vendor_id, newDev->info.device_id,
		newDev->info.class_base, newDev->info.class_sub);

	// append
	if (parent->child == NULL) {
		parent->child = newDev;
	} else {
		PCIDev *sub = parent->child;
		while (sub->next)
			sub = sub->next;
		sub->next = newDev;
	}

	return newDev;
}


uint64
PCI::_BarSize(uint64 bits)
{
	if (!bits)
		return 0;

	uint64 size = 1;
	while ((bits & size) == 0)
		size <<= 1;

	return size;
}


size_t
PCI::_GetBarInfo(PCIDev *dev, uint8 offset, uint32 &_ramAddress,
	uint32 &_pciAddress, uint32 &_size, uint8 &flags, uint32 *_highRAMAddress,
	uint32 *_highPCIAddress, uint32 *_highSize)
{
	uint64 pciAddress = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, offset, 4);
	WriteConfig(dev->domain, dev->bus, dev->device, dev->function, offset, 4,
		0xffffffff);
	uint64 size = ReadConfig(dev->domain, dev->bus, dev->device, dev->function,
		offset, 4);
	WriteConfig(dev->domain, dev->bus, dev->device, dev->function, offset, 4,
		pciAddress);

	uint32 mask = PCI_address_memory_32_mask;
	bool is64bit = false;
	if ((pciAddress & PCI_address_space) != 0)
		mask = PCI_address_io_mask;
	else {
		is64bit = (pciAddress & PCI_address_type) == PCI_address_type_64;

		if (is64bit && _highRAMAddress != NULL) {
			uint64 highPCIAddress = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, offset + 4, 4);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				offset + 4, 4, 0xffffffff);
			uint64 highSize = ReadConfig(dev->domain, dev->bus, dev->device,
				dev->function, offset + 4, 4);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				offset + 4, 4, highPCIAddress);

			pciAddress |= highPCIAddress << 32;
			size |= highSize << 32;
		}
	}

	flags = (uint32)pciAddress & ~mask;
	pciAddress &= ((uint64)0xffffffff << 32) | mask;
	size &= ((uint64)0xffffffff << 32) | mask;

	size = _BarSize(size);
	uint64 ramAddress = pci_ram_address(pciAddress);

	_ramAddress = ramAddress;
	_pciAddress = pciAddress;
	_size = size;
	if (!is64bit)
		return 1;

	if (_highRAMAddress == NULL || _highPCIAddress == NULL || _highSize == NULL)
		panic("64 bit PCI BAR but no space to store high values\n");
	else {
		*_highRAMAddress = ramAddress >> 32;
		*_highPCIAddress = pciAddress >> 32;
		*_highSize = size >> 32;
	}

	return 2;
}


void
PCI::_GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 &_address, uint32 *_size,
	uint8 *_flags)
{
	uint32 oldValue = ReadConfig(dev->domain, dev->bus, dev->device, dev->function,
		offset, 4);
	WriteConfig(dev->domain, dev->bus, dev->device, dev->function, offset, 4,
		0xfffffffe); // LSB must be 0
	uint32 newValue = ReadConfig(dev->domain, dev->bus, dev->device, dev->function,
		offset, 4);
	WriteConfig(dev->domain, dev->bus, dev->device, dev->function, offset, 4,
		oldValue);

	_address = oldValue & PCI_rom_address_mask;
	if (_size != NULL)
		*_size = _BarSize(newValue & PCI_rom_address_mask);
	if (_flags != NULL)
		*_flags = newValue & 0xf;
}


void
PCI::_ReadBasicInfo(PCIDev *dev)
{
	uint8 virtualBus;

	if (_CreateVirtualBus(dev->domain, dev->bus, &virtualBus) != B_OK) {
		dprintf("PCI: CreateVirtualBus failed, domain %u, bus %u\n", dev->domain, dev->bus);
		return;
	}

	dev->info.vendor_id = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_vendor_id, 2);
	dev->info.device_id = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_device_id, 2);
	dev->info.bus = virtualBus;
	dev->info.device = dev->device;
	dev->info.function = dev->function;
	dev->info.revision = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_revision, 1);
	dev->info.class_api = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_class_api, 1);
	dev->info.class_sub = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_class_sub, 1);
	dev->info.class_base = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_class_base, 1);
	dev->info.line_size = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_line_size, 1);
	dev->info.latency = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_latency, 1);
	// BeOS does not mask off the multifunction bit, developer must use
	// (header_type & PCI_header_type_mask)
	dev->info.header_type = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_header_type, 1);
	dev->info.bist = ReadConfig(dev->domain, dev->bus, dev->device,
		dev->function, PCI_bist, 1);
	dev->info.reserved = 0;
}


void
PCI::_ReadHeaderInfo(PCIDev *dev)
{
	switch (dev->info.header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
		{
			// disable PCI device address decoding (io and memory) while BARs
			// are modified
			uint16 pcicmd = ReadConfig(dev->domain, dev->bus, dev->device,
				dev->function, PCI_command, 2);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_command, 2,
				pcicmd & ~(PCI_command_io | PCI_command_memory));

			// get BAR size infos
			_GetRomBarInfo(dev, PCI_rom_base, dev->info.u.h0.rom_base_pci,
				&dev->info.u.h0.rom_size);
			for (int i = 0; i < 6;) {
				i += _GetBarInfo(dev, PCI_base_registers + 4 * i,
					dev->info.u.h0.base_registers[i],
					dev->info.u.h0.base_registers_pci[i],
					dev->info.u.h0.base_register_sizes[i],
					dev->info.u.h0.base_register_flags[i],
					i < 5 ? &dev->info.u.h0.base_registers[i + 1] : NULL,
					i < 5 ? &dev->info.u.h0.base_registers_pci[i + 1] : NULL,
					i < 5 ? &dev->info.u.h0.base_register_sizes[i + 1] : NULL);
			}

			// restore PCI device address decoding
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_command, 2, pcicmd);

			dev->info.u.h0.rom_base = (uint32)pci_ram_address(
				dev->info.u.h0.rom_base_pci);

			dev->info.u.h0.cardbus_cis = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_cardbus_cis, 4);
			dev->info.u.h0.subsystem_id = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_subsystem_id, 2);
			dev->info.u.h0.subsystem_vendor_id = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_subsystem_vendor_id,
				2);
			dev->info.u.h0.interrupt_line = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_interrupt_line, 1);
			dev->info.u.h0.interrupt_pin = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_interrupt_pin, 1);
			dev->info.u.h0.min_grant = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_min_grant, 1);
			dev->info.u.h0.max_latency = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_max_latency, 1);
			break;
		}

		case PCI_header_type_PCI_to_PCI_bridge:
		{
			// disable PCI device address decoding (io and memory) while BARs
			// are modified
			uint16 pcicmd = ReadConfig(dev->domain, dev->bus, dev->device,
				dev->function, PCI_command, 2);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_command, 2,
				pcicmd & ~(PCI_command_io | PCI_command_memory));

			_GetRomBarInfo(dev, PCI_bridge_rom_base,
				dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2;) {
				i += _GetBarInfo(dev, PCI_base_registers + 4 * i,
					dev->info.u.h1.base_registers[i],
					dev->info.u.h1.base_registers_pci[i],
					dev->info.u.h1.base_register_sizes[i],
					dev->info.u.h1.base_register_flags[i],
					i < 1 ? &dev->info.u.h1.base_registers[i + 1] : NULL,
					i < 1 ? &dev->info.u.h1.base_registers_pci[i + 1] : NULL,
					i < 1 ? &dev->info.u.h1.base_register_sizes[i + 1] : NULL);
			}

			// restore PCI device address decoding
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_command, 2, pcicmd);

			dev->info.u.h1.rom_base = (uint32)pci_ram_address(
				dev->info.u.h1.rom_base_pci);

			dev->info.u.h1.primary_bus = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_primary_bus, 1);
			dev->info.u.h1.secondary_bus = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_secondary_bus, 1);
			dev->info.u.h1.subordinate_bus = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_subordinate_bus, 1);
			dev->info.u.h1.secondary_latency = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_secondary_latency, 1);
			dev->info.u.h1.io_base = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_io_base, 1);
			dev->info.u.h1.io_limit = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_io_limit, 1);
			dev->info.u.h1.secondary_status = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_secondary_status, 2);
			dev->info.u.h1.memory_base = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_memory_base, 2);
			dev->info.u.h1.memory_limit = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function,
				PCI_prefetchable_memory_base, 2);
			dev->info.u.h1.prefetchable_memory_limit = ReadConfig(
				dev->domain, dev->bus, dev->device, dev->function,
				PCI_prefetchable_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base_upper32 = ReadConfig(
				dev->domain, dev->bus, dev->device, dev->function,
				PCI_prefetchable_memory_base_upper32, 4);
			dev->info.u.h1.prefetchable_memory_limit_upper32 = ReadConfig(
				dev->domain, dev->bus, dev->device, dev->function,
				PCI_prefetchable_memory_limit_upper32, 4);
			dev->info.u.h1.io_base_upper16 = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_io_base_upper16, 2);
			dev->info.u.h1.io_limit_upper16 = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_io_limit_upper16, 2);
			dev->info.u.h1.interrupt_line = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_interrupt_line, 1);
			dev->info.u.h1.interrupt_pin = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_interrupt_pin, 1);
			dev->info.u.h1.bridge_control = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_bridge_control, 2);
			dev->info.u.h1.subsystem_id = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_sub_device_id_1, 2);
			dev->info.u.h1.subsystem_vendor_id = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_sub_vendor_id_1, 2);
			break;
		}

		case PCI_header_type_cardbus:
		{
			// for testing only, not final:
			dev->info.u.h2.subsystem_id = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_sub_device_id_2, 2);
			dev->info.u.h2.subsystem_vendor_id = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_sub_vendor_id_2, 2);
			dev->info.u.h2.primary_bus = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_primary_bus_2, 1);
			dev->info.u.h2.secondary_bus = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_secondary_bus_2, 1);
			dev->info.u.h2.subordinate_bus = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_subordinate_bus_2, 1);
			dev->info.u.h2.secondary_latency = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_secondary_latency_2, 1);
			dev->info.u.h2.reserved = 0;
			dev->info.u.h2.memory_base = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_memory_base0_2, 4);
			dev->info.u.h2.memory_limit = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_memory_limit0_2, 4);
			dev->info.u.h2.memory_base_upper32 = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_memory_base1_2, 4);
			dev->info.u.h2.memory_limit_upper32 = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_memory_limit1_2, 4);
			dev->info.u.h2.io_base = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_io_base0_2, 4);
			dev->info.u.h2.io_limit = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_io_limit0_2, 4);
			dev->info.u.h2.io_base_upper32 = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_io_base1_2, 4);
			dev->info.u.h2.io_limit_upper32 = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_io_limit1_2, 4);
			dev->info.u.h2.secondary_status = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_secondary_status_2, 2);
			dev->info.u.h2.bridge_control = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_bridge_control_2, 2);
			break;
		}

		default:
			TRACE(("PCI: Header type unknown (0x%02x)\n", dev->info.header_type));
			break;
	}
}


void
PCI::RefreshDeviceInfo()
{
	for (uint32 domain = 0; domain < fDomainCount; domain++)
		_RefreshDeviceInfo(fDomainData[domain].bus);
}


void
PCI::_RefreshDeviceInfo(PCIBus *bus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		_ReadBasicInfo(dev);
		_ReadHeaderInfo(dev);
		_ReadMSIInfo(dev);
		_ReadMSIXInfo(dev);
		_ReadHtMappingInfo(dev);
		if (dev->child)
			_RefreshDeviceInfo(dev->child);
	}
}


status_t
PCI::ReadConfig(uint8 domain, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 *value)
{
	domain_data *info = _GetDomainData(domain);
	if (!info)
		return B_ERROR;

	if (device > (info->max_bus_devices - 1)
		|| function > 7
		|| (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3)
		|| (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: can't read config for domain %d, %u:%u:%u, offset %u, size %u\n",
			 domain, bus, device, function, offset, size);
		return B_ERROR;
	}

	status_t status = (*info->controller->read_pci_config)(info->controller_cookie,
		bus, device, function, offset, size, value);
	if (status != B_OK) {
		dprintf("PCI: failed to read config for domain %d, %u:%u:%u, offset %u, size %u\n",
			domain, bus, device, function, offset, size);
	}
	return status;
}


uint32
PCI::ReadConfig(uint8 domain, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size)
{
	uint32 value;
	if (ReadConfig(domain, bus, device, function, offset, size, &value)
			!= B_OK)
		return 0xffffffff;

	return value;
}


uint32
PCI::ReadConfig(PCIDev *device, uint16 offset, uint8 size)
{
	uint32 value;
	if (ReadConfig(device->domain, device->bus, device->device,
			device->function, offset, size, &value) != B_OK)
		return 0xffffffff;

	return value;
}


status_t
PCI::WriteConfig(uint8 domain, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	domain_data *info = _GetDomainData(domain);
	if (!info)
		return B_ERROR;

	if (device > (info->max_bus_devices - 1)
		|| function > 7
		|| (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3)
		|| (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: can't write config for domain %d, bus %u, device %u, function %u, offset %u, size %u\n",
			 domain, bus, device, function, offset, size);
		return B_ERROR;
	}

	return (*info->controller->write_pci_config)(info->controller_cookie, bus,
		device, function, offset, size, value);
}


status_t
PCI::WriteConfig(PCIDev *device, uint16 offset, uint8 size, uint32 value)
{
	return WriteConfig(device->domain, device->bus, device->device,
		device->function, offset, size, value);
}


status_t
PCI::FindCapability(uint8 domain, uint8 bus, uint8 device, uint8 function,
	uint8 capID, uint8 *offset)
{
	uint16 status = ReadConfig(domain, bus, device, function, PCI_status, 2);
	if (!(status & PCI_status_capabilities)) {
		FLOW("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x "
			"not supported\n", bus, device, function, capID);
		return B_ERROR;
	}

	uint8 headerType = ReadConfig(domain, bus, device, function,
		PCI_header_type, 1);
	uint8 capPointer;

	switch (headerType & PCI_header_type_mask) {
		case PCI_header_type_generic:
		case PCI_header_type_PCI_to_PCI_bridge:
			capPointer = PCI_capabilities_ptr;
			break;
		case PCI_header_type_cardbus:
			capPointer = PCI_capabilities_ptr_2;
			break;
		default:
			TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability "
				"%#02x unknown header type\n", bus, device, function, capID);
			return B_ERROR;
	}

	capPointer = ReadConfig(domain, bus, device, function, capPointer, 1);
	capPointer &= ~3;
	if (capPointer == 0) {
		TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x "
			"empty list\n", bus, device, function, capID);
		return B_NAME_NOT_FOUND;
	}

	for (int i = 0; i < 48; i++) {
		if (ReadConfig(domain, bus, device, function, capPointer, 1) == capID) {
			if (offset != NULL)
				*offset = capPointer;
			return B_OK;
		}

		capPointer = ReadConfig(domain, bus, device, function, capPointer + 1,
			1);
		capPointer &= ~3;

		if (capPointer == 0)
			return B_NAME_NOT_FOUND;
	}

	TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x circular list\n", bus, device, function, capID);
	return B_ERROR;
}


status_t
PCI::FindCapability(PCIDev *device, uint8 capID, uint8 *offset)
{
	return FindCapability(device->domain, device->bus, device->device,
		device->function, capID, offset);
}


status_t
PCI::FindExtendedCapability(uint8 domain, uint8 bus, uint8 device,
	uint8 function, uint16 capID, uint16 *offset)
{
	if (FindCapability(domain, bus, device, function, PCI_cap_id_pcie)
		!= B_OK) {
		FLOW("PCI:FindExtendedCapability ERROR %u:%u:%u capability %#02x "
			"not supported\n", bus, device, function, capID);
		return B_ERROR;
	}
	uint16 capPointer = PCI_extended_capability;
	uint32 capability = ReadConfig(domain, bus, device, function,
		capPointer, 4);

	if (capability == 0 || capability == 0xffffffff)
			return B_NAME_NOT_FOUND;

	for (int i = 0; i < 48; i++) {
		if (PCI_extcap_id(capability) == capID) {
			if (offset != NULL)
				*offset = capPointer;
			return B_OK;
		}

		capPointer = PCI_extcap_next_ptr(capability) & ~3;
		if (capPointer < PCI_extended_capability)
			return B_NAME_NOT_FOUND;
		capability = ReadConfig(domain, bus, device, function,
			capPointer, 4);
	}

	TRACE_CAP("PCI:FindExtendedCapability ERROR %u:%u:%u capability %#04x "
		"circular list\n", bus, device, function, capID);
	return B_ERROR;
}


status_t
PCI::FindExtendedCapability(PCIDev *device, uint16 capID, uint16 *offset)
{
	return FindExtendedCapability(device->domain, device->bus, device->device,
		device->function, capID, offset);
}


status_t
PCI::FindHTCapability(uint8 domain, uint8 bus, uint8 device,
	uint8 function, uint16 capID, uint8 *offset)
{
	uint8 capPointer;
	// consider the passed offset as the current ht capability block pointer
	// when it's non zero
	if (offset != NULL && *offset != 0) {
		capPointer = ReadConfig(domain, bus, device, function, *offset + 1,
			1);
	} else if (FindCapability(domain, bus, device, function, PCI_cap_id_ht,
		&capPointer) != B_OK) {
		FLOW("PCI:FindHTCapability ERROR %u:%u:%u capability %#02x "
			"not supported\n", bus, device, function, capID);
		return B_NAME_NOT_FOUND;
	}

	uint16 mask = PCI_ht_command_cap_mask_5_bits;
	if (capID == PCI_ht_command_cap_slave || capID == PCI_ht_command_cap_host)
		mask = PCI_ht_command_cap_mask_3_bits;
	for (int i = 0; i < 48; i++) {
		capPointer &= ~3;
		if (capPointer == 0)
			return B_NAME_NOT_FOUND;

		uint8 capability = ReadConfig(domain, bus, device, function,
			capPointer, 1);
		if (capability == PCI_cap_id_ht) {
			if ((ReadConfig(domain, bus, device, function,
					capPointer + PCI_ht_command, 2) & mask) == capID) {
				if (offset != NULL)
					*offset = capPointer;
				return B_OK;
			}
		}

		capPointer = ReadConfig(domain, bus, device, function, capPointer + 1,
			1);
	}

	TRACE_CAP("PCI:FindHTCapability ERROR %u:%u:%u capability %#04x "
		"circular list\n", bus, device, function, capID);
	return B_ERROR;
}


status_t
PCI::FindHTCapability(PCIDev *device, uint16 capID, uint8 *offset)
{
	return FindHTCapability(device->domain, device->bus, device->device,
		device->function, capID, offset);
}


PCIDev *
PCI::FindDevice(uint8 domain, uint8 bus, uint8 device, uint8 function)
{
	if (domain >= fDomainCount)
		return NULL;

	return _FindDevice(fDomainData[domain].bus, domain, bus, device, function);
}


PCIDev *
PCI::_FindDevice(PCIBus *current, uint8 domain, uint8 bus, uint8 device,
	uint8 function)
{
	if (current->domain == domain) {
		// search device on this bus

		for (PCIDev *child = current->child; child != NULL;
				child = child->next) {
			if (child->bus == bus && child->device == device
				&& child->function == function)
				return child;

			if (child->child != NULL) {
				// search child busses
				PCIDev *found = _FindDevice(child->child, domain, bus, device,
					function);
				if (found != NULL)
					return found;
			}
		}
	}

	return NULL;
}


status_t
PCI::UpdateInterruptLine(uint8 domain, uint8 bus, uint8 _device, uint8 function,
	uint8 newInterruptLineValue)
{
	PCIDev *device = FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	pci_info &info = device->info;
	switch (info.header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
			info.u.h0.interrupt_line = newInterruptLineValue;
			break;

		case PCI_header_type_PCI_to_PCI_bridge:
			info.u.h1.interrupt_line = newInterruptLineValue;
			break;

		default:
			return B_ERROR;
	}

	return WriteConfig(device, PCI_interrupt_line, 1, newInterruptLineValue);
}


uint8
PCI::GetPowerstate(PCIDev *device)
{
	uint8 capabilityOffset;
	status_t res = FindCapability(device, PCI_cap_id_pm, &capabilityOffset);
	if (res == B_OK) {
		uint32 state = ReadConfig(device, capabilityOffset + PCI_pm_status, 2);
		return (state & PCI_pm_mask);
	}
	return PCI_pm_state_d0;
}


void
PCI::SetPowerstate(PCIDev *device, uint8 newState)
{
	uint8 capabilityOffset;
	status_t res = FindCapability(device, PCI_cap_id_pm, &capabilityOffset);
	if (res == B_OK) {
		uint32 state = ReadConfig(device, capabilityOffset + PCI_pm_status, 2);
		if ((state & PCI_pm_mask) != newState) {
			WriteConfig(device, capabilityOffset + PCI_pm_status, 2,
				(state & ~PCI_pm_mask) | newState);
			if ((state & PCI_pm_mask) == PCI_pm_state_d3)
				snooze(10);
		}
	}
}


status_t
PCI::GetPowerstate(uint8 domain, uint8 bus, uint8 _device, uint8 function,
	uint8* state)
{
	PCIDev *device = FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	*state = GetPowerstate(device);
	return B_OK;
}


status_t
PCI::SetPowerstate(uint8 domain, uint8 bus, uint8 _device, uint8 function,
	uint8 newState)
{
	PCIDev *device = FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	SetPowerstate(device, newState);
	return B_OK;
}


//#pragma mark - MSI

uint32
PCI::GetMSICount(PCIDev *device)
{
	if (!msi_supported())
		return 0;

	msi_info *info = &device->msi;
	if (!info->msi_capable)
		return 0;

	return info->message_count;
}


status_t
PCI::ConfigureMSI(PCIDev *device, uint32 count, uint32 *startVector)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	if (count == 0 || startVector == NULL)
		return B_BAD_VALUE;

	msi_info *info = &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (count > 32 || count > info->message_count
		|| ((count - 1) & count) != 0 /* needs to be a power of 2 */) {
		return B_BAD_VALUE;
	}

	if (info->configured_count != 0)
		return B_BUSY;

	status_t result = msi_allocate_vectors(count, &info->start_vector,
		&info->address_value, &info->data_value);
	if (result != B_OK)
		return result;

	uint8 offset = info->capability_offset;
	WriteConfig(device, offset + PCI_msi_address, 4,
		info->address_value & 0xffffffff);
	if (info->control_value & PCI_msi_control_64bit) {
		WriteConfig(device, offset + PCI_msi_address_high, 4,
			info->address_value >> 32);
		WriteConfig(device, offset + PCI_msi_data_64bit, 2,
			info->data_value);
	} else
		WriteConfig(device, offset + PCI_msi_data, 2, info->data_value);

	info->control_value &= ~PCI_msi_control_mme_mask;
	info->control_value |= (ffs(count) - 1) << 4;
	WriteConfig(device, offset + PCI_msi_control, 2, info->control_value);

	info->configured_count = count;
	*startVector = info->start_vector;
	return B_OK;
}


status_t
PCI::UnconfigureMSI(PCIDev *device)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	// try MSI-X
	status_t result = _UnconfigureMSIX(device);
	if (result != B_UNSUPPORTED && result != B_NO_INIT)
		return result;

	msi_info *info =  &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	msi_free_vectors(info->configured_count, info->start_vector);

	info->control_value &= ~PCI_msi_control_mme_mask;
	WriteConfig(device, info->capability_offset + PCI_msi_control, 2,
		info->control_value);

	info->configured_count = 0;
	info->address_value = 0;
	info->data_value = 0;
	return B_OK;
}


status_t
PCI::EnableMSI(PCIDev *device)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	msi_info *info =  &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// ensure the pinned interrupt is disabled
	WriteConfig(device, PCI_command, 2,
		ReadConfig(device, PCI_command, 2) | PCI_command_int_disable);

	// enable msi generation
	info->control_value |= PCI_msi_control_enable;
	WriteConfig(device, info->capability_offset + PCI_msi_control, 2,
		info->control_value);

	// enable HT msi mapping (if applicable)
	_HtMSIMap(device, info->address_value);

	dprintf("msi enabled: 0x%04" B_PRIx32 "\n",
		ReadConfig(device, info->capability_offset + PCI_msi_control, 2));
	return B_OK;
}


status_t
PCI::DisableMSI(PCIDev *device)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	// try MSI-X
	status_t result = _DisableMSIX(device);
	if (result != B_UNSUPPORTED && result != B_NO_INIT)
		return result;

	msi_info *info =  &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// disable HT msi mapping (if applicable)
	_HtMSIMap(device, 0);

	// disable msi generation
	info->control_value &= ~PCI_msi_control_enable;
	WriteConfig(device, info->capability_offset + PCI_msi_control, 2,
		info->control_value);

	return B_OK;
}


uint32
PCI::GetMSIXCount(PCIDev *device)
{
	if (!msi_supported())
		return 0;

	msix_info *info = &device->msix;
	if (!info->msix_capable)
		return 0;

	return info->message_count;
}


status_t
PCI::ConfigureMSIX(PCIDev *device, uint32 count, uint32 *startVector)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	if (count == 0 || startVector == NULL)
		return B_BAD_VALUE;

	msix_info *info = &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (count > 32 || count > info->message_count) {
		return B_BAD_VALUE;
	}

	if (info->configured_count != 0)
		return B_BUSY;

	// map the table bar
	size_t tableSize = info->message_count * 16;
	addr_t address;
	phys_addr_t barAddr = device->info.u.h0.base_registers[info->table_bar];
	uchar flags = device->info.u.h0.base_register_flags[info->table_bar];
	if ((flags & PCI_address_type) == PCI_address_type_64) {
		barAddr |= (uint64)device->info.u.h0.base_registers[
			info->table_bar + 1] << 32;
	}
	area_id area = map_physical_memory("msi table map",
		barAddr, tableSize + info->table_offset,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&address);
	if (area < 0)
		return area;
	info->table_area_id = area;
	info->table_address = address + info->table_offset;

	// and the pba bar if necessary
	if (info->table_bar != info->pba_bar) {
		barAddr = device->info.u.h0.base_registers[info->pba_bar];
		flags = device->info.u.h0.base_register_flags[info->pba_bar];
		if ((flags & PCI_address_type) == PCI_address_type_64) {
			barAddr |= (uint64)device->info.u.h0.base_registers[
				info->pba_bar + 1] << 32;
		}
		area = map_physical_memory("msi pba map",
			barAddr, tableSize + info->pba_offset,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(void**)&address);
		if (area < 0) {
			delete_area(info->table_area_id);
			info->table_area_id = -1;
			return area;
		}
		info->pba_area_id = area;
	} else
		info->pba_area_id = -1;
	info->pba_address = address + info->pba_offset;

	status_t result = msi_allocate_vectors(count, &info->start_vector,
		&info->address_value, &info->data_value);
	if (result != B_OK) {
		delete_area(info->pba_area_id);
		delete_area(info->table_area_id);
		info->pba_area_id = -1;
		info->table_area_id = -1;
		return result;
	}

	// ensure the memory i/o is enabled
	WriteConfig(device, PCI_command, 2,
		ReadConfig(device, PCI_command, 2) | PCI_command_memory);

	uint32 data_value = info->data_value;
	for (uint32 index = 0; index < count; index++) {
		volatile uint32 *entry = (uint32*)(info->table_address + 16 * index);
		*(entry + 3) |= PCI_msix_vctrl_mask;
		*entry++ = info->address_value & 0xffffffff;
		*entry++ = info->address_value >> 32;
		*entry++ = data_value++;
		*entry &= ~PCI_msix_vctrl_mask;
	}

	info->configured_count = count;
	*startVector = info->start_vector;
	dprintf("msix configured for %" B_PRIu32 " vectors\n", count);
	return B_OK;
}


status_t
PCI::EnableMSIX(PCIDev *device)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	msix_info *info = &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// ensure the pinned interrupt is disabled
	WriteConfig(device, PCI_command, 2,
		ReadConfig(device, PCI_command, 2) | PCI_command_int_disable);

	// enable msi-x generation
	info->control_value |= PCI_msix_control_enable;
	WriteConfig(device, info->capability_offset + PCI_msix_control, 2,
		info->control_value);

	// enable HT msi mapping (if applicable)
	_HtMSIMap(device, info->address_value);

	dprintf("msi-x enabled: 0x%04" B_PRIx32 "\n",
		ReadConfig(device, info->capability_offset + PCI_msix_control, 2));
	return B_OK;
}


void
PCI::_HtMSIMap(PCIDev *device, uint64 address)
{
	ht_mapping_info *info = &device->ht_mapping;
	if (!info->ht_mapping_capable)
		return;

	bool enabled = (info->control_value & PCI_ht_command_msi_enable) != 0;
	if ((address != 0) != enabled) {
		if (enabled) {
			info->control_value &= ~PCI_ht_command_msi_enable;
		} else {
			if ((address >> 20) != (info->address_value >> 20))
				return;
			dprintf("ht msi mapping enabled\n");
			info->control_value |= PCI_ht_command_msi_enable;
		}
		WriteConfig(device, info->capability_offset + PCI_ht_command, 2,
			info->control_value);
	}
}


void
PCI::_ReadMSIInfo(PCIDev *device)
{
	if (!msi_supported())
		return;

	msi_info *info = &device->msi;
	info->msi_capable = false;
	status_t result = FindCapability(device->domain, device->bus,
		device->device, device->function, PCI_cap_id_msi,
		&info->capability_offset);
	if (result != B_OK)
		return;

	info->msi_capable = true;
	info->control_value = ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msi_control, 2);
	info->message_count
		= 1 << ((info->control_value & PCI_msi_control_mmc_mask) >> 1);
	info->configured_count = 0;
	info->data_value = 0;
	info->address_value = 0;
}


void
PCI::_ReadMSIXInfo(PCIDev *device)
{
	if (!msi_supported())
		return;

	msix_info *info = &device->msix;
	info->msix_capable = false;
	status_t result = FindCapability(device->domain, device->bus,
		device->device, device->function, PCI_cap_id_msix,
		&info->capability_offset);
	if (result != B_OK)
		return;

	info->msix_capable = true;
	info->control_value = ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msix_control, 2);
	info->message_count
		= (info->control_value & PCI_msix_control_table_size) + 1;
	info->configured_count = 0;
	info->data_value = 0;
	info->address_value = 0;
	info->table_area_id = -1;
	info->pba_area_id = -1;
	uint32 table_value = ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msix_table, 4);
	uint32 pba_value = ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msix_pba, 4);

	info->table_bar = table_value & PCI_msix_bir_mask;
	info->table_offset = table_value & PCI_msix_offset_mask;
	info->pba_bar = pba_value & PCI_msix_bir_mask;
	info->pba_offset = pba_value & PCI_msix_offset_mask;
}


void
PCI::_ReadHtMappingInfo(PCIDev *device)
{
	if (!msi_supported())
		return;

	ht_mapping_info *info = &device->ht_mapping;
	info->ht_mapping_capable = false;

	uint8 offset = 0;
	if (FindHTCapability(device, PCI_ht_command_cap_msi_mapping,
		&offset) == B_OK) {
		info->control_value = ReadConfig(device, offset + PCI_ht_command,
			2);
		info->capability_offset = offset;
		info->ht_mapping_capable = true;
		if ((info->control_value & PCI_ht_command_msi_fixed) != 0) {
#if defined(__i386__) || defined(__x86_64__)
			info->address_value = MSI_ADDRESS_BASE;
#else
			// TODO: investigate what should be set here for non-x86
			dprintf("PCI_ht_command_msi_fixed flag unimplemented\n");
			info->address_value = 0;
#endif
		} else {
			info->address_value = ReadConfig(device, offset
				+ PCI_ht_msi_address_high, 4);
			info->address_value <<= 32;
			info->address_value |= ReadConfig(device, offset
				+ PCI_ht_msi_address_low, 4);
		}
		dprintf("found an ht msi mapping at %#" B_PRIx64 "\n",
			info->address_value);
	}
}


status_t
PCI::_UnconfigureMSIX(PCIDev *device)
{
	msix_info *info =  &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// disable msi-x generation
	info->control_value &= ~PCI_msix_control_enable;
	WriteConfig(device, info->capability_offset + PCI_msix_control, 2,
		info->control_value);

	msi_free_vectors(info->configured_count, info->start_vector);
	for (uint8 index = 0; index < info->configured_count; index++) {
		volatile uint32 *entry = (uint32*)(info->table_address + 16 * index);
		if ((*(entry + 3) & PCI_msix_vctrl_mask) == 0)
			*(entry + 3) |= PCI_msix_vctrl_mask;
	}

	if (info->pba_area_id != -1)
		delete_area(info->pba_area_id);
	if (info->table_area_id != -1)
		delete_area(info->table_area_id);
	info->pba_area_id= -1;
	info->table_area_id = -1;

	info->configured_count = 0;
	info->address_value = 0;
	info->data_value = 0;
	return B_OK;
}


status_t
PCI::_DisableMSIX(PCIDev *device)
{
	msix_info *info =  &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// disable HT msi mapping (if applicable)
	_HtMSIMap(device, 0);

	// disable msi-x generation
	info->control_value &= ~PCI_msix_control_enable;
	gPCI->WriteConfig(device, info->capability_offset + PCI_msix_control, 2,
		info->control_value);

	return B_OK;
}
