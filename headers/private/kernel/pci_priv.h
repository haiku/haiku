#include <BeBuild.h>
#include <SupportDefs.h>

status_t 	pci_config_init();
uint32		read_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void		write_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32	value);

status_t 	pci_io_init();
uint8		read_io_8(int mapped_io_addr);
void		write_io_8(int mapped_io_addr, uint8 value);
uint16		read_io_16(int mapped_io_addr);
void		write_io_16(int mapped_io_addr, uint16 value);
uint32		read_io_32(int mapped_io_addr);
void		write_io_32(int mapped_io_addr, uint32 value);
void *		ram_address(const void *physical_address_in_system_memory);
