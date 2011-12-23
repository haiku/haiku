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


status_t
pci_controller_add(pci_controller *controller, void *cookie)
{
	return gPCI->AddController(controller, cookie);
}


long
pci_get_nth_pci_info(long index, pci_info *outInfo)
{
	return gPCI->GetNthInfo(index, outInfo);
}


uint32
pci_read_config(uint8 virtualBus, uint8 device, uint8 function, uint8 offset,
	uint8 size)
{
	uint8 bus;
	int domain;
	uint32 value;

	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return 0xffffffff;

	if (gPCI->ReadConfig(domain, bus, device, function, offset, size,
			&value) != B_OK)
		return 0xffffffff;

	return value;
}


void
pci_write_config(uint8 virtualBus, uint8 device, uint8 function, uint8 offset,
	uint8 size, uint32 value)
{
	uint8 bus;
	int domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return;

	gPCI->WriteConfig(domain, bus, device, function, offset, size, value);
}


status_t
pci_find_capability(uchar virtualBus, uchar device, uchar function,
	uchar capID, uchar *offset)
{
	uint8 bus;
	int domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	return gPCI->FindCapability(domain, bus, device, function, capID, offset);
}


status_t
pci_reserve_device(uchar virtualBus, uchar device, uchar function,
	const char *driverName, void *nodeCookie)
{
	status_t status;
	uint8 bus;
	int domain;
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

	device_attr matchPCIRoot[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "PCI"}},
		{NULL}
	};
	device_attr matchThis[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "pci"}},

		// location on PCI bus
		{B_PCI_DEVICE_DOMAIN, B_UINT32_TYPE, {ui32: domain}},
		{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {ui8: bus}},
		{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {ui8: device}},
		{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {ui8: function}},
		{NULL}
	};
	device_attr legacyAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "legacy_driver"}},
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Legacy Driver Reservation"}},
		{NULL}
	};
	device_attr drvAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "legacy_driver"}},
		{"legacy_driver", B_STRING_TYPE, {string: driverName}},
		{"legacy_driver_cookie", B_UINT64_TYPE, {ui64: (uint64)nodeCookie}},
		{NULL}
	};
	device_node *root, *pci, *node, *legacy;

	status = B_DEVICE_NOT_FOUND;
	root = gDeviceManager->get_root_node();
	if (!root)
		return status;

	pci = NULL;
	if (gDeviceManager->get_next_child_node(root, matchPCIRoot, &pci) < B_OK)
		goto err0;

	node = NULL;
	if (gDeviceManager->get_next_child_node(pci, matchThis, &node) < B_OK)
		goto err1;

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
	gDeviceManager->put_node(pci);
	gDeviceManager->put_node(root);

	return B_OK;

err3:
	gDeviceManager->unregister_node(legacy);
err2:
	gDeviceManager->put_node(node);
err1:
	gDeviceManager->put_node(pci);
err0:
	gDeviceManager->put_node(root);
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
	int domain;
	TRACE(("pci_unreserve_device(%d, %d, %d, %s)\n", virtualBus, device,
		function, driverName));

	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	//TRACE(("%s(%d [%d:%d], %d, %d, %s, %p)\n", __FUNCTION__, virtualBus,
	//	domain, bus, device, function, driverName, nodeCookie));

	device_attr matchPCIRoot[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "PCI"}},
		{NULL}
	};
	device_attr matchThis[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "pci"}},

		// location on PCI bus
		{B_PCI_DEVICE_DOMAIN, B_UINT32_TYPE, {ui32: domain}},
		{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {ui8: bus}},
		{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {ui8: device}},
		{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {ui8: function}},
		{NULL}
	};
	device_attr legacyAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "legacy_driver"}},
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Legacy Driver Reservation"}},
		{NULL}
	};
	device_attr drvAttrs[] = {
		// info about device
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "legacy_driver"}},
		{"legacy_driver", B_STRING_TYPE, {string: driverName}},
		{"legacy_driver_cookie", B_UINT64_TYPE, {ui64: (uint64)nodeCookie}},
		{NULL}
	};
	device_node *root, *pci, *node, *legacy, *drv;

	status = B_DEVICE_NOT_FOUND;
	root = gDeviceManager->get_root_node();
	if (!root)
		return status;

	pci = NULL;
	if (gDeviceManager->get_next_child_node(root, matchPCIRoot, &pci) < B_OK)
		goto err0;

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
	gDeviceManager->put_node(pci);
	gDeviceManager->put_node(root);
	return B_OK;

