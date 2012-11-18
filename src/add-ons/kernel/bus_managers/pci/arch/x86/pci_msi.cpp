/*
 *	Copyright 2010, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */

#include "pci_msi.h"
#include "pci_arch_info.h"
#include "pci.h"
#include "pci_private.h"

#include <arch/x86/msi.h>
#include <debug.h>

extern PCI *gPCI;


uint8
pci_get_msi_count(uint8 virtualBus, uint8 _device, uint8 function)
{
	if (!msi_supported())
		return 0;

	uint8 bus;
	int domain;
	if (gPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return 0;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return 0;

	msi_info *info = &device->arch_info.msi;
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
	int domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msi_info *info =  &device->arch_info.msi;
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
	int domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msi_info *info =  &device->arch_info.msi;
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
	int domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msi_info *info =  &device->arch_info.msi;
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
	int domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	PCIDev *device = gPCI->FindDevice(domain, bus, _device, function);
	if (device == NULL)
		return B_ERROR;

	msi_info *info =  &device->arch_info.msi;
	if (!info->msi_capable)
		return B_UNSUPPORTED;

	if (info->configured_count == 0)
		return B_NO_INIT;

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

	msi_info *info = &device->arch_info.msi;
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
