#include "pci_priv.h"

status_t
pci_config_init()
{
	return B_OK;
}


uint32
read_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	return 0;
}


void
write_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32	value)
{
}
