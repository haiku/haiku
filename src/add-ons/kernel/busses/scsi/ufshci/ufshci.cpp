/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamza Dridi, <dridiha98@gmail.com>
 */

#include "ufshci.h"

#include <device_manager.h>
#include <drivers/KernelExport.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>

#include "PCI.h"
#include "SupportDefs.h"
#include "TypeConstants.h"

#include "bus/SCSI.h"
#include "module.h"

device_manager_info* gDeviceManager;
scsi_for_sim_interface* gSCSI;

extern scsi_sim_interface gUfsHciSimInterface;

typedef struct UfsPciId {
	uint16 vendorId;
	uint16 deviceId;
} UfsPciId;

static UfsPciId kSupportedVendors[] = {
	{.vendorId = UFS_PCI_VENDOR_ID_REDHAT, .deviceId = 0x0013},
	{.vendorId = UFS_PCI_VENDOR_ID_SAMSUNG, .deviceId = 0xC00C},
	{.vendorId = UFS_PCI_VENDOR_ID_AMD, .deviceId = 0x1B29},
};

static inline bool
is_vendor_supported(uint16 vendorId, uint16 deviceId)
{
	CALLED();

	for (uint i = 0; i < B_COUNT_OF(kSupportedVendors); i++) {
		if (vendorId == kSupportedVendors[i].vendorId && deviceId == kSupportedVendors[i].deviceId)
			return true;
	}

	return false;
}


static float
supports_device(device_node* parent)
{
	CALLED();

	uint16 deviceId, vendorId;
	const char* bus;
	uint16 type, subType;


	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK)
		return -1.0f;


	if (strcmp(bus, "pci"))
		return -1.0f;

	if (gDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendorId, false) != B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceId, false) != B_OK) {
		return -1.0f;
	}
	if (gDeviceManager->get_attr_uint16(parent, B_DEVICE_TYPE, &type, false) != B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_SUB_TYPE, &subType, false) != B_OK) {
		return -1.0f;
	}

	if (type != PCI_mass_storage || subType != PCI_ufs)
		return -1.0f;

	TRACE("supports_device:: UFSHCI type %d, subtype %d\n", type, subType);
	if (!is_vendor_supported(vendorId, deviceId))
		return -1.0f;

	TRACE("supports_device:: UFSHCI Controller detected\n");
	return 1.0f;
}


static status_t
init_driver(device_node* node, void** cookie)
{
	CALLED();

	*cookie = node;

	return B_OK;
}


static void
uninit_driver(void* _cookie)
{
	CALLED();
}


static status_t
register_device(device_node* node)
{
	CALLED();

	device_attr attrs[] = {{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "ufshci"}},
		{SCSI_DEVICE_MAX_TARGET_COUNT, B_UINT32_TYPE, {.ui32 = UFSHCI_MAX_TARGET_COUNT}},
		{SCSI_DEVICE_MAX_LUN_COUNT, B_UINT32_TYPE, {.ui32 = UFSHCI_MAX_LUN_COUNT}},
		{B_DMA_ALIGNMENT, B_UINT32_TYPE, {.ui32 = UFSHCI_DMA_ALIGNMENT}},
		{B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE, {.ui32 = UFS_UTRLD_MAX_PRDT_ENTRIES}},
		{B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE, {.ui32 = UFS_UTRLD_MAX_PRDT_BLOCKS}}, {NULL}};

	return gDeviceManager->register_node(node, UFSHCI_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
register_child_devices(void* _cookie)
{
	CALLED();

	device_node* parent = (device_node*)_cookie;
	device_attr attrs[] = {
		{B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {.string = SCSI_FOR_SIM_MODULE_NAME}},
		{SCSI_DESCRIPTION_CONTROLLER_NAME, B_STRING_TYPE, {.string = UFSHCI_DRIVER_MODULE_NAME}},

		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "Ufshci Sim Controller"}}, {NULL}};


	return gDeviceManager->register_node(parent, UFSHCI_SIM_MODULE_NAME, attrs, NULL, NULL);
}


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager},
	{SCSI_FOR_SIM_MODULE_NAME, (module_info**)&gSCSI},
	{NULL}
};


static struct driver_module_info sUfsDriver = {
	{UFSHCI_DRIVER_MODULE_NAME, 0, NULL},
	supports_device,
	register_device,
	init_driver,
	uninit_driver,
	register_child_devices,
	NULL,
	NULL
};


module_info* modules[] = {
	(module_info*)&sUfsDriver,
	(module_info*)&gUfsHciSimInterface,
	NULL
};
