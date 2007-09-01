/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_port.h"
#include "ahci_controller.h"

#include <KernelExport.h>
#include <stdio.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIPort::AHCIPort(AHCIController *controller, int index)
	: fIndex(index)
	, fRegs(&controller->fRegs->port[index])
{
}
				

AHCIPort::~AHCIPort()
{
}

	
status_t
AHCIPort::Init()
{
	TRACE("AHCIPort::Init port %d\n", fIndex);
	return B_OK;
}


void		
AHCIPort::Uninit()
{
	TRACE("AHCIPort::Uninit port %d\n", fIndex);
}


void
AHCIPort::Interrupt()
{
	TRACE("AHCIPort::Interrupt port %d\n", fIndex);
}


void
AHCIPort::ExecuteRequest(scsi_ccb *request)
{

	TRACE("AHCIPort::ExecuteRequest port %d, opcode %u, length %u\n", fIndex, request->cdb[0], request->cdb_length);

	request->subsys_status = SCSI_DEV_NOT_THERE;
	gSCSI->finished(request, 1);
	return;

	request->subsys_status = SCSI_REQ_CMP;
	gSCSI->finished(request, 1);
}


uchar
AHCIPort::AbortRequest(scsi_ccb *request)
{

	return SCSI_REQ_CMP;
}


uchar
AHCIPort::TerminateRequest(scsi_ccb *request)
{
	return SCSI_REQ_CMP;
}


uchar
AHCIPort::ResetDevice()
{
	return SCSI_REQ_CMP;
}

