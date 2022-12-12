/*
 *	Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 *	Copyright 2010, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */

#include "pci_msi.h"
#include "pci.h"
#include "pci_private.h"

#include <strings.h>

#include <arch/x86/msi.h>
#include <debug.h>

extern PCI *gPCI;


static status_t pci_unconfigure_msix(PCIDev *device);
static status_t pci_disable_msix(PCIDev *device);


static void
pci_ht_msi_map(PCIDev *device, uint64 address)
{
	ht_mapping_info *info = &device->ht_mapping;
	if (!info->ht_mapping_capable)
		return;

	bool enabled = (info->control_value & PCI_ht_command_msi_enable) != 0;
	if ((address != 0) != enabled) {
		if (enabled) {
			info->control_value &= ~PCI_ht_command_msi_enable;
		} else {
			if ((address >> 20) != (info->address_value >> 20))
				return;
			dprintf("ht msi mapping enabled\n");
			info->control_value |= PCI_ht_command_msi_enable;
		}
		gPCI->WriteConfig(device, info->capability_offset + PCI_ht_command, 2,
			info->control_value);
	}
}


void
pci_read_ht_mapping_info(PCIDev *device)
{
	if (!msi_supported())
		return;

	ht_mapping_info *info = &device->ht_mapping;
	info->ht_mapping_capable = false;

	uint8 offset = 0;
	if (gPCI->FindHTCapability(device, PCI_ht_command_cap_msi_mapping,
		&offset) == B_OK) {
		info->control_value = gPCI->ReadConfig(device, offset + PCI_ht_command,
			2);
		info->capability_offset = offset;
		info->ht_mapping_capable = true;
		if ((info->control_value & PCI_ht_command_msi_fixed) != 0)
			info->address_value = MSI_ADDRESS_BASE;
		else {
			info->address_value = gPCI->ReadConfig(device, offset
				+ PCI_ht_msi_address_high, 4);
			info->address_value <<= 32;
			info->address_value |= gPCI->ReadConfig(device, offset
				+ PCI_ht_msi_address_low, 4);
		}
		dprintf("found an ht msi mapping at %#" B_PRIx64 "\n",
			info->address_value);
	}
}


uint8
pci_get_msi_count(uint8 virtualBus, uint8 _device, uint8 function)
{
	if (!msi_supported())
		return 0;

	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return 0;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return 0;

	msi_info *info = &device->msi;
	if (!info->msi_capable)
		return 0;

	return info->message_count;
}


status_t
pci_configure_msi(uint8 virtualBus, uint8 _device, uint8 function,
	uint8 count, uint8 *startVector)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	if (count == 0 || startVector == NULL)
		return B_BAD_VALUE;

	uint8 bus;
	uint8 domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msi_info *info =  &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (count > 32 || count > info->message_count
		|| ((count - 1) & count) != 0 /* needs to be a power of 2 */) {
		return B_BAD_VALUE;
	}

	if (info->configured_count != 0)
		return B_BUSY;

	result = msi_allocate_vectors(count, &info->start_vector,
		&info->address_value, &info->data_value);
	if (result != B_OK)
		return result;

	uint8 offset = info->capability_offset;
	gPCI->WriteConfig(device, offset + PCI_msi_address, 4,
		info->address_value & 0xffffffff);
	if (info->control_value & PCI_msi_control_64bit) {
		gPCI->WriteConfig(device, offset + PCI_msi_address_high, 4,
			info->address_value >> 32);
		gPCI->WriteConfig(device, offset + PCI_msi_data_64bit, 2,
			info->data_value);
	} else
		gPCI->WriteConfig(device, offset + PCI_msi_data, 2, info->data_value);

	info->control_value &= ~PCI_msi_control_mme_mask;
	info->control_value |= (ffs(count) - 1) << 4;
	gPCI->WriteConfig(device, offset + PCI_msi_control, 2, info->control_value);

	info->configured_count = count;
	*startVector = info->start_vector;
	return B_OK;
}


status_t
pci_unconfigure_msi(uint8 virtualBus, uint8 _device, uint8 function)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	uint8 bus;
	uint8 domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	// try MSI-X
	result = pci_unconfigure_msix(device);
	if (result != B_UNSUPPORTED && result != B_NO_INIT)
		return result;

	msi_info *info =  &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	msi_free_vectors(info->configured_count, info->start_vector);

	info->control_value &= ~PCI_msi_control_mme_mask;
	gPCI->WriteConfig(device, info->capability_offset + PCI_msi_control, 2,
		info->control_value);

	info->configured_count = 0;
	info->address_value = 0;
	info->data_value = 0;
	return B_OK;
}


