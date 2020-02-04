/*
 * Copyright 2007, François Revol <revol@free.fr>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "pci_atari.h"

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <PCI.h>

#include "pci_controller.h"

/*
 * Here we fake a PCI bus that maps the physical memory
 * (which is also I/O on 68k), and fake some system devices.
 * Some other devices are faked as ISA because they need DMA
 * notification.
 * 
 * TODO: anything to be done to support VME cards ?
 * I don't think they are PnP at all anyway.
 * 
 * TODO: On Hades/Milan clones a real PCI bus is accessible
 * through PAE-like extra bits in page descriptors. This one
 * should be handled in a separate file.
 */

//XXX:find one and put in shared priv header!
// 0x68xx is free according to pci.ids
// 68fx (f=fake) x = 0:amiga, 1:apple, 2:atari
#define FAKEV 0x68f2

// bus number
#define BN 0

// default line size
#define DLL 1
// default latency
#define DL 0
// default header_type
#define DH PCI_header_type_generic
// default bist
#define DB 0

#define PEI 0

#define INVV 0xffff //0x0000 ??
#define INVD 0xffff

struct fake_pci_device {
	pci_info info;
	
};

static struct fake_pci_device gFakePCIDevices[] = {
{ {FAKEV, 0x0000, BN, 0, 0, 0, 0xff, PCI_host, PCI_bridge, DLL, DL, DH, DB, 0, PEI }}, /* cpu */
{ {FAKEV, 0x0001, BN, 1, 0, 0, 0xff, 0x68/*fake*/, PCI_processor, DLL, DL, DH, DB, 0, PEI }}, /* cpu */
{ {FAKEV, 0x0002, BN, 2, 0, 0, 0xff, PCI_display_other, PCI_display, DLL, DL, DH, DB, 0, /*0xFFFF8200,*/ PEI }}, /* gfx */
{ {FAKEV, 0x0003, BN, 3, 0, 0, 0xff, PCI_ide, PCI_mass_storage, DLL, DL, DH, DB, 0, /*0xFFF00000,*/ PEI }}, /* ide */
{ {FAKEV, 0x0004, BN, 4, 0, 0, 0xff, PCI_scsi, PCI_mass_storage, DLL, DL, DH, DB, 0, PEI }}, /* scsi */
{ {FAKEV, 0x0005, BN, 5, 0, 0, 0xff, 0x0/*CHANGEME*/, PCI_multimedia, DLL, DL, DH, DB, /*0x00,*/ 0, /*0xFFFF8900,*/ PEI }}, /* snd */
//UART ?
//centronics?
{ {INVV, INVD} }
};
#define FAKE_DEVICES_COUNT (sizeof(gFakePCIDevices)/sizeof(struct fake_pci_device)-1)

struct m68k_atari_fake_host_bridge {
	uint32					bus;
};


#define out8rb(address, value)	m68k_out8((vuint8*)(address), value)
#define out16rb(address, value)	m68k_out16_reverse((vuint16*)(address), value)
#define out32rb(address, value)	m68k_out32_reverse((vuint32*)(address), value)
#define in8rb(address)			m68k_in8((const vuint8*)(address))
#define in16rb(address)			m68k_in16_reverse((const vuint16*)(address))
#define in32rb(address)			m68k_in32_reverse((const vuint32*)(address))


static int		m68k_atari_enable_config(struct m68k_atari_fake_host_bridge *bridge,
					uint8 bus, uint8 slot, uint8 function, uint8 offset);

static status_t	m68k_atari_read_pci_config(void *cookie, uint8 bus, uint8 device,
					uint8 function, uint16 offset, uint8 size, uint32 *value);
static status_t	m68k_atari_write_pci_config(void *cookie, uint8 bus,
					uint8 device, uint8 function, uint16 offset, uint8 size,
					uint32 value);
static status_t	m68k_atari_get_max_bus_devices(void *cookie, int32 *count);
static status_t	m68k_atari_read_pci_irq(void *cookie, uint8 bus, uint8 device,
					uint8 function, uint8 pin, uint8 *irq);
static status_t	m68k_atari_write_pci_irq(void *cookie, uint8 bus, uint8 device,
					uint8 function, uint8 pin, uint8 irq);

static pci_controller sM68kAtariPCIController = {
	m68k_atari_read_pci_config,
	m68k_atari_write_pci_config,
	m68k_atari_get_max_bus_devices,
	m68k_atari_read_pci_irq,
	m68k_atari_write_pci_irq,
};


