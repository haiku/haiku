/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_port.h"
#include "ahci_controller.h"
#include "util.h"

#include <KernelExport.h>
#include <stdio.h>
#include <string.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIPort::AHCIPort(AHCIController *controller, int index)
	: fIndex(index)
	, fRegs(&controller->fRegs->port[index])
	, fArea(-1)
{
}
				

AHCIPort::~AHCIPort()
{
}

	
status_t
AHCIPort::Init()
{
	TRACE("AHCIPort::Init port %d\n", fIndex);

	size_t size = sizeof(command_list_entry) * COMMAND_LIST_ENTRY_COUNT + sizeof(fis) + sizeof(command_table) + sizeof(prd) * PRD_TABLE_ENTRY_COUNT;

	char *virtAddr;
	char *physAddr;

	fArea = alloc_mem((void **)&virtAddr, (void **)&physAddr, size, 0, "some AHCI port");
	if (fArea < B_OK) {
		TRACE("failed allocating memory for port %d\n", fIndex);
		return fArea;
	}
	memset(virtAddr, 0, size);

	fCommandList = (command_list_entry *)virtAddr;
	virtAddr += sizeof(command_list_entry) * COMMAND_LIST_ENTRY_COUNT;
	fFIS = (fis *)virtAddr;
	virtAddr += sizeof(fis);
	fCommandTable = (command_table *)virtAddr;
	virtAddr += sizeof(command_table);
	fPRDTable = (prd *)virtAddr;
	
	fRegs->clb  = LO32(physAddr);
	fRegs->clbu = HI32(physAddr);
	physAddr += sizeof(command_list_entry) * COMMAND_LIST_ENTRY_COUNT;
	fRegs->fb   = LO32(physAddr);
	fRegs->fbu  = HI32(physAddr);
	physAddr += sizeof(fis);
	fCommandList[0].ctba  = LO32(physAddr);
	fCommandList[0].ctbau = HI32(physAddr);
	// prdt follows after command table

	return B_OK;
}


void		
AHCIPort::Uninit()
{
	TRACE("AHCIPort::Uninit port %d\n", fIndex);

	// disable interrupts
	fRegs->ie = 0;
	
	// clear pending interrupts
	fRegs->is = fRegs->is;
	
	// invalidate DMA addresses
	fRegs->clb  = 0;
	fRegs->clbu = 0;
	fRegs->fb   = 0;
	fRegs->fbu  = 0;

	delete_area(fArea);
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

