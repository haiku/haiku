/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioSCSIPrivate.h"

#include <new>
#include <stdlib.h>
#include <string.h>


#define VIRTIO_SCSI_ID_GENERATOR "virtio_scsi/id"
#define VIRTIO_SCSI_ID_ITEM "virtio_scsi/id"
#define VIRTIO_SCSI_BRIDGE_PRETTY_NAME "Virtio SCSI Bridge"
#define VIRTIO_SCSI_CONTROLLER_PRETTY_NAME "Virtio SCSI Controller"

#define VIRTIO_SCSI_DEVICE_MODULE_NAME "busses/scsi/virtio_scsi/driver_v1"
#define VIRTIO_SCSI_SIM_MODULE_NAME "busses/scsi/virtio_scsi/sim/driver_v1"


device_manager_info *gDeviceManager;
scsi_for_sim_interface *gSCSI;


//	#pragma mark - SIM module interface


static void
set_scsi_bus(scsi_sim_cookie cookie, scsi_bus bus)
{
	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	sim->SetBus(bus);
}


static void
scsi_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	CALLED();
	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	if (sim->ExecuteRequest(request) == B_BUSY)
		gSCSI->requeue(request, true);
}


static uchar
abort_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	CALLED();
	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	return sim->AbortRequest(request);
}


static uchar
reset_device(scsi_sim_cookie cookie, uchar targetID, uchar targetLUN)
{
	CALLED();
	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	return sim->ResetDevice(targetID, targetLUN);
}


static uchar
terminate_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	CALLED();
	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	return sim->TerminateRequest(request);
}


static uchar
path_inquiry(scsi_sim_cookie cookie, scsi_path_inquiry *info)
{
	CALLED();

	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	if (sim->Bus() == NULL)
		return SCSI_NO_HBA;

	sim->PathInquiry(info);
	return SCSI_REQ_CMP;
}


//! this is called immediately before the SCSI bus manager scans the bus
static uchar
scan_bus(scsi_sim_cookie cookie)
{
	CALLED();

	return SCSI_REQ_CMP;
}


static uchar
reset_bus(scsi_sim_cookie cookie)
{
	CALLED();

	return SCSI_REQ_CMP;
}


/*!	Get restrictions of one device
	(used for non-SCSI transport protocols and bug fixes)
*/
static void
get_restrictions(scsi_sim_cookie cookie, uchar targetID, bool *isATAPI,
	bool *noAutoSense, uint32 *maxBlocks)
{
	CALLED();
	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	sim->GetRestrictions(targetID, isATAPI, noAutoSense, maxBlocks);
}


static status_t
ioctl(scsi_sim_cookie cookie, uint8 targetID, uint32 op, void *buffer,
	size_t length)
{
	CALLED();
	VirtioSCSIController *sim = (VirtioSCSIController *)cookie;
	return sim->Control(targetID, op, buffer, length);
}


//	#pragma mark -


static status_t
sim_init_bus(device_node *node, void **_cookie)
{
	CALLED();

	VirtioSCSIController *controller =  new(std::nothrow)
		VirtioSCSIController(node);
	if (controller == NULL)
		return B_NO_MEMORY;
	status_t status = controller->InitCheck();
	if (status < B_OK) {
		delete controller;
		return status;
	}

	*_cookie = controller;
	return B_OK;
}


static void
sim_uninit_bus(void *cookie)
{
	CALLED();
	VirtioSCSIController *controller = (VirtioSCSIController*)cookie;

	delete controller;
}


//	#pragma mark -


static float
virtio_scsi_supports_device(device_node *parent)
{
	const char *bus;
	uint16 deviceType;

	// make sure parent is really the Virtio bus manager
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "virtio"))
		return 0.0;

	// check whether it's really a Virtio SCSI Device
	if (gDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK || deviceType != VIRTIO_DEVICE_ID_SCSI)
		return 0.0;

	TRACE("Virtio SCSI device found!\n");

	return 0.6f;
}


