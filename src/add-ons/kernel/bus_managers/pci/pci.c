#include <KernelExport.h>
#include <bus_manager.h>
#include <PCI.h>
#include "pci_priv.h"

bool 		gIrqRouterAvailable = false;
spinlock	gConfigLock = 0;

void		pci_scan_device(uint8 bus, uint8 dev, uint8 func);
void		pci_scan_bus(uint8 bus);
long		get_nth_pci_info(long index, pci_info *outInfo);
status_t	pci_module_init(void);
status_t	pci_module_uninit(void);
int32		std_ops(int32 op, ...);
status_t	pci_module_rescan(void);

void
pci_scan_device(uint8 bus, uint8 dev, uint8 func)
{
	uint16 vendorID;
	uint16 deviceID;
	uint8 base_class, sub_class;

	deviceID = pci_read_config(bus, dev, 0, PCI_device_id, 2);
	if (deviceID == 0xffff)
		return;

	dprintf("PCI: pci_scan_device, bus %u, device %u, function %u\n", bus, dev, func);

	vendorID = pci_read_config(bus, dev, 0, PCI_vendor_id, 2);

	base_class = pci_read_config(bus, dev, func, PCI_class_base, 1);
	sub_class  = pci_read_config(bus, dev, func, PCI_class_sub, 1);
	
	dprintf("PCI:   vendor 0x%04x, device 0x%04x, base_class 0x%02x, sub_class 0x%02x\n",
		vendorID, deviceID, base_class, sub_class);
		
	if (base_class == PCI_bridge) {
		uint8 secondary_bus = pci_read_config(bus, dev, func, PCI_secondary_bus, 1);
		pci_scan_bus(secondary_bus);
	}
}


void
pci_scan_bus(uint8 bus)
{
	uint8 dev;
	
	dprintf("PCI: pci_scan_bus, bus %u\n", bus);

	for (dev = 0; dev < gMaxBusDevices; dev++) {
		uint16 vendorID = pci_read_config(bus, dev, 0, PCI_vendor_id, 2);
		uint16 deviceID;
		uint8 type, func, nfunc;
		if (vendorID == 0xffff)
			continue;
		
		deviceID = pci_read_config(bus, dev, 0, PCI_device_id, 2);
		type = pci_read_config(bus, dev, 0, PCI_header_type, 1);
		type &= PCI_header_type_mask;
		if ((type & PCI_multifunction) == 0	/*&& !pci_quirk_multifunction(vendorID, deviceID) */)
			nfunc = 1;
		else
			nfunc = 8;

		for (func = 0; func < nfunc; func++)
			pci_scan_device(bus, dev, func);
	}
}


long
get_nth_pci_info(long index, pci_info *outInfo)
{
	return 0;
}


status_t
pci_module_init(void)
{
	dprintf("PCI: pci_module_init\n");

	if (B_OK != pci_io_init()) {
		dprintf("PCI: pci_io_init failed\n");
		return B_ERROR;
	}

	if (B_OK != pci_config_init()) {
		dprintf("PCI: pci_config_init failed\n");
		return B_ERROR;
	}
	
	if (B_OK != pci_irq_init()) {
		dprintf("PCI: IRQ router not available\n");
	} else {
		gIrqRouterAvailable = true;
	}

	pci_scan_bus(0);
	return B_OK;
}


status_t
pci_module_uninit(void)
{
	dprintf("PCI: pci_module_uninit\n");
	return B_OK;
}


status_t
pci_module_rescan(void)
{
	return B_OK;
}


int32
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return pci_module_init();

		case B_MODULE_UNINIT:
			return pci_module_uninit();
	}

	return EINVAL;	
}


struct pci_module_info pci_module = {
	{
		{
			B_PCI_MODULE_NAME,
			B_KEEP_LOADED,
			std_ops
		},
		pci_module_rescan
	},
	&pci_read_io_8,
	&pci_write_io_8,
	&pci_read_io_16,
	&pci_write_io_16,
	&pci_read_io_32,
	&pci_write_io_32,
	&get_nth_pci_info,
	&pci_read_config,
	&pci_write_config,
	&pci_ram_address
};


module_info *modules[] = {
	(module_info *)&pci_module,
	NULL
};
