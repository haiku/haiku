#include <KernelExport.h>
#include <PCI.h>
#include "util/kernel_cpp.h"
#include "pci_priv.h"
#include "pci.h"

static PCI *pcidb;

void
pci_init(void)
{
	pcidb = new PCI;
}

void
pci_uninit(void)
{
	delete pcidb;
}

long
pci_get_nth_pci_info(long index, pci_info *outInfo)
{
	return pcidb->GetNthPciInfo(index, outInfo);
}


PCI::PCI()
{
	fRootBus.parent = 0;
	fRootBus.bus = 0;
	fRootBus.child = 0;
	
	DiscoverBus(&fRootBus);
}


PCI::~PCI()
{
}


status_t
PCI::GetNthPciInfo(long index, pci_info *outInfo)
{
	long curindex = 0;
	return GetNthPciInfo(&fRootBus, &curindex, index, outInfo);
}


status_t
PCI::GetNthPciInfo(PCIBus *bus, long *curindex, long wantindex, pci_info *outInfo)
{
	// maps tree structure to linear indexed view
	PCIDev *dev = bus->child;
	while (dev) {
		if (*curindex == wantindex) {
			*outInfo = dev->info;
			return B_OK;
		}
		*curindex += 1;
		if (dev->child && B_OK == GetNthPciInfo(dev->child, curindex, wantindex, outInfo))
			return B_OK;
		dev = dev->next;
	}
	return B_ERROR;
}


void
PCI::DiscoverBus(PCIBus *bus)
{
	dprintf("PCI: DiscoverBus, bus %u\n", bus->bus);

	for (int dev = 0; dev < gMaxBusDevices; dev++) {
		uint16 vendor_id = pci_read_config(bus->bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;
		
		uint8 type = pci_read_config(bus->bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++)
			DiscoverDevice(bus, dev, func);
	}
}


void
PCI::DiscoverDevice(PCIBus *bus, uint8 dev, uint8 func)
{
	dprintf("PCI: DiscoverDevice, bus %u, dev %u, func %u\n", bus->bus, dev, func);

	uint16 device_id = pci_read_config(bus->bus, dev, func, PCI_device_id, 2);
	if (device_id == 0xffff)
		return;

	PCIDev *newdev = CreateDevice(bus, dev, func);

	uint8 base_class = pci_read_config(bus->bus, dev, func, PCI_class_base, 1);
	uint8 sub_class	 = pci_read_config(bus->bus, dev, func, PCI_class_sub, 1);
	if (base_class == PCI_bridge && sub_class == PCI_pci) {
		uint8 secondary_bus = pci_read_config(bus->bus, dev, func, PCI_secondary_bus, 1);
		PCIBus *newbus = CreateBus(newdev, secondary_bus);
		DiscoverBus(newbus);
	}
}


PCIBus *
PCI::CreateBus(PCIDev *parent, uint8 bus)
{
	PCIBus *newbus = new PCIBus;
	newbus->parent = parent;
	newbus->bus = bus;
	newbus->child = 0;
	
	// append
	parent->child = newbus;
	
	return newbus;
}

	
PCIDev *
PCI::CreateDevice(PCIBus *parent, uint8 dev, uint8 func)
{
	dprintf("PCI: CreateDevice, bus %u, dev %u, func %u:\n", parent->bus, dev, func);
	
	PCIDev *newdev = new PCIDev;
	newdev->next = 0;
	newdev->parent = parent;
	newdev->child = 0;
	newdev->bus = parent->bus;
	newdev->dev = dev;
	newdev->func = func;

	ReadPciBasicInfo(newdev);

	dprintf("PCI: vendor 0x%04x, device 0x%04x, class_base 0x%02x, class_sub 0x%02x\n",
		newdev->info.vendor_id, newdev->info.device_id, newdev->info.class_base, newdev->info.class_sub);

	// append
	if (parent->child == 0) {
		parent->child = newdev;
	} else {
		PCIDev *sub = parent->child;
		while (sub->next)
			sub = sub->next;
		sub->next = newdev;
	}
	
	return newdev;
}


void
PCI::ReadPciBasicInfo(PCIDev *dev)
{
	dev->info.vendor_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_vendor_id, 2);
	dev->info.device_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_device_id, 2);
	dev->info.bus = dev->bus;
	dev->info.device = dev->dev;
	dev->info.function = dev->func;
	dev->info.revision = pci_read_config(dev->bus, dev->dev, dev->func, PCI_revision, 1);
	dev->info.class_api = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_api, 1);
	dev->info.class_sub = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_sub, 1);
	dev->info.class_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_base, 1);
	dev->info.line_size = pci_read_config(dev->bus, dev->dev, dev->func, PCI_line_size, 1);
	dev->info.latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_latency, 1);
	dev->info.header_type = pci_read_config(dev->bus, dev->dev, dev->func, PCI_header_type, 1);
	dev->info.header_type &= PCI_header_type_mask; // this masks off the multifunction bit
	dev->info.bist = pci_read_config(dev->bus, dev->dev, dev->func, PCI_bist, 1);
	dev->info.reserved = 0;
}


void
PCI::ReadPciHeaderInfo(PCIDev *dev)
{
	switch (dev->info.header_type) {
		case 0:
			break;
		case 1:
			break;
		default:
			dprintf("PCI: Header type unknown (%d)\n", dev->info.header_type);
			break;
	}
}