err3:
	gDeviceManager->put_node(legacy);
err2:
	gDeviceManager->put_node(node);
err1:
	gDeviceManager->put_node(pci);
err0:
	gDeviceManager->put_node(root);
	TRACE(("pci_unreserve_device for driver %s failed: %s\n", driverName,
		strerror(status)));
	return status;
}


status_t
pci_update_interrupt_line(uchar virtualBus, uchar device, uchar function,
	uchar newInterruptLineValue)
{
	uint8 bus;
	int domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return B_ERROR;

	return gPCI->UpdateInterruptLine(domain, bus, device, function,
		newInterruptLineValue);
}


// used by pci_info.cpp print_info_basic()
void
__pci_resolve_virtual_bus(uint8 virtualBus, int *domain, uint8 *bus)
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

			kprintf("[0x%lx]  ", address + i * itemSize);

			if (num > displayWidth) {
				// make sure the spacing in the last line is correct
				for (j = displayed; j < displayWidth * itemSize; j++)
					kprintf(" ");
			}
			kprintf("  ");
		}

		switch (itemSize) {
			case 1:
				kprintf(" %02x", pci_read_io_8(address + i * itemSize));
				break;
			case 2:
				kprintf(" %04x", pci_read_io_16(address + i * itemSize));
				break;
			case 4:
				kprintf(" %08lx", pci_read_io_32(address + i * itemSize));
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
	gPCI->ClearDeviceStatus(NULL, true);
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

	if (pci_io_init() != B_OK) {
		TRACE(("PCI: pci_io_init failed\n"));
		return B_ERROR;
	}

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

	if (pci_controller_init() != B_OK) {
		TRACE(("PCI: pci_controller_init failed\n"));
		panic("PCI: pci_controller_init failed\n");
		return B_ERROR;
	}

	gPCI->InitDomainData();
	gPCI->InitBus();

	add_debugger_command("pcistatus", &pcistatus, "dump and clear pci device status registers");
	add_debugger_command("pcirefresh", &pcirefresh, "refresh and print all pci_info");

	return B_OK;
}


void
pci_uninit(void)
{
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

	delete gPCI;
}


// #pragma mark PCI class


PCI::PCI()
	:
	fRootBus(0),
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
PCI::InitBus()
{
	PCIBus **nextBus = &fRootBus;
	for (int i = 0; i < fDomainCount; i++) {
		PCIBus *bus = new PCIBus;
		bus->next = NULL;
		bus->parent = NULL;
		bus->child = NULL;
		bus->domain = i;
		bus->bus = 0;
		*nextBus = bus;
		nextBus = &bus->next;
	}

	if (fBusEnumeration) {
		for (int i = 0; i < fDomainCount; i++) {
			_EnumerateBus(i, 0);
		}
	}

	if (1) {
		for (int i = 0; i < fDomainCount; i++) {
			_FixupDevices(i, 0);
		}
	}

	if (fRootBus) {
		_DiscoverBus(fRootBus);
		_ConfigureBridges(fRootBus);
		ClearDeviceStatus(fRootBus, false);
		_RefreshDeviceInfo(fRootBus);
	}
}


PCI::~PCI()
{
}


