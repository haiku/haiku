#include <KernelExport.h>
#include <PCI.h>
#include "pci.h"


#define USE_PCI_HEADER 0

#if USE_PCI_HEADER
  #include "pcihdr.h"
#endif

void pci_print_info()
{
	pci_info info;
	for (long index = 0; B_OK == pci_get_nth_pci_info(index, &info); index++) {
		dprintf("PCI: %2ld: vendor 0x%04x, device 0x%04x, class_base 0x%02x, class_sub 0x%02x\n",
			index, info.vendor_id, info.device_id, info.class_base, info.class_sub);
	}
}

