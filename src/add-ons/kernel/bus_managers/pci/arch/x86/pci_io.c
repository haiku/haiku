#include "pci_priv.h"

status_t
pci_io_init()
{
	return B_OK;
}


uint8
read_io_8(int mapped_io_addr)
{
	return 0;
}


void
write_io_8(int mapped_io_addr, uint8 value)
{
}


uint16
read_io_16(int mapped_io_addr)
{
	return 0;
}


void
write_io_16(int mapped_io_addr, uint16 value)
{
}


uint32
read_io_32(int mapped_io_addr)
{
	return 0;
}


void
write_io_32(int mapped_io_addr, uint32 value)
{
}


void *
ram_address(const void *physical_address_in_system_memory)
{
	return 0;
}
