#include <KernelExport.h>
#include "pci_priv.h"
#include "bios.h"
#include "arch_cpu.h"

static uint32 (*gReadConfigFunc)(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
static void (*gWriteConfigFunc)(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value);

int	gMaxBusDevices;

status_t detect_config_mechanism(void);
bool pci_config_access_ok(void);
uint32 pci_mech1_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void pci_mech1_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value);
uint32 pci_mech2_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void pci_mech2_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value);


#define PCI_MECH1_REQ_PORT				0xCF8
#define PCI_MECH1_DATA_PORT 			0xCFC
#define PCI_MECH1_REQ_DATA(bus, device, func, offset) \
	(0x80000000 | (bus << 16) | (device << 11) | (func << 8) | (offset & ~3))

#define PCI_MECH2_ENABLE_PORT			0x0cf8
#define PCI_MECH2_FORWARD_PORT			0x0cfa
#define PCI_MECH2_CONFIG_PORT(dev, offset) \
	(uint16)(0xC00 | (dev << 8) | offset)


status_t
pci_config_init(void)
{
	pci_bios_init();
	
	if (B_OK != detect_config_mechanism()) {
		dprintf("PCI: failed to detect configuration mechanism\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
detect_config_mechanism(void)
{
	// PCI configuration mechanism 1 is the preferred one.
	// If it doesn't work, try mechanism 2.
	// Finally, try to fallback to PCI BIOS

	if (1) {
		// check for mechanism 1
		out32(0x80000000, PCI_MECH1_REQ_PORT);
		if (0x80000000 == in32(PCI_MECH1_REQ_PORT)) {
			dprintf("PCI: mechanism 1 found\n");
			gMaxBusDevices = 32;
			gReadConfigFunc = pci_mech1_read_config;
			gWriteConfigFunc = pci_mech1_write_config;
			if (pci_config_access_ok()) {
				dprintf("PCI: using mechanism 1\n");
				return B_OK;
			}
		}
	}

	if (1) {
		// check for mechanism 2
		out8(0x00, 0xCFB);
		out8(0x00, 0xCF8);
		out8(0x00, 0xCFA);
		if (in8(0xCF8) == 0x00 && in8(0xCFA) == 0x00) {
			dprintf("PCI: mechanism 2 found\n");
			gMaxBusDevices = 16;
			gReadConfigFunc = pci_mech2_read_config;
			gWriteConfigFunc = pci_mech2_write_config;
			if (pci_config_access_ok()) {
				dprintf("PCI: using mechanism 2\n");
				return B_OK;
			}
		}
	}

	if (1) {
		// check for PCI BIOS
		if (pci_bios_available()) {
			gReadConfigFunc = pci_bios_read_config;
			gWriteConfigFunc = pci_bios_write_config;
			gMaxBusDevices = 32;
			if (pci_config_access_ok()) {
				dprintf("PCI: using PCI BIOS\n");
				return B_OK;
			}
		}
	}
	
	dprintf("PCI: no configuration mechanism found\n");
	return B_ERROR;
}


bool
pci_config_access_ok(void)
{
	return true;
}


uint32
pci_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	return gReadConfigFunc(bus, device, function, offset, size);
}


void
pci_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value)
{
	gWriteConfigFunc(bus, device, function, offset, size, value);
}


uint32
pci_mech1_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	uint32 result;
	cpu_status status;
	if (device > 31 || function > 7 || (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3) || (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: mech1 can't read config for bus %u, device %u, function %u, offset %u, size %u\n",
			 bus, device, function, offset, size);
		return 0xffffffff;
	}
	
	PCI_LOCK_CONFIG(status);
	out32(PCI_MECH1_REQ_DATA(bus, device, function, offset), PCI_MECH1_REQ_PORT);
	switch (size) {
		case 1:
			result = in8(PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 2:
			result = in16(PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 4:
			result = in32(PCI_MECH1_DATA_PORT);
			break;
		default:
			result = 0xffffffff;
	}
	PCI_UNLOCK_CONFIG(status);
	return result;
}


void
pci_mech1_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value)
{
	cpu_status status;
	if (device > 31 || function > 7 || (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3) || (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: mech1 can't write config for bus %u, device %u, function %u, offset %u, size %u\n",
			 bus, device, function, offset, size);
		return;
	}
	
	PCI_LOCK_CONFIG(status);
	out32(PCI_MECH1_REQ_DATA(bus, device, function, offset), PCI_MECH1_REQ_PORT);
	switch (size) {
		case 1:
			out8(value, PCI_MECH1_DATA_PORT + (offset & 3));
		break;
		case 2:
			out16(value, PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 4:
			out32(value, PCI_MECH1_DATA_PORT);
			break;
	}
	PCI_UNLOCK_CONFIG(status);
}


uint32
pci_mech2_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	uint32 result;
	cpu_status status;
	if (device > 15 || function > 7 || (size != 1 && size != 2 && size != 4)) {
		dprintf("PCI: mech2 can't read config for bus %u, device %u, function %u, offset %u, size %u\n",
			 bus, device, function, offset, size);
		return 0xffffffff;
	}
	
	PCI_LOCK_CONFIG(status);
	out8((uint8)(0xf0 | (function << 1)), PCI_MECH2_ENABLE_PORT);
	out8(bus, PCI_MECH2_FORWARD_PORT);
	switch (size) {
		case 1:
			result = in8(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 2:
			result = in16(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 4:
			result = in32(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		default:
			result = 0xffffffff;
	}
	out8(0, PCI_MECH2_ENABLE_PORT);
	PCI_UNLOCK_CONFIG(status);
	return result;
}


void
pci_mech2_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value)
{
	cpu_status status;
	if (device > 15 || function > 7 || (size != 1 && size != 2 && size != 4)) {
		dprintf("PCI: mech2 can't write config for bus %u, device %u, function %u, offset %u, size %u\n",
			 bus, device, function, offset, size);
		return;
	}

	PCI_LOCK_CONFIG(status);
	out8((uint8)(0xf0 | (function << 1)), PCI_MECH2_ENABLE_PORT);
	out8(bus, PCI_MECH2_FORWARD_PORT);
	switch (size) {
		case 1:
			out8(value, PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 2:
			out16(value, PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 4:
			out32(value, PCI_MECH2_CONFIG_PORT(device, offset));
			break;
	}
	out8(0, PCI_MECH2_ENABLE_PORT);
	PCI_UNLOCK_CONFIG(status);
}


void *
pci_ram_address(const void *physical_address_in_system_memory)
{
	return physical_address_in_system_memory;
}
