/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AHCI_PORT_H
#define _AHCI_PORT_H

#include "ahci_defs.h"

class AHCIController;

class AHCIPort {
public:
				AHCIPort(AHCIController *controller, int index);
				~AHCIPort();

	status_t	Init();
	void		Uninit();

	void		Interrupt();


	void		ExecuteRequest(scsi_ccb *request);
	uchar		AbortRequest(scsi_ccb *request);
	uchar		TerminateRequest(scsi_ccb *request);
	uchar		ResetDevice();

private:

private:
	int						fIndex;
	volatile ahci_port *	fRegs;
	area_id					fArea;

	volatile fis *					fFIS;
	volatile command_list_entry *	fCommandList;
	volatile command_table *		fCommandTable;
	volatile prd *					fPRDTable;
};

#endif	// _AHCI_PORT_H
