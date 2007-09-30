/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_controller.h"

#include <KernelExport.h>
#include <string.h>
#include <new>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
//#define FLOW(a...)	dprintf("ahci: " a)
#define FLOW(a...)


pci_device_module_info *gPCI;
scsi_for_sim_interface *gSCSI;


//	#pragma mark - SIM module interface


//! execute request
static void
ahci_scsi_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	FLOW("ahci_scsi_io, cookie %p, path_id %u, target_id %u, target_lun %u\n", 
		cookie, request->path_id, request->target_id, request->target_lun);
	static_cast<AHCIController *>(cookie)->ExecuteRequest(request);
}


//! abort request
static uchar
ahci_abort_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	TRACE("ahci_abort_io, cookie %p\n", cookie);
	return static_cast<AHCIController *>(cookie)->AbortRequest(request);
}


static uchar
ahci_reset_device(scsi_sim_cookie cookie, uchar targetID, uchar targetLUN)
{
	TRACE("ahci_reset_device, cookie %p\n", cookie);
	return static_cast<AHCIController *>(cookie)->ResetDevice(targetID, targetLUN);
}


//! terminate request
static uchar
ahci_terminate_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	TRACE("ahci_terminate_io, cookie %p\n", cookie);
	return static_cast<AHCIController *>(cookie)->TerminateRequest(request);
}


//! get information about bus
static uchar
ahci_path_inquiry(scsi_sim_cookie cookie, scsi_path_inquiry *info)
{
	TRACE("ahci_path_inquiry, cookie %p\n", cookie);

	memset(info, 0, sizeof(*info));
	info->version_num = 1;
	// supports tagged requests and soft reset
	info->hba_inquiry = 0; // SCSI_PI_TAG_ABLE | SCSI_PI_SOFT_RST;
	// controller is 32, devices are 0 to 31
	info->initiator_id = 32;
	// adapter command queue size
	info->hba_queue_size = 1;

	return SCSI_REQ_CMP;
}


//! this is called immediately before the SCSI bus manager scans the bus
static uchar
ahci_scan_bus(scsi_sim_cookie cookie)
{
	TRACE("ahci_scan_bus, cookie %p\n", cookie);

	return SCSI_REQ_CMP;
}


static uchar
ahci_reset_bus(scsi_sim_cookie cookie)
{
	TRACE("ahci_reset_bus, cookie %p\n", cookie);

	return SCSI_REQ_CMP;
}


/*!	Get restrictions of one device 
	(used for non-SCSI transport protocols and bug fixes)
*/
static void
ahci_get_restrictions(scsi_sim_cookie cookie, uchar targetID, bool *isATAPI,
	bool *noAutoSense, uint32 *maxBlocks)
{
	TRACE("ahci_get_restrictions, cookie %p\n", cookie);

	*isATAPI = false;
	*noAutoSense = false;
	*maxBlocks = 256;
}


static status_t
ahci_ioctl(scsi_sim_cookie cookie, uint8 targetID, uint32 op, void *buffer,
	size_t length)
{
	TRACE("ahci_ioctl, cookie %p\n", cookie);
	return B_BAD_VALUE;
}


//	#pragma mark -


static status_t
ahci_sim_init_bus(device_node_handle node, void *userCookie, void **_cookie)
{
	pci_device_info *pciDevice;
	device_node_handle parent;
	AHCIController *controller;
	status_t status;

	TRACE("ahci_sim_init_bus, userCookie %p\n", userCookie);

	// initialize parent (the bus) to get the PCI interface and device
	parent = gDeviceManager->get_parent(node);
	status = gDeviceManager->init_driver(parent, &pciDevice, NULL, NULL);
	gDeviceManager->put_device_node(parent);
	if (status != B_OK)
		return status;

	controller =  new(std::nothrow) AHCIController(node, pciDevice);
	if (!controller)
		return B_NO_MEMORY;
	status = controller->Init();
	if (status < B_OK) {
		delete controller;
		return status;
	}

	*_cookie = controller;
	TRACE("cookie = %p\n", *_cookie);
	return B_OK;
}


static status_t
ahci_sim_uninit_bus(void *cookie)
{
	TRACE("ahci_sim_uninit_bus, cookie %p\n", cookie);
	AHCIController *controller = static_cast<AHCIController *>(cookie);

	device_node_handle parent = gDeviceManager->get_parent(
		controller->DeviceNode());

	controller->Uninit();
	delete controller;

	gDeviceManager->uninit_driver(parent);
	gDeviceManager->put_device_node(parent);

	return B_OK;
}


static void
ahci_sim_bus_removed(device_node_handle node, void *cookie)
{
	TRACE("ahci_sim_bus_removed, cookie %p\n", cookie);
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


scsi_sim_interface gAHCISimInterface = {
	{
		{
			AHCI_SIM_MODULE_NAME,
			0,
			std_ops
		},
		NULL,	// supported devices				
		NULL,	// register node
		ahci_sim_init_bus,
		ahci_sim_uninit_bus,
		ahci_sim_bus_removed,
		NULL,	// device cleanup
		NULL,	// get supported paths
	},
	ahci_scsi_io,
	ahci_abort_io,
	ahci_reset_device,
	ahci_terminate_io,
	ahci_path_inquiry,
	ahci_scan_bus,
	ahci_reset_bus,
	ahci_get_restrictions,
	ahci_ioctl
};