status_t
pci_enable_msi(uint8 virtualBus, uint8 _device, uint8 function)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	uint8 bus;
	uint8 domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msi_info *info =  &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// ensure the pinned interrupt is disabled
	gPCI->WriteConfig(device, PCI_command, 2,
		gPCI->ReadConfig(device, PCI_command, 2) | PCI_command_int_disable);

	// enable msi generation
	info->control_value |= PCI_msi_control_enable;
	gPCI->WriteConfig(device, info->capability_offset + PCI_msi_control, 2,
		info->control_value);

	// enable HT msi mapping (if applicable)
	pci_ht_msi_map(device, info->address_value);

	dprintf("msi enabled: 0x%04" B_PRIx32 "\n",
		gPCI->ReadConfig(device, info->capability_offset + PCI_msi_control, 2));
	return B_OK;
}


status_t
pci_disable_msi(uint8 virtualBus, uint8 _device, uint8 function)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	uint8 bus;
	uint8 domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	// try MSI-X
	result = pci_disable_msix(device);
	if (result != B_UNSUPPORTED && result != B_NO_INIT)
		return result;

	msi_info *info =  &device->msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// disable HT msi mapping (if applicable)
	pci_ht_msi_map(device, 0);

	// disable msi generation
	info->control_value &= ~PCI_msi_control_enable;
	gPCI->WriteConfig(device, info->capability_offset + PCI_msi_control, 2,
		info->control_value);

	return B_OK;
}


void
pci_read_msi_info(PCIDev *device)
{
	if (!msi_supported())
		return;

	msi_info *info = &device->msi;
	info->msi_capable = false;
	status_t result = gPCI->FindCapability(device->domain, device->bus,
		device->device, device->function, PCI_cap_id_msi,
		&info->capability_offset);
	if (result != B_OK)
		return;

	info->msi_capable = true;
	info->control_value = gPCI->ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msi_control, 2);
	info->message_count
		= 1 << ((info->control_value & PCI_msi_control_mmc_mask) >> 1);
	info->configured_count = 0;
	info->data_value = 0;
	info->address_value = 0;
}


uint8
pci_get_msix_count(uint8 virtualBus, uint8 _device, uint8 function)
{
	if (!msi_supported())
		return 0;

	uint8 bus;
	uint8 domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return 0;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return 0;

	msix_info *info = &device->msix;
	if (!info->msix_capable)
		return 0;

	return info->message_count;
}


status_t
pci_configure_msix(uint8 virtualBus, uint8 _device, uint8 function,
	uint8 count, uint8 *startVector)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	if (count == 0 || startVector == NULL)
		return B_BAD_VALUE;

	uint8 bus;
	uint8 domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msix_info *info =  &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (count > 32 || count > info->message_count) {
		return B_BAD_VALUE;
	}

	if (info->configured_count != 0)
		return B_BUSY;

	// map the table bar
	size_t tableSize = info->message_count * 16;
	addr_t address;
	phys_addr_t barAddr = device->info.u.h0.base_registers[info->table_bar];
	uchar flags = device->info.u.h0.base_register_flags[info->table_bar];
	if ((flags & PCI_address_type) == PCI_address_type_64) {
		barAddr |= (uint64)device->info.u.h0.base_registers[
			info->table_bar + 1] << 32;
	}
	area_id area = map_physical_memory("msi table map",
		barAddr, tableSize + info->table_offset,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&address);
	if (area < 0)
		return area;
	info->table_area_id = area;
	info->table_address = address + info->table_offset;

	// and the pba bar if necessary
	if (info->table_bar != info->pba_bar) {
		barAddr = device->info.u.h0.base_registers[info->pba_bar];
		flags = device->info.u.h0.base_register_flags[info->pba_bar];
		if ((flags & PCI_address_type) == PCI_address_type_64) {
			barAddr |= (uint64)device->info.u.h0.base_registers[
				info->pba_bar + 1] << 32;
		}
		area = map_physical_memory("msi pba map",
			barAddr, tableSize + info->pba_offset,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(void**)&address);
		if (area < 0) {
			delete_area(info->table_area_id);
			info->table_area_id = -1;
			return area;
		}
		info->pba_area_id = area;
	} else
		info->pba_area_id = -1;
	info->pba_address = address + info->pba_offset;

	result = msi_allocate_vectors(count, &info->start_vector,
		&info->address_value, &info->data_value);
	if (result != B_OK) {
		delete_area(info->pba_area_id);
		delete_area(info->table_area_id);
		info->pba_area_id = -1;
		info->table_area_id = -1;
		return result;
	}

	// ensure the memory i/o is enabled
	gPCI->WriteConfig(device, PCI_command, 2,
		gPCI->ReadConfig(device, PCI_command, 2) | PCI_command_memory);

	uint32 data_value = info->data_value;
	for (uint32 index = 0; index < count; index++) {
		volatile uint32 *entry = (uint32*)(info->table_address + 16 * index);
		*(entry + 3) |= PCI_msix_vctrl_mask;
		*entry++ = info->address_value & 0xffffffff;
		*entry++ = info->address_value >> 32;
		*entry++ = data_value++;
		*entry &= ~PCI_msix_vctrl_mask;
	}

	info->configured_count = count;
	*startVector = info->start_vector;
	dprintf("msix configured for %d vectors\n", count);
	return B_OK;
}


