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

	// disable transitions to partial or slumber state
	fRegs->sctl |= 0x300;

	// clear IRQ status bits
	fRegs->is = fRegs->is;

	// clear error bits
	fRegs->serr = fRegs->serr;

	// power up device
	fRegs->cmd |= PORT_CMD_POD;

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

	if (1 /* fRegs->sig == 0xffffffff */)
		ResetDevice();

	PostResetDevice();

	TRACE("ie   0x%08lx\n", fRegs->ie);
	TRACE("is   0x%08lx\n", fRegs->is);
	TRACE("cmd  0x%08lx\n", fRegs->cmd);
	TRACE("ssts 0x%08lx\n", fRegs->ssts);
	TRACE("sctl 0x%08lx\n", fRegs->sctl);
	TRACE("serr 0x%08lx\n", fRegs->serr);
	TRACE("sact 0x%08lx\n", fRegs->sact);
	TRACE("tfd  0x%08lx\n", fRegs->tfd);

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
	spin(1100);
	fRegs->sctl &= ~0xf;
	FlushPostedWrites();

	if (wait_until_set(&fRegs->ssts, 0x1, 100000) < B_OK) {
		TRACE("AHCIPort::ResetDevice port %d no device detected\n", fIndex);
	}

	// clear error bits
	fRegs->serr = fRegs->serr;
	FlushPostedWrites();

	if (fRegs->ssts & 1) {
		if (wait_until_set(&fRegs->ssts, 0x3, 500000) < B_OK) {
			TRACE("AHCIPort::ResetDevice port %d device present but no phy communication\n", fIndex);
		}
	}

	// clear error bits
	fRegs->serr = fRegs->serr;
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

	if ((fRegs->ssts & 0xf) != 0x3 || (fRegs->tfd & 0xff) == 0x7f) {
		TRACE("AHCIPort::PostResetDevice port %d: no device\n", fIndex);
		return B_OK;
	}

	if ((fRegs->tfd & 0xff) == 0xff)
		snooze(200000);

	if ((fRegs->tfd & 0xff) == 0xff) {
		TRACE("AHCIPort::PostResetDevice port %d: invalid task file status 0xff\n", fIndex);
		return B_ERROR;
	}

	wait_until_clear(&fRegs->tfd, ATA_BSY, 31000000);

	if (fRegs->sig == 0xeb140101)
		fRegs->cmd |= PORT_CMD_ATAPI;
	else
		fRegs->cmd &= ~PORT_CMD_ATAPI;
	FlushPostedWrites();

	TRACE("device signature 0x%08lx (%s)\n", fRegs->sig,
		(fRegs->sig == 0xeb140101) ? "ATAPI" : (fRegs->sig == 0x00000101) ? "ATA" : "unknown");

	return B_OK;
}


void
AHCIPort::DumpD2HFis()
{
	TRACE("D2H FIS:\n");
	TRACE("  DW0  %02x %02x %02x %02x\n", fFIS->rfis[3], fFIS->rfis[2], fFIS->rfis[1], fFIS->rfis[0]);
	TRACE("  DW1  %02x %02x %02x %02x\n", fFIS->rfis[7], fFIS->rfis[6], fFIS->rfis[5], fFIS->rfis[4]);
	TRACE("  DW2  %02x %02x %02x %02x\n", fFIS->rfis[11], fFIS->rfis[10], fFIS->rfis[9], fFIS->rfis[8]);
	TRACE("  DW3  %02x %02x %02x %02x\n", fFIS->rfis[15], fFIS->rfis[14], fFIS->rfis[13], fFIS->rfis[12]);
	TRACE("  DW4  %02x %02x %02x %02x\n", fFIS->rfis[19], fFIS->rfis[18], fFIS->rfis[17], fFIS->rfis[16]);
}


void
AHCIPort::Interrupt()
{
	uint32 is = fRegs->is;
	TRACE("AHCIPort::Interrupt port %d, status %#08lx\n", fIndex, is);

	// clear interrupts
	fRegs->is = is;
}