status_t
PCI::_CreateVirtualBus(int domain, uint8 bus, uint8 *virtualBus)
{
#if defined(__INTEL__)

	// IA32 doesn't use domains
	if (domain)
		panic("PCI::CreateVirtualBus domain != 0");
	*virtualBus = bus;
	return B_OK;

#else

	if (fNextVirtualBus > 0xff)
		panic("PCI::CreateVirtualBus: virtual bus number space exhausted");
	if (unsigned(domain) > 0xff)
		panic("PCI::CreateVirtualBus: domain %d too large", domain);

	uint16 value = domain << 8 | bus;

	for (VirtualBusMap::Iterator it = fVirtualBusMap.Begin(); it != fVirtualBusMap.End(); ++it) {
		if (it->Value() == value) {
			*virtualBus = it->Key();
			FLOW("PCI::CreateVirtualBus: domain %d, bus %d already in map => virtualBus %d\n", domain, bus, *virtualBus);
			return B_OK;
		}
	}

	*virtualBus = fNextVirtualBus++;

	FLOW("PCI::CreateVirtualBus: domain %d, bus %d => virtualBus %d\n", domain, bus, *virtualBus);

	return fVirtualBusMap.Insert(*virtualBus, value);

#endif
}


status_t
PCI::ResolveVirtualBus(uint8 virtualBus, int *domain, uint8 *bus)
{
#if defined(__INTEL__)

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
PCI::AddController(pci_controller *controller, void *controller_cookie)
{
	if (fDomainCount == MAX_PCI_DOMAINS)
		return B_ERROR;

	fDomainData[fDomainCount].controller = controller;
	fDomainData[fDomainCount].controller_cookie = controller_cookie;

	// initialized later to avoid call back into controller at this point
	fDomainData[fDomainCount].max_bus_devices = -1;

	fDomainCount++;
	return B_OK;
}

void
PCI::InitDomainData()
{
	for (int i = 0; i < fDomainCount; i++) {
		int32 count;
		status_t status;

		status = (*fDomainData[i].controller->get_max_bus_devices)(fDomainData[i].controller_cookie, &count);
		fDomainData[i].max_bus_devices = (status == B_OK) ? count : 0;
	}
}


domain_data *
PCI::_GetDomainData(int domain)
{
	if (domain < 0 || domain >= fDomainCount)
		return NULL;

	return &fDomainData[domain];
}


inline int
PCI::_NumFunctions(int domain, uint8 bus, uint8 device)
{
	uint8 type = ReadConfig(domain, bus, device,
		0, PCI_header_type, 1);
	return (type & PCI_multifunction) != 0 ? 8 : 1;
}


status_t
PCI::GetNthInfo(long index, pci_info *outInfo)
{
	long currentIndex = 0;
	if (!fRootBus)
		return B_ERROR;

	return _GetNthInfo(fRootBus, &currentIndex, index, outInfo);
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

	if (bus->next)
		return _GetNthInfo(bus->next, currentIndex, wantIndex, outInfo);

	return B_ERROR;
}


void
PCI::_EnumerateBus(int domain, uint8 bus, uint8 *subordinateBus)
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

			TRACE(("PCI: found PCI-PCI bridge: domain %u, bus %u, dev %u, func %u\n", domain, bus, dev, function));
			TRACE(("PCI: original settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
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

			TRACE(("PCI: disabled settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
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

			TRACE(("PCI: probing settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
				ReadConfig(domain, bus, dev, function, PCI_command, 2),
				ReadConfig(domain, bus, dev, function, PCI_primary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_secondary_bus, 1),
				ReadConfig(domain, bus, dev, function, PCI_subordinate_bus, 1)));

			// enumerate bus
			_EnumerateBus(domain, lastUsedBusNumber + 1, &lastUsedBusNumber);

			// close Scheunentor
			WriteConfig(domain, bus, dev, function, PCI_subordinate_bus, 1, lastUsedBusNumber);

			TRACE(("PCI: configured settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
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
PCI::_FixupDevices(int domain, uint8 bus)
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
			&& dev->info.class_sub == PCI_pci) {
			uint16 bridgeControlOld = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_bridge_control, 2);
			uint16 bridgeControlNew = bridgeControlOld;
			// Enable: Parity Error Response, SERR, Master Abort Mode, Discard
			// Timer SERR
			// Clear: Discard Timer Status
			bridgeControlNew |= (1 << 0) | (1 << 1) | (1 << 5) | (1 << 10)
				| (1 << 11);
			// Set discard timer to 2^15 PCI clocks
			bridgeControlNew &= ~((1 << 8) | (1 << 9));
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_bridge_control, 2, bridgeControlNew);
			bridgeControlNew = ReadConfig(dev->domain, dev->bus, dev->device,
				dev->function, PCI_bridge_control, 2);
			dprintf("PCI: dom %u, bus %u, dev %2u, func %u, changed PCI bridge control from 0x%04x to 0x%04x\n",
				dev->domain, dev->bus, dev->device, dev->function, bridgeControlOld, bridgeControlNew);
		}

		if (dev->child)
			_ConfigureBridges(dev->child);
	}

	if (bus->next)
		_ConfigureBridges(bus->next);
}


void
PCI::ClearDeviceStatus(PCIBus *bus, bool dumpStatus)
{
	if (!bus) {
		if (!fRootBus)
			return;
		bus = fRootBus;
	}

	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		// Clear and dump PCI device status
		uint16 status = ReadConfig(dev->domain, dev->bus, dev->device,
			dev->function, PCI_status, 2);
		WriteConfig(dev->domain, dev->bus, dev->device, dev->function, PCI_status,
			2, status);
		if (dumpStatus) {
			kprintf("domain %u, bus %u, dev %2u, func %u, PCI device status 0x%04x\n",
				dev->domain, dev->bus, dev->device, dev->function, status);
			if (status & (1 << 15))
				kprintf("  Detected Parity Error\n");
			if (status & (1 << 14))
				kprintf("  Signalled System Error\n");
			if (status & (1 << 13))
				kprintf("  Received Master-Abort\n");
			if (status & (1 << 12))
				kprintf("  Received Target-Abort\n");
			if (status & (1 << 11))
				kprintf("  Signalled Target-Abort\n");
			if (status & (1 << 8))
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
				kprintf("domain %u, bus %u, dev %2u, func %u, PCI bridge secondary status 0x%04x\n",
					dev->domain, dev->bus, dev->device, dev->function, secondaryStatus);
				if (secondaryStatus & (1 << 15))
					kprintf("  Detected Parity Error\n");
				if (secondaryStatus & (1 << 14))
					kprintf("  Received System Error\n");
				if (secondaryStatus & (1 << 13))
					kprintf("  Received Master-Abort\n");
				if (secondaryStatus & (1 << 12))
					kprintf("  Received Target-Abort\n");
				if (secondaryStatus & (1 << 11))
					kprintf("  Signalled Target-Abort\n");
				if (secondaryStatus & (1 << 8))
					kprintf("  Data Parity Reported\n");
			}

			// Clear and dump the discard-timer error bit located in bridge-control register
			uint16 bridgeControl = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_bridge_control, 2);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_bridge_control, 2, bridgeControl);
			if (dumpStatus) {
				kprintf("domain %u, bus %u, dev %2u, func %u, PCI bridge control 0x%04x\n",
					dev->domain, dev->bus, dev->device, dev->function, bridgeControl);
				if (bridgeControl & (1 << 10)) {
					kprintf("  bridge-control: Discard Timer Error\n");
				}
			}
		}

		if (dev->child)
			ClearDeviceStatus(dev->child, dumpStatus);
	}

	if (bus->next)
		ClearDeviceStatus(bus->next, dumpStatus);
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

	if (bus->next)
		_DiscoverBus(bus->next);
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
	if (baseClass == PCI_bridge && subClass == PCI_pci) {
		uint8 secondaryBus = ReadConfig(bus->domain, bus->bus, dev, function,
			PCI_secondary_bus, 1);
		PCIBus *newBus = _CreateBus(newDev, bus->domain, secondaryBus);
		_DiscoverBus(newBus);
	}
}


