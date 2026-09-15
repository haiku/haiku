/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamza Dridi, <dridiha98@gmail.com>
 */

#include "ufshci.h"

#include <KernelExport.h>

#include <bus/PCI.h>
#include <bus/SCSI.h>
#include <string.h>

extern device_manager_info* gDeviceManager;
extern scsi_for_sim_interface* gSCSI;


static void
ufshci_set_scsi_bus(scsi_sim_cookie cookie, scsi_bus bus)
{
	CALLED();
	static_cast<UfsHciController*>(cookie)->SetBus(bus);
}


static void
ufshci_scsi_io(scsi_sim_cookie cookie, scsi_ccb* request)
{
	CALLED();
	static_cast<UfsHciController*>(cookie)->ExecuteRequest(request);
}


static uchar
ufshci_abort_io(scsi_sim_cookie cookie, scsi_ccb* request)
{
	CALLED();
	return SCSI_REQ_CMP;
}


static uchar
ufshci_reset_device(scsi_sim_cookie cookie, uchar targetID, uchar targetLUN)
{
	CALLED();
	return SCSI_REQ_CMP;
}


static uchar
ufshci_terminate_io(scsi_sim_cookie cookie, scsi_ccb* request)
{
	CALLED();
	return SCSI_REQ_CMP;
}


static uchar
ufshci_path_inquiry(scsi_sim_cookie cookie, scsi_path_inquiry* info)
{
	CALLED();
	return static_cast<UfsHciController*>(cookie)->PathInquiry(info);
}


static uchar
ufshci_scan_bus(scsi_sim_cookie cookie)
{
	CALLED();
	return SCSI_REQ_CMP;
}


static uchar
ufshci_reset_bus(scsi_sim_cookie cookie)
{
	CALLED();
	return SCSI_REQ_CMP;
}


static void
ufshci_get_restrictions(scsi_sim_cookie cookie, uchar targetID, bool* isATAPI, bool* noAutoSense,
	uint32* maxBlocks)
{
	CALLED();
	static_cast<UfsHciController*>(cookie)->GetRestrictions(isATAPI, noAutoSense, maxBlocks);
}


static status_t
ufshci_ioctl(scsi_sim_cookie cookie, uint8 targetID, uint32 op, void* buffer, size_t length)
{
	CALLED();
	return B_DEV_INVALID_IOCTL;
}


static status_t
ufshci_sim_init_bus(device_node* node, void** _cookie)
{

	CALLED();

	device_node* parent = gDeviceManager->get_parent_node(node);
	device_node* pciParent = gDeviceManager->get_parent_node(parent);
	pci_device* pci_dev;
	pci_device_module_info* pci;

	status_t status
		= gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci, (void**)&pci_dev);

	if (status != B_OK) {
		TRACE("init_driver:: Error getting parent driver: %s\n", strerror(status));
		return status;
	}

	struct pci_info pci_device_info;
	pci->get_pci_info(pci_dev, &pci_device_info);

	UfsHciController* bus = new(std::nothrow) UfsHciController(pci, pci_dev, &pci_device_info);

	if (bus == NULL)
		return B_NO_MEMORY;

	status = bus->Init();

	if (status < B_OK) {
		delete bus;
		return status;
	}

	*_cookie = bus;

	return B_OK;
}


static void
ufshci_sim_uninit_bus(void* cookie)
{
	CALLED();

	UfsHciController* controller = static_cast<UfsHciController*>(cookie);

	delete controller;
}


static void
ufshci_sim_bus_removed(void* cookie)
{
	CALLED();
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


scsi_sim_interface gUfsHciSimInterface = {
	{
		{
			UFSHCI_SIM_MODULE_NAME,
			0,
			std_ops
		},

		NULL, // supported devices
		NULL, // register node
		ufshci_sim_init_bus,
		ufshci_sim_uninit_bus,
		NULL, // register child devices
		NULL, // rescan
		ufshci_sim_bus_removed
	},

	ufshci_set_scsi_bus,
	ufshci_scsi_io,
	ufshci_abort_io,
	ufshci_reset_device,
	ufshci_terminate_io,
	ufshci_path_inquiry,
	ufshci_scan_bus,
	ufshci_reset_bus,
	ufshci_get_restrictions,
	ufshci_ioctl
};