static status_t
pci_unconfigure_msix(PCIDev *device)
{
	msix_info *info =  &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// disable msi-x generation
	info->control_value &= ~PCI_msix_control_enable;
	gPCI->WriteConfig(device, info->capability_offset + PCI_msix_control, 2,
		info->control_value);

	msi_free_vectors(info->configured_count, info->start_vector);
	for (uint8 index = 0; index < info->configured_count; index++) {
		volatile uint32 *entry = (uint32*)(info->table_address + 16 * index);
		if ((*(entry + 3) & PCI_msix_vctrl_mask) == 0)
			*(entry + 3) |= PCI_msix_vctrl_mask;
	}

	if (info->pba_area_id != -1)
		delete_area(info->pba_area_id);
	if (info->table_area_id != -1)
		delete_area(info->table_area_id);
	info->pba_area_id= -1;
	info->table_area_id = -1;

	info->configured_count = 0;
	info->address_value = 0;
	info->data_value = 0;
	return B_OK;
}


status_t
pci_enable_msix(uint8 virtualBus, uint8 _device, uint8 function)
{
	if (!msi_supported())
		return B_UNSUPPORTED;

	uint8 bus;
	uint8 domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msix_info *info =  &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// ensure the pinned interrupt is disabled
	gPCI->WriteConfig(device, PCI_command, 2,
		gPCI->ReadConfig(device, PCI_command, 2) | PCI_command_int_disable);

	// enable msi-x generation
	info->control_value |= PCI_msix_control_enable;
	gPCI->WriteConfig(device, info->capability_offset + PCI_msix_control, 2,
		info->control_value);

	// enable HT msi mapping (if applicable)
	pci_ht_msi_map(device, info->address_value);

	dprintf("msi-x enabled: 0x%04" B_PRIx32 "\n",
		gPCI->ReadConfig(device, info->capability_offset + PCI_msix_control, 2));
	return B_OK;
}


status_t
pci_disable_msix(PCIDev *device)
{
	msix_info *info =  &device->msix;
	if (!info->msix_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

	// disable HT msi mapping (if applicable)
	pci_ht_msi_map(device, 0);

	// disable msi-x generation
	info->control_value &= ~PCI_msix_control_enable;
	gPCI->WriteConfig(device, info->capability_offset + PCI_msix_control, 2,
		info->control_value);

	return B_OK;
}


void
pci_read_msix_info(PCIDev *device)
{
	if (!msi_supported())
		return;

	msix_info *info = &device->msix;
	info->msix_capable = false;
	status_t result = gPCI->FindCapability(device->domain, device->bus,
		device->device, device->function, PCI_cap_id_msix,
		&info->capability_offset);
	if (result != B_OK)
		return;

	info->msix_capable = true;
	info->control_value = gPCI->ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msix_control, 2);
	info->message_count
		= (info->control_value & PCI_msix_control_table_size) + 1;
	info->configured_count = 0;
	info->data_value = 0;
	info->address_value = 0;
	info->table_area_id = -1;
	info->pba_area_id = -1;
	uint32 table_value = gPCI->ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msix_table, 4);
	uint32 pba_value = gPCI->ReadConfig(device->domain, device->bus,
		device->device, device->function,
		info->capability_offset + PCI_msix_pba, 4);

	info->table_bar = table_value & PCI_msix_bir_mask;
	info->table_offset = table_value & PCI_msix_offset_mask;
	info->pba_bar = pba_value & PCI_msix_bir_mask;
	info->pba_offset = pba_value & PCI_msix_offset_mask;
}
