#include "pci_priv.h"
#include "bios.h"

void
pci_bios_init(void)
{
}


bool
pci_bios_available(void)
{
	return false;
}


uint32
pci_bios_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	return 0;
}


void
pci_bios_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value)
{
}
