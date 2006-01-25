#ifndef __PCI_X86_BIOS_H
#define __PCI_X86_BIOS_H

#include <SupportDefs.h>

status_t pci_bios_init(void);

status_t pci_bios_read_config(void *cookie, 
							  uint8 bus, uint8 device, uint8 function,
							  uint8 offset, uint8 size, uint32 *value);

status_t pci_bios_write_config(void *cookie, 
							   uint8 bus, uint8 device, uint8 function,
							   uint8 offset, uint8 size, uint32 value);

status_t pci_bios_get_max_bus_devices(void *cookie, int32 *count);

#endif
