/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AHCI_CONTROLLER_H
#define _AHCI_CONTROLLER_H


#include "ahci_defs.h"

class AHCIController
{
public:
				AHCIController(device_node_handle node, pci_device_module_info *pci, scsi_for_sim_interface * scsi);
				~AHCIController();
	

	void		ExecuteRequest(scsi_ccb *request);
	uchar		AbortRequest(scsi_ccb *request);
	uchar		TerminateRequest(scsi_ccb *request);
	uchar		ResetDevice(uchar targetID, uchar targetLUN);


private:
	bool		IsDevicePresent(uint device);


private:
	device_node_handle 			fNode;
	pci_device_module_info *	fPCI;
	scsi_for_sim_interface *	fSCSI;
	uint32						fDevicePresentMask;
};


inline bool
AHCIController::IsDevicePresent(uint device)
{
	return fDevicePresentMask & (1 << device);
}


#endif