PCIBus *
PCI::_CreateBus(PCIDev *parent, int domain, uint8 bus)
{
	PCIBus *newBus = new(std::nothrow) PCIBus;
	if (newBus == NULL)
		return NULL;

	newBus->next = NULL;
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
	FLOW("PCI: CreateDevice, domain %u, bus %u, dev %u, func %u:\n", parent->domain, parent->bus, device, function);

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

	_ReadBasicInfo(newDev);

	FLOW("PCI: CreateDevice, vendor 0x%04x, device 0x%04x, class_base 0x%02x, class_sub 0x%02x\n",
		newDev->info.vendor_id, newDev->info.device_id, newDev->info.class_base, newDev->info.class_sub);

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


uint32
PCI::_BarSize(uint32 bits, uint32 mask)
{
	bits &= mask;
	if (!bits)
		return 0;
	uint32 size = 1;
	while (!(bits & size))
		size <<= 1;
	return size;
}


void
PCI::_GetBarInfo(PCIDev *dev, uint8 offset, uint32 *_address, uint32 *_size,
	uint8 *_flags)
{
	uint32 oldValue = ReadConfig(dev->domain, dev->bus, dev->device, dev->function,
		offset, 4);
	WriteConfig(dev->domain, dev->bus, dev->device, dev->function, offset, 4,
		0xffffffff);
	uint32 newValue = ReadConfig(dev->domain, dev->bus, dev->device, dev->function,
		offset, 4);
	WriteConfig(dev->domain, dev->bus, dev->device, dev->function, offset, 4,
		oldValue);

	*_address = oldValue & PCI_address_memory_32_mask;
	if (_size != NULL)
		*_size = _BarSize(newValue, PCI_address_memory_32_mask);
	if (_flags != NULL)
		*_flags = newValue & 0xf;
}


void
PCI::_GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 *_address, uint32 *_size,
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

	*_address = oldValue & PCI_rom_address_mask;
	if (_size != NULL)
		*_size = _BarSize(newValue, PCI_rom_address_mask);
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
			_GetRomBarInfo(dev, PCI_rom_base, &dev->info.u.h0.rom_base_pci,
				&dev->info.u.h0.rom_size);
			for (int i = 0; i < 6; i++) {
				_GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h0.base_registers_pci[i],
					&dev->info.u.h0.base_register_sizes[i],
					&dev->info.u.h0.base_register_flags[i]);
			}

			// restore PCI device address decoding
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_command, 2, pcicmd);

			dev->info.u.h0.rom_base = (ulong)pci_ram_address(
				(void *)dev->info.u.h0.rom_base_pci);
			for (int i = 0; i < 6; i++) {
				dev->info.u.h0.base_registers[i] = (ulong)pci_ram_address(
					(void *)dev->info.u.h0.base_registers_pci[i]);
			}

			dev->info.u.h0.cardbus_cis = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_cardbus_cis, 4);
			dev->info.u.h0.subsystem_id = ReadConfig(dev->domain, dev->bus,
				dev->device, dev->function, PCI_subsystem_id, 2);
			dev->info.u.h0.subsystem_vendor_id = ReadConfig(dev->domain,
				dev->bus, dev->device, dev->function, PCI_subsystem_vendor_id, 2);
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
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = ReadConfig(dev->domain, dev->bus, dev->device,
				dev->function, PCI_command, 2);
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_command, 2,
				pcicmd & ~(PCI_command_io | PCI_command_memory));

			_GetRomBarInfo(dev, PCI_bridge_rom_base,
				&dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				_GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h1.base_registers_pci[i],
					&dev->info.u.h1.base_register_sizes[i],
					&dev->info.u.h1.base_register_flags[i]);
			}

			// restore PCI device address decoding
			WriteConfig(dev->domain, dev->bus, dev->device, dev->function,
				PCI_command, 2, pcicmd);

			dev->info.u.h1.rom_base = (ulong)pci_ram_address(
				(void *)dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				dev->info.u.h1.base_registers[i] = (ulong)pci_ram_address(
					(void *)dev->info.u.h1.base_registers_pci[i]);
			}

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
				dev->bus, dev->device, dev->function, PCI_prefetchable_memory_base, 2);
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
	if (fRootBus == NULL)
		return;

	_RefreshDeviceInfo(fRootBus);
}