char c(int i)
{
	if (i == '\n' || i == '\r' || i == '\b')
		return '?';
	return i;
}

void
AHCIPort::IdentifyDevice()
{
	TRACE("AHCIPort::IdentifyDevice port %d\n", fIndex);

	void *phy; 
	uint8 *data;
	int size = 512;

	area_id id = alloc_mem((void **)&data, &phy, size, 0, "identify device");

	TRACE("virt   0x%08lx\n", data);
	TRACE("phys   0x%08lx\n", phy);

	memset(data, 0, size);

	TRACE("ci   0x%08lx\n", fRegs->ci);
	TRACE("ie   0x%08lx\n", fRegs->ie);
	TRACE("is   0x%08lx\n", fRegs->is);
	TRACE("cmd  0x%08lx\n", fRegs->cmd);
	TRACE("ssts 0x%08lx\n", fRegs->ssts);
	TRACE("sctl 0x%08lx\n", fRegs->sctl);
	TRACE("serr 0x%08lx\n", fRegs->serr);
	TRACE("sact 0x%08lx\n", fRegs->sact);
	TRACE("tfd  0x%08lx\n", fRegs->tfd);


	memset((void *)fCommandTable->cfis, 0, 5 * 4);
	fCommandTable->cfis[0] = 0x27;
	fCommandTable->cfis[1] = 0x80;
	fCommandTable->cfis[2] = 0xec;


	fCommandList->prdtl_flags_cfl = 0;
	fCommandList->prdtl = 1;
//	fCommandList->c = 1;
	fCommandList->cfl = 5;
	fCommandList->prdbc = 0;

	TRACE("prdtl_flags_cfl %08x\n", fCommandList->prdtl_flags_cfl);
	TRACE("prdbc           %08x\n", fCommandList->prdbc);


	fPRDTable->dba = LO32(phy);
	fPRDTable->dbau = HI32(phy);
	fPRDTable->dbc = DBC_I | (size - 1);

	TRACE("dba  %08x\n", fPRDTable->dba);
	TRACE("dbau %08x\n", fPRDTable->dbau);
	TRACE("dbc  %08x\n", fPRDTable->dbc);

	fRegs->ci |= 1;
	FlushPostedWrites();

	snooze(500000);

	TRACE("prdbc %ld\n", fCommandList->prdbc);

	TRACE("ci   0x%08lx\n", fRegs->ci);
	TRACE("ie   0x%08lx\n", fRegs->ie);
	TRACE("is   0x%08lx\n", fRegs->is);
	TRACE("cmd  0x%08lx\n", fRegs->cmd);
	TRACE("ssts 0x%08lx\n", fRegs->ssts);
	TRACE("sctl 0x%08lx\n", fRegs->sctl);
	TRACE("serr 0x%08lx\n", fRegs->serr);
	TRACE("sact 0x%08lx\n", fRegs->sact);
	TRACE("tfd  0x%08lx\n", fRegs->tfd);

	for (int i = 0; i < size; i += 8) {
		TRACE("  %02x %02x %02x %02x %02x %02x %02x %02x\n", data[i], data[i+1], data[i+2], data[i+3], data[i+4], data[i+5], data[i+6], data[i+7]);
//		TRACE("  %c%c%c%c%c%c%c%c\n", c(data[i]), c(data[i+1]), c(data[i+2]), c(data[i+3]), c(data[i+4]), c(data[i+5]), c(data[i+6]), c(data[i+7]));
	}

	delete_area(id);
}


void
AHCIPort::ScsiExecuteRequest(scsi_ccb *request)
{

	TRACE("AHCIPort::ScsiExecuteRequest port %d, opcode %u, length %u\n", fIndex, request->cdb[0], request->cdb_length);

	switch (request->cdb[0]) {
		case 0x00:
			break;
		case 0x12:
			IdentifyDevice();
			break;
	}

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
