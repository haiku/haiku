#include "pci_priv.h"

status_t
pci_irq_init(void)
{
	return B_ERROR;
}


uint8
pci_read_irq(uint8 bus, uint8 device, uint8 function, uint8 line)
{
	return 0;
}


void
pci_write_irq(uint8 bus, uint8 device, uint8 function, uint8 line, uint8 irq)
{
}