void
PCI::_RefreshDeviceInfo(PCIBus *bus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		_ReadBasicInfo(dev);
		_ReadHeaderInfo(dev);
#ifdef __INTEL__
		pci_read_arch_info(dev);
#endif
		if (dev->child)
			_RefreshDeviceInfo(dev->child);
	}

	if (bus->next)
		_RefreshDeviceInfo(bus->next);
}


status_t
PCI::ReadConfig(int domain, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size, uint32 *value)
{
	domain_data *info = _GetDomainData(domain);
	if (!info)
		return B_ERROR;

	if (device > (info->max_bus_devices - 1)
		|| function > 7
		|| (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3)
		|| (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: can't read config for domain %d, bus %u, device %u, function %u, offset %u, size %u\n",
			 domain, bus, device, function, offset, size);
		return B_ERROR;
	}

	return (*info->controller->read_pci_config)(info->controller_cookie, bus,
		device, function, offset, size, value);
}


uint32
PCI::ReadConfig(int domain, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size)
{
	uint32 value;
	if (ReadConfig(domain, bus, device, function, offset, size, &value)
			!= B_OK)
		return 0xffffffff;

	return value;
}


uint32
PCI::ReadConfig(PCIDev *device, uint8 offset, uint8 size)
{
	uint32 value;
	if (ReadConfig(device->domain, device->bus, device->device,
			device->function, offset, size, &value) != B_OK)
		return 0xffffffff;

	return value;
}


