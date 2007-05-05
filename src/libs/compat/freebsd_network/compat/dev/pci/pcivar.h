#ifndef _FBSD_COMPAT_DEV_PCI_PCIVAR_H_
#define _FBSD_COMPAT_DEV_PCI_PCIVAR_H_

#include <sys/bus.h>

int pci_enable_busmaster(device_t dev);
int pci_enable_io(device_t dev, int reg);

uint32_t pci_get_devid(device_t dev);
void pci_set_intpin(device_t dev, uint8_t pin);
uint8_t pci_get_intpin(device_t dev);

uint16_t pci_get_vendor(device_t dev);
uint16_t pci_get_device(device_t dev);
uint16_t pci_get_subvendor(device_t dev);
uint16_t pci_get_subdevice(device_t dev);
uint8_t pci_get_revid(device_t dev);

uint32_t pci_read_config(device_t dev, int reg, int width);
void pci_write_config(device_t dev, int reg, uint32_t val, int width);

#endif