static status_t
m68k_atari_read_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 *value)
{
	struct fake_pci_device *devices = (struct fake_pci_device *)cookie;
	struct fake_pci_device *dev;

	if (bus != 0)
		return EINVAL;
	if (device >= FAKE_DEVICES_COUNT)
		return EINVAL;
	if (function != 0)
		return EINVAL;
	dev = &devices[device];
	
#define O(pn,n,s)			\
	case pn:				\
		if (size != s) {	\
			panic("invalid pci config size %d for offset %d", size, offset); \
			return EINVAL;	\
		}			\
		*value = dev->info.n;	\
		return B_OK

	if (1) {
		switch (offset) {
		O(PCI_vendor_id, vendor_id, 2);
		O(PCI_device_id, device_id, 2);
		O(PCI_revision, revision, 1);
		O(PCI_class_api, class_api, 1);
		O(PCI_class_sub, class_sub, 1);
		O(PCI_class_base, class_base, 1);
		O(PCI_line_size, line_size, 1);
		O(PCI_latency, latency, 1);
		O(PCI_header_type, header_type, 1);
		O(PCI_bist, bist, 1);
		}
	}
//#undef O
#if 0
#define PCI_command                             0x04            /* (2 byte) command */
#define PCI_status                              0x06            /* (2 byte) status */
#endif

	if (dev->info.header_type == 0x00 || dev->info.header_type == 0x01) {
		switch (offset) {
		case PCI_base_registers:
			return EINVAL;
		O(PCI_interrupt_line, u.h0.interrupt_line, 1);
		O(PCI_interrupt_pin, u.h0.interrupt_pin, 1);
			default:
				break;
		}
	}

	if (dev->info.header_type == 0x00) {
		switch (offset) {
			default:
				break;
		}
	}

	if (dev->info.header_type == 0x01) {
		switch (offset) {
		O(PCI_primary_bus, u.h1.primary_bus, 1);
		O(PCI_secondary_bus, u.h1.secondary_bus, 1);
		O(PCI_subordinate_bus, u.h1.subordinate_bus, 1);
		O(PCI_secondary_latency, u.h1.secondary_latency, 1);
			default:
				break;
		}
	}

	*value = 0xffffffff;
	panic("invalid pci config offset %d", offset);
	return EINVAL;
	//return B_OK;
}


static status_t
m68k_atari_write_pci_config(void *cookie, uint8 bus, uint8 device,
	uint8 function, uint16 offset, uint8 size, uint32 value)
{
#if 0
	if (m68k_atari_enable_config(bridge, bus, device, function, offset)) {
		switch (size) {
			case 1:
				out8rb(caoff, (uint8)value);
				(void)in8rb(caoff);
				break;
			case 2:
				out16rb(caoff, (uint16)value);
				(void)in16rb(caoff);
				break;
			case 4:
				out32rb(caoff, value);
				(void)in32rb(caoff);
				break;
		}
	}

#endif
	panic("write pci config dev %d offset %d", device, offset);
	return B_ERROR;
	return B_OK;
}


static status_t
m68k_atari_get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 32;
	return B_OK;
}


static status_t
m68k_atari_read_pci_irq(void *cookie, uint8 bus, uint8 device,
	uint8 function, uint8 pin, uint8 *irq)
{
#warning M68K: WRITEME
	return B_ERROR;
}


static status_t
m68k_atari_write_pci_irq(void *cookie, uint8 bus, uint8 device,
	uint8 function, uint8 pin, uint8 irq)
{
#warning M68K: WRITEME
	return B_ERROR;
}


// #pragma mark -


static int
m68k_atari_enable_config(struct m68k_atari_fake_host_bridge *bridge, uint8 bus,
	uint8 slot, uint8 function, uint8 offset)
{
#warning M68K: WRITEME
	return 0;
}


// #pragma mark -




status_t
m68k_atari_pci_controller_init(void)
{
	struct m68k_atari_fake_host_bridge *bridge;
	bridge = (struct m68k_atari_fake_host_bridge *)
		malloc(sizeof(struct m68k_atari_fake_host_bridge));
	if (!bridge)
		return B_NO_MEMORY;

	bridge->bus = 0;

	status_t error = pci_controller_add(&sM68kAtariPCIController, bridge);

	if (error != B_OK)
		free(bridge);

	// TODO: probe Hades & Milan bridges
	return error;
}


// #pragma mark - support functions


