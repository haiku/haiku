
void pci_bios_init(void);

bool pci_bios_available(void);

uint32 pci_bios_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void pci_bios_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value);
