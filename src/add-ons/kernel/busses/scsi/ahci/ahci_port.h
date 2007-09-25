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

	status_t	Init1();
	status_t	Init2();
	void		Uninit();

	void		Interrupt();


	void		ScsiExecuteRequest(scsi_ccb *request);
	uchar		ScsiAbortRequest(scsi_ccb *request);
	uchar		ScsiTerminateRequest(scsi_ccb *request);
	uchar		ScsiResetDevice();

private:
	status_t	ResetDevice();
	status_t	PostResetDevice();
	void		FlushPostedWrites();
	void		DumpD2HFis();

private:
	int						fIndex;
	volatile ahci_port *	fRegs;
	area_id					fArea;

	volatile fis *					fFIS;
	volatile command_list_entry *	fCommandList;
	volatile command_table *		fCommandTable;
	volatile prd *					fPRDTable;
};

inline void
AHCIPort::FlushPostedWrites()
{
	volatile uint32 dummy = fRegs->cmd;
	dummy = dummy;
}
#endif	// _AHCI_PORT_H
