#include "pci_priv.h"
#include "pci_bios.h"

status_t
pci_bios_init(void)
{
	return B_ERROR;
}


status_t
pci_bios_read_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					 uint8 offset, uint8 size, uint32 *value)
{
	return B_ERROR;
}


status_t
pci_bios_write_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					  uint8 offset, uint8 size, uint32 value)
{
	return B_ERROR;
}


status_t
pci_bios_get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 32;
	return B_OK;
}
