/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "HyperVSCSI.h"


device_manager_info* gDeviceManager;
scsi_for_sim_interface* gSCSI;


static status_t
hyperv_scsi_init_bus(device_node* node, void** _driverCookie)
{
	CALLED();

	HyperVSCSI* scsi = new(std::nothrow) HyperVSCSI(node);
	if (scsi == NULL)
		return B_NO_MEMORY;

	status_t status = scsi->InitCheck();
	if (status != B_OK) {
		delete scsi;
		return status;
	}

	*_driverCookie = scsi;
	return B_OK;
}


static void
hyperv_scsi_uninit_bus(void* driverCookie)
{
	CALLED();
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(driverCookie);
	delete scsi;
}


static void
hyperv_scsi_set_scsi_bus(scsi_sim_cookie cookie, scsi_bus bus)
{
	CALLED();
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(cookie);
	scsi->SetBus(bus);
}


static void
hyperv_scsi_scsi_io(scsi_sim_cookie cookie, scsi_ccb* ccb)
{
	CALLED();
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(cookie);
	if (scsi->StartIO(ccb) == B_BUSY)
		gSCSI->requeue(ccb, true);
}


static uchar
hyperv_scsi_abort(scsi_sim_cookie cookie, scsi_ccb* ccb_to_abort)
{
	CALLED();
	return SCSI_REQ_CMP;
}


static uchar
hyperv_scsi_reset_device(scsi_sim_cookie cookie, uchar target_id, uchar target_lun)
{
	CALLED();
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(cookie);
	return scsi->ResetDevice(target_id, target_lun);
}


static uchar
hyperv_scsi_term_io(scsi_sim_cookie cookie, scsi_ccb* ccb_to_terminate)
{
	CALLED();
	return SCSI_REQ_CMP;
}


static uchar
hyperv_scsi_path_inquiry(scsi_sim_cookie cookie, scsi_path_inquiry* inquiry_data)
{
	CALLED();
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(cookie);
	return scsi->PathInquiry(inquiry_data);
}


static uchar
hyperv_scsi_scan_bus(scsi_sim_cookie cookie)
{
	// Function is called prior to a full bus scan
	CALLED();
	return SCSI_REQ_CMP;
}


static uchar
hyperv_scsi_reset_bus(scsi_sim_cookie cookie)
{
	CALLED();
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(cookie);
	return scsi->ResetBus();
}


static void
hyperv_scsi_get_restrictions(scsi_sim_cookie cookie, uchar target_id, bool* is_atapi,
	bool* no_autosense, uint32* max_blocks)
{
	CALLED();

	// SCSI CD-ROM and fixed disks are both supported on Gen2 VMs
	// Always indicate ATAPI for compatibility with both and older versions of Hyper-V
	*is_atapi = true;
	*no_autosense = false;
	*max_blocks = HV_SCSI_MAX_BLOCK_COUNT;
}


static status_t
hyperv_scsi_ioctl(scsi_sim_cookie, uint8 targetID, uint32 op, void* buffer, size_t length)
{
	CALLED();
	return B_DEV_INVALID_IOCTL;
}


static float
hyperv_scsi_supports_device(device_node* parent)
{
	CALLED();

	// Check if parent is the Hyper-V bus manager
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK)
		return -1;

	if (strcmp(bus, HYPERV_BUS_NAME) != 0)
		return 0.0f;

	// Check if parent is a Hyper-V SCSI controller
	const char* type;
	if (gDeviceManager->get_attr_string(parent, HYPERV_DEVICE_TYPE_STRING_ITEM, &type, false)
			!= B_OK)
		return 0.0f;

	// TODO: This driver can support the IDE acceleration device, but as only fixed disks are
	// exposed here, there will need to be some logic either here or in the ATA PCI driver to
	// prevent ATA disks from being handled by the ATA driver
	// Hyper-V exposes two dual channel Intel PIIX4 adapters on Gen1 VMs only
	bool isIDE = false; // strcmp(type, VMBUS_TYPE_IDE) == 0;
	if (strcmp(type, VMBUS_TYPE_SCSI) != 0 && !isIDE)
		return 0.0f;

	TRACE("Hyper-V %s controller found!\n", isIDE ? "IDE" : "SCSI");
	return 0.8f;
}