static status_t
virtio_scsi_register_device(device_node *parent)
{
	CALLED();
	virtio_device_interface* virtio = NULL;
	virtio_device* virtioDevice = NULL;
	struct virtio_scsi_config config;

	gDeviceManager->get_driver(parent, (driver_module_info **)&virtio,
		(void **)&virtioDevice);

	status_t status = virtio->read_device_config(virtioDevice, 0, &config,
		sizeof(struct virtio_scsi_config));
	if (status != B_OK)
		return status;

	uint32 max_targets = config.max_target + 1;
	uint32 max_luns = config.max_lun + 1;
	uint32 max_blocks = 0x10000;
	if (config.max_sectors != 0)
		max_blocks = config.max_sectors;

	device_attr attrs[] = {
		{ SCSI_DEVICE_MAX_TARGET_COUNT, B_UINT32_TYPE,
			{ ui32: max_targets }},
		{ SCSI_DEVICE_MAX_LUN_COUNT, B_UINT32_TYPE,
			{ ui32: max_luns }},
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: VIRTIO_SCSI_BRIDGE_PRETTY_NAME }},

		// DMA properties
		{ B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE, { ui32: max_blocks }},
		{ B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE,
			{ ui32: config.seg_max }},
		{ NULL }
	};

	return gDeviceManager->register_node(parent, VIRTIO_SCSI_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_scsi_init_driver(device_node *node, void **_cookie)
{
	CALLED();
	*_cookie = node;
	return B_OK;
}


static status_t
virtio_scsi_register_child_devices(void *cookie)
{
	CALLED();
	device_node *node = (device_node *)cookie;

	int32 id = gDeviceManager->create_id(VIRTIO_SCSI_ID_GENERATOR);
	if (id < 0)
		return id;

	device_attr attrs[] = {
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ string: SCSI_FOR_SIM_MODULE_NAME }},
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: VIRTIO_SCSI_CONTROLLER_PRETTY_NAME }},
		{ SCSI_DESCRIPTION_CONTROLLER_NAME, B_STRING_TYPE,
			{ string: VIRTIO_SCSI_DEVICE_MODULE_NAME }},
		{ B_DMA_MAX_TRANSFER_BLOCKS, B_UINT32_TYPE, { ui32: 255 }},
		{ VIRTIO_SCSI_ID_ITEM, B_UINT32_TYPE, { ui32: (uint32)id }},
			{ NULL }
	};

	status_t status = gDeviceManager->register_node(node,
		VIRTIO_SCSI_SIM_MODULE_NAME, attrs, NULL, NULL);
	if (status < B_OK)
		gDeviceManager->free_id(VIRTIO_SCSI_ID_GENERATOR, id);

	return status;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


static scsi_sim_interface sVirtioSCSISimInterface = {
	{
		{
			VIRTIO_SCSI_SIM_MODULE_NAME,
			0,
			std_ops
		},
		NULL,	// supported devices
		NULL,	// register node
		sim_init_bus,
		sim_uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		NULL	// bus_removed
	},
	set_scsi_bus,
	scsi_io,
	abort_io,
	reset_device,
	terminate_io,
	path_inquiry,
	scan_bus,
	reset_bus,
	get_restrictions,
	ioctl
};


static driver_module_info sVirtioSCSIDevice = {
	{
		VIRTIO_SCSI_DEVICE_MODULE_NAME,
		0,
		std_ops
	},
	virtio_scsi_supports_device,
	virtio_scsi_register_device,
	virtio_scsi_init_driver,
	NULL,	// uninit_driver,
	virtio_scsi_register_child_devices,
	NULL,	// rescan
	NULL,	// device_removed
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info **)&gSCSI },
	{}
};


module_info *modules[] = {
	(module_info *)&sVirtioSCSIDevice,
	(module_info *)&sVirtioSCSISimInterface,
	NULL
};