status_t
PCI::WriteConfig(int domain, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size, uint32 value)
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
PCI::WriteConfig(PCIDev *device, uint8 offset, uint8 size, uint32 value)
{
	return WriteConfig(device->domain, device->bus, device->device,
		device->function, offset, size, value);
}


status_t
PCI::FindCapability(int domain, uint8 bus, uint8 device, uint8 function,
	uint8 capID, uint8 *offset)
{
	if (offset == NULL) {
		TRACE_CAP("PCI: FindCapability() ERROR %u:%u:%u capability %#02x offset NULL pointer\n", bus, device, function, capID);
		return B_BAD_VALUE;
	}

	uint16 status = ReadConfig(domain, bus, device, function, PCI_status, 2);
	if (!(status & PCI_status_capabilities)) {
		TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x not supported\n", bus, device, function, capID);
		return B_ERROR;
	}

	uint8 headerType = ReadConfig(domain, bus, device, function,
		PCI_header_type, 1);
	uint8 capPointer;

	switch (headerType & PCI_header_type_mask) {
		case PCI_header_type_generic:
		case PCI_header_type_PCI_to_PCI_bridge:
			capPointer = ReadConfig(domain, bus, device, function,
				PCI_capabilities_ptr, 1);
			break;
		case PCI_header_type_cardbus:
			capPointer = ReadConfig(domain, bus, device, function,
				PCI_capabilities_ptr_2, 1);
			break;
		default:
			TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x unknown header type\n", bus, device, function, capID);
			return B_ERROR;
	}

	capPointer &= ~3;
	if (capPointer == 0) {
		TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x empty list\n", bus, device, function, capID);
		return B_NAME_NOT_FOUND;
	}

	for (int i = 0; i < 48; i++) {
		if (ReadConfig(domain, bus, device, function, capPointer, 1) == capID) {
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


PCIDev *
PCI::FindDevice(int domain, uint8 bus, uint8 device, uint8 function)
{
	return _FindDevice(fRootBus, domain, bus, device, function);
}


PCIDev *
PCI::_FindDevice(PCIBus *current, int domain, uint8 bus, uint8 device,
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

	// search other busses
	if (current->next != NULL)
		return _FindDevice(current->next, domain, bus, device, function);

	return NULL;
}


status_t
PCI::UpdateInterruptLine(int domain, uint8 bus, uint8 _device, uint8 function,
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
