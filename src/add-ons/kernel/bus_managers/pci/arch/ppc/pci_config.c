#include "pci_priv.h"

int	gMaxBusDevices;

status_t
pci_config_init()
{
	return B_OK;
}


uint32
pci_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	return 0;
}


void
pci_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32	value)
{
}


void *
pci_ram_address(const void *physical_address_in_system_memory)
{
	return 0;
}
