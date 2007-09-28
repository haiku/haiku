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
	void		ScsiTestUnitReady(scsi_ccb *request);
	void		ScsiInquiry(scsi_ccb *request);
	void		ScsiReadCapacity(scsi_ccb *request);
	void		ScsiReadWrite(scsi_ccb *request, uint64 position, size_t length, bool isWrite);

	status_t	ResetDevice();
	status_t	PostResetDevice();
	void		FlushPostedWrites();
	void		DumpD2HFis();

	void		StartTransfer();
	status_t	WaitForTransfer(int *status, bigtime_t timeout);
	void		FinishTransfer();


//	uint8 *		SetCommandFis(volatile command_list_entry *cmd, volatile fis *fis, const void *data, size_t dataSize);
	status_t	FillPrdTable(volatile prd *prdTable, int *prdCount, int prdMax, const void *data, size_t dataSize, bool ioc = false);
	status_t	FillPrdTable(volatile prd *prdTable, int *prdCount, int prdMax, const physical_entry *sgTable, int sgCount, bool ioc = false);

private:
	int						fIndex;
	volatile ahci_port *	fRegs;
	area_id					fArea;
	sem_id							fRequestSem;
	sem_id							fResponseSem;
	volatile bool					fCommandActive;

	volatile fis *					fFIS;
	volatile command_list_entry *	fCommandList;
	volatile command_table *		fCommandTable;
	volatile prd *					fPRDTable;
	uint64							fHarddiskSize;
};

inline void
AHCIPort::FlushPostedWrites()
{
	volatile uint32 dummy = fRegs->cmd;
	dummy = dummy;
}
#endif	// _AHCI_PORT_H
