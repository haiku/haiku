#include <KernelExport.h>
#include <bus_manager.h>
#include <PCI.h>
#include "pci_priv.h"

bool gIrqRouterAvailable = false;

void pci_scan_bus(uint8 bus);
long get_nth_pci_info(long index, pci_info *outInfo);
status_t pci_module_init(void);
status_t pci_module_uninit(void);
int32 std_ops(int32 op, ...);
status_t pci_module_rescan(void);


void
pci_scan_bus(uint8 bus)
{
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
