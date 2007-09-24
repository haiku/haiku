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
AHCIPort::Init1()
{
	TRACE("AHCIPort::Init1 port %d\n", fIndex);

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

	// clear IRQ status bits
	fRegs->is = fRegs->is;

	// clear error bits
//	fRegs->serr = fRegs->serr;
	fRegs->serr = 0xffffffff;

	// spin up device
	fRegs->cmd |= PORT_CMD_SUD;

	// activate link
	fRegs->cmd = (fRegs->cmd & ~PORT_CMD_ICC_MASK) | PORT_CMD_ICC_ACTIVE;
	
	// enable FIS receive
	fRegs->cmd |= PORT_CMD_FER;

	FlushPostedWrites();

	return B_OK;
}


// called with global interrupts enabled
status_t
AHCIPort::Init2()
{
	TRACE("AHCIPort::Init2 port %d\n", fIndex);

	// start DMA engine
	fRegs->cmd |= PORT_CMD_ST;

	// enable interrupts
	fRegs->ie = PORT_INT_MASK;

	FlushPostedWrites();

//	if (fRegs->sig == 0xffffffff)
		ResetDevice();

	PostResetDevice();

	TRACE("ie   0x%08lx\n", fRegs->ie);
	TRACE("is   0x%08lx\n", fRegs->is);
	TRACE("cmd  0x%08lx\n", fRegs->cmd);
	TRACE("ssts 0x%08lx\n", fRegs->ssts);
	TRACE("sctl 0x%08lx\n", fRegs->sctl);
	TRACE("serr 0x%08lx\n", fRegs->serr);
	TRACE("sact 0x%08lx\n", fRegs->sact);

	return B_OK;
}


void		
AHCIPort::Uninit()
{
	TRACE("AHCIPort::Uninit port %d\n", fIndex);

	// disable FIS receive
	fRegs->cmd &= ~PORT_CMD_FER;

	// wait for receive completition, up to 500ms
	if (wait_until_clear(&fRegs->cmd, PORT_CMD_FR, 500000) < B_OK) {
		TRACE("AHCIPort::Uninit port %d error FIS rx still running\n", fIndex);
	}

	// stop DMA engine
	fRegs->cmd &= ~PORT_CMD_ST;

	// wait for DMA completition
	if (wait_until_clear(&fRegs->cmd, PORT_CMD_CR, 500000) < B_OK) {
		TRACE("AHCIPort::Uninit port %d error DMA engine still running\n", fIndex);
	}

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



status_t
AHCIPort::ResetDevice()
{
	TRACE("AHCIPort::ResetDevice port %d\n", fIndex);

	// stop DMA engine
	fRegs->cmd &= ~PORT_CMD_ST;
	FlushPostedWrites();

	if (wait_until_clear(&fRegs->cmd, PORT_CMD_CR, 500000) < B_OK) {
		TRACE("AHCIPort::ResetDevice port %d error DMA engine doesn't stop\n", fIndex);
	}

	// perform a hard reset
	fRegs->sctl = (fRegs->sctl & ~0xf) | 1;
	FlushPostedWrites();
	snooze(10000);
	fRegs->sctl &= ~0xf;
	FlushPostedWrites();

	if (wait_until_set(&fRegs->ssts, 0x1, 6000000) < B_OK) {
		TRACE("AHCIPort::ResetDevice port %d no device detected\n", fIndex);
	}

	// clear error bits
//	fRegs->serr = fRegs->serr;
	fRegs->serr = 0xffffffff;
	FlushPostedWrites();

	// start DMA engine
	fRegs->cmd |= PORT_CMD_ST;
	FlushPostedWrites();

	return B_OK;
}


status_t
AHCIPort::PostResetDevice()
{
	TRACE("AHCIPort::PostResetDevice port %d\n", fIndex);

	TRACE("tfd 1 0x%08lx\n", fRegs->tfd);

	// wait for DMA idle ?
//	if (wait_until_clear(&fRegs->cmd, PORT_CMD_CR, 1000000) < B_OK) {
//		TRACE("AHCIPort::PostResetDevice port %d error DMA engine doesn't stop\n", fIndex);
//	}

	switch (fRegs->tfd & 0xff) {
		case 0x7f:
			TRACE("no device present?\n");
			break;
		case 0xff:
			TRACE("invalid task file status 0xff\n");
			// fall through
		default:
			TRACE("waiting...\n");
			wait_until_clear(&fRegs->tfd, ATA_BSY | ATA_DRQ, 31000000);
	}

	if (fRegs->sig == 0xeb140101)
		fRegs->cmd |= PORT_CMD_ATAPI;
	else
		fRegs->cmd &= ~PORT_CMD_ATAPI;
	FlushPostedWrites();

	TRACE("device signature 0x%08lx (%s)\n", fRegs->sig,
		(fRegs->sig == 0xeb140101) ? "ATAPI" : (fRegs->sig == 0x00000101) ? "ATA" : "unknown");

	TRACE("tfd 0x%08lx\n", fRegs->tfd);
	TRACE("device detection: 0x%lx\n", fRegs->ssts & 0xf);

	return B_OK;
}


void
AHCIPort::Interrupt()
{
	uint32 is = fRegs->is;
	TRACE("AHCIPort::Interrupt port %d, status %#08lx\n", fIndex, is);

	// clear interrupts
	fRegs->is = is;
}


void
AHCIPort::ScsiExecuteRequest(scsi_ccb *request)
{

	TRACE("AHCIPort::ScsiExecuteRequest port %d, opcode %u, length %u\n", fIndex, request->cdb[0], request->cdb_length);


	request->subsys_status = SCSI_DEV_NOT_THERE;
	gSCSI->finished(request, 1);
	return;

	request->subsys_status = SCSI_REQ_CMP;
	gSCSI->finished(request, 1);
}


uchar
AHCIPort::ScsiAbortRequest(scsi_ccb *request)
{

	return SCSI_REQ_CMP;
}


uchar
AHCIPort::ScsiTerminateRequest(scsi_ccb *request)
{
	return SCSI_REQ_CMP;
}


uchar
AHCIPort::ScsiResetDevice()
{
	return SCSI_REQ_CMP;
}