static status_t
hyperv_scsi_register_device(device_node* parent)
{
	CALLED();

	// Check if parent is a Hyper-V IDE controller as it has different LUN/target limits
	const char* type;
	status_t status = gDeviceManager->get_attr_string(parent, HYPERV_DEVICE_TYPE_STRING_ITEM,
		&type, false);
	if (status != B_OK)
		return status;

	bool isIDE = strcmp(type, VMBUS_TYPE_IDE) == 0;
	device_attr attributes[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = isIDE ? HYPERV_PRETTYNAME_IDE : HYPERV_PRETTYNAME_SCSI }},

		{ SCSI_DEVICE_MAX_TARGET_COUNT, B_UINT32_TYPE,
			{ .ui32 = isIDE ? HV_SCSI_MAX_IDE_DEVICES : HV_SCSI_MAX_SCSI_DEVICES }},
		{ SCSI_DEVICE_MAX_LUN_COUNT, B_UINT32_TYPE,
			{ .ui32 = 1 }},

		{ B_DMA_ALIGNMENT, B_UINT32_TYPE,
			{ .ui32 = 1 }}, // Word alignment
		{ B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE,
			{ .ui32 = HV_PAGE_SIZE }},
		{ B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE,
			{ .ui32 = HV_SCSI_MAX_BUFFER_SEGMENTS }},

		{ NULL }
	};

	return gDeviceManager->register_node(parent, HYPERV_SCSI_DRIVER_MODULE_NAME, attributes, NULL,
		NULL);
}


static status_t
hyperv_scsi_init_driver(device_node* node, void** _driverCookie)
{
	CALLED();
	*_driverCookie = node;
	return B_OK;
}


static status_t
hyperv_scsi_register_child_devices(void* driverCookie)
{
	CALLED();
	device_node* node = reinterpret_cast<device_node*>(driverCookie);

	int32 id = gDeviceManager->create_id(HYPERV_SCSI_ID_GENERATOR);
	if (id < 0)
		return id;

	device_attr attributes[] = {
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ .string = SCSI_FOR_SIM_MODULE_NAME }},
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = HYPERV_SCSI_SIM_PRETTY_NAME }},

		{ SCSI_DESCRIPTION_CONTROLLER_NAME, B_STRING_TYPE,
			{ .string = HYPERV_SCSI_DRIVER_MODULE_NAME }},

		{ HYPERV_SCSI_ID_ITEM, B_UINT32_TYPE,
			{ .ui32 = static_cast<uint32>(id) }},

		{ NULL }
	};

	status_t status = gDeviceManager->register_node(node, HYPERV_SCSI_SIM_MODULE_NAME, attributes,
		NULL, NULL);
	if (status != B_OK)
		gDeviceManager->free_id(HYPERV_SCSI_ID_GENERATOR, id);

	return status;
}


static scsi_sim_interface sHyperVSCSISimInterface = {
	{
		{
			HYPERV_SCSI_SIM_MODULE_NAME,
			0,
			NULL
		},

		NULL,	// supports device
		NULL,	// register device
		hyperv_scsi_init_bus,
		hyperv_scsi_uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan child devices
		NULL,	// device removed
		NULL,	// suspend
		NULL	// resume
	},

	hyperv_scsi_set_scsi_bus,
	hyperv_scsi_scsi_io,
	hyperv_scsi_abort,
	hyperv_scsi_reset_device,
	hyperv_scsi_term_io,
	hyperv_scsi_path_inquiry,
	hyperv_scsi_scan_bus,
	hyperv_scsi_reset_bus,
	hyperv_scsi_get_restrictions,
	hyperv_scsi_ioctl
};


static driver_module_info sHyperVSCSIModule = {
	{
		HYPERV_SCSI_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	hyperv_scsi_supports_device,
	hyperv_scsi_register_device,
	hyperv_scsi_init_driver,
	NULL,	// uninit driver
	hyperv_scsi_register_child_devices,
	NULL,	// rescan child devices
	NULL,	// device removed
	NULL,	// suspend
	NULL	// resume
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info**)&gSCSI },
	{}
};


module_info *modules[] = {
	(module_info*)&sHyperVSCSIModule,
	(module_info*)&sHyperVSCSISimInterface,
	NULL
};
