/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AHCI_CONTROLLER_H
#define _AHCI_CONTROLLER_H


#include "ahci_defs.h"


class AHCIController {
public:
				AHCIController(device_node_handle node,
					pci_device_info *pciDevice);
				~AHCIController();

	status_t	Init();
	void		Uninit();

	void		ExecuteRequest(scsi_ccb *request);
	uchar		AbortRequest(scsi_ccb *request);
	uchar		TerminateRequest(scsi_ccb *request);
	uchar		ResetDevice(uchar targetID, uchar targetLUN);

	device_node_handle DeviceNode() { return fNode; }

private:
	bool		IsDevicePresent(uint device);

private:
	device_node_handle 			fNode;
	pci_device_info*			fPCIDevice;
	uint32						fDevicePresentMask;


// --- Instance check workaround begin
	port_id fInstanceCheck;
// --- Instance check workaround end

};


inline bool
AHCIController::IsDevicePresent(uint device)
{
	return fDevicePresentMask & (1 << device);
}


#endif	// _AHCI_CONTROLLER_H
