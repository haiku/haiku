#include <BeBuild.h>
#include <SupportDefs.h>

status_t 	pci_config_init(void);
uint32		pci_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void		pci_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32	value);
void *		pci_ram_address(const void *physical_address_in_system_memory);

status_t 	pci_io_init(void);
uint8		pci_read_io_8(int mapped_io_addr);
void		pci_write_io_8(int mapped_io_addr, uint8 value);
uint16		pci_read_io_16(int mapped_io_addr);
void		pci_write_io_16(int mapped_io_addr, uint16 value);
uint32		pci_read_io_32(int mapped_io_addr);
void		pci_write_io_32(int mapped_io_addr, uint32 value);

status_t	pci_irq_init(void);
uint8		pci_read_irq(uint8 bus, uint8 device, uint8 function, uint8 line);
void		pci_write_irq(uint8 bus, uint8 device, uint8 function, uint8 line, uint8 irq);

extern int	gMaxBusDevices;
extern bool gIrqRouterAvailable;

#define PCI_LOCK_CONFIG(status) \
{ \
	status = disable_interrupts(); \
	acquire_spinlock(&gConfigLock); \
}

#define PCI_UNLOCK_CONFIG(cpu_status) \
{ \
	release_spinlock(&gConfigLock); \
	restore_interrupts(cpu_status); \
}
