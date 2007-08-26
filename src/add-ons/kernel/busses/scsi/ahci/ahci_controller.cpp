/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "ahci_controller.h"

#include <KernelExport.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[30m " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIController::AHCIController(device_node_handle node, pci_device_module_info *pci, scsi_for_sim_interface * scsi)
 :	fNode(node)
 ,	fPCI(pci)
 ,	fSCSI(scsi)
 ,	fDevicePresentMask(0)
{

	fDevicePresentMask = (1 << 7) | (1 << 19);

#if 0
	uchar cap_ofs;
	status_t res = fPCI->find_pci_capability(device, PCI_cap_id_sata, &cap_ofs);
	if (res == B_OK) {
		uint32 satacr0;
		uint32 satacr1;
		TRACE("PCI SATA capability found at offset 0x%x\n", cap_ofs);
		satacr0 = pci->read_pci_config(device, cap_ofs, 4);
		satacr1 = pci->read_pci_config(device, cap_ofs + 4, 4);
		TRACE("satacr0 = 0x%08x, satacr1 = 0x%08x\n", satacr0, satacr1);
	}
#endif
}


AHCIController::~AHCIController()
{
}


void
AHCIController::ExecuteRequest(scsi_ccb *request)
{
	if (request->target_lun || !IsDevicePresent(request->target_id)) {
		request->subsys_status = SCSI_DEV_NOT_THERE;
		fSCSI->finished(request, 1);
		return;
	}

	TRACE("AHCIController::ExecuteRequest opcode %u, length %u\n", request->cdb[0], request->cdb_length);

	request->subsys_status = SCSI_REQ_CMP;
	fSCSI->finished(request, 1);
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

