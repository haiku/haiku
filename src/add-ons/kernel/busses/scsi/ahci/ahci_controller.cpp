/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_controller.h"

#include <KernelExport.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIController::AHCIController(device_node_handle node, pci_device_info *device)
	: fNode(node)
	, fPCIDevice(device)
	, fDevicePresentMask(0)
{
	fDevicePresentMask = (1 << 7) | (1 << 19);

	uint16 vendorID = gPCI->read_pci_config(fPCIDevice, PCI_vendor_id, 2);
	uint16 deviceID = gPCI->read_pci_config(fPCIDevice, PCI_device_id, 2);

	TRACE("AHCIController::AHCIController vendor %04x, device %04x\n",
		vendorID, deviceID);

	uchar capabilityOffset;
	status_t res = gPCI->find_pci_capability(fPCIDevice, PCI_cap_id_sata,
		&capabilityOffset);
	if (res == B_OK) {
		uint32 satacr0;
		uint32 satacr1;
		TRACE("PCI SATA capability found at offset 0x%x\n", capabilityOffset);
		satacr0 = gPCI->read_pci_config(fPCIDevice, capabilityOffset, 4);
		satacr1 = gPCI->read_pci_config(fPCIDevice, capabilityOffset + 4, 4);
		TRACE("satacr0 = 0x%08lx, satacr1 = 0x%08lx\n", satacr0, satacr1);
	}
}


AHCIController::~AHCIController()
{
}


void
AHCIController::ExecuteRequest(scsi_ccb *request)
{
	if (request->target_lun || !IsDevicePresent(request->target_id)) {
		request->subsys_status = SCSI_DEV_NOT_THERE;
		gSCSI->finished(request, 1);
		return;
	}

	TRACE("AHCIController::ExecuteRequest opcode %u, length %u\n", request->cdb[0], request->cdb_length);

	request->subsys_status = SCSI_DEV_NOT_THERE;
	gSCSI->finished(request, 1);
	return;

	request->subsys_status = SCSI_REQ_CMP;
	gSCSI->finished(request, 1);
}


uchar
AHCIController::AbortRequest(scsi_ccb *request)
{
	if (request->target_lun || !IsDevicePresent(request->target_id))
		return SCSI_DEV_NOT_THERE;

	return SCSI_REQ_CMP;
}


uchar
AHCIController::TerminateRequest(scsi_ccb *request)
{
	if (request->target_lun || !IsDevicePresent(request->target_id))
		return SCSI_DEV_NOT_THERE;

	return SCSI_REQ_CMP;
}


uchar
AHCIController::ResetDevice(uchar targetID, uchar targetLUN)
{
	if (targetLUN || !IsDevicePresent(targetID))
		return SCSI_DEV_NOT_THERE;

	return SCSI_REQ_CMP;
}

