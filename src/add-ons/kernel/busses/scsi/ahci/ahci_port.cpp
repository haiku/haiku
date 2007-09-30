/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_port.h"
#include "ahci_controller.h"
#include "util.h"
#include "ata_cmds.h"
#include "scsi_cmds.h"

#include <KernelExport.h>
#include <ByteOrder.h>
#include <stdio.h>
#include <string.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
// #define FLOW(a...)	dprintf("ahci: " a)
#define FLOW(a...)


AHCIPort::AHCIPort(AHCIController *controller, int index)
	: fIndex(index)
	, fRegs(&controller->fRegs->port[index])
	, fArea(-1)
	, fRequestSem(-1)
	, fResponseSem(-1)
	, fCommandActive(false)
	, fDevicePresent(false)
	, fUse48BitCommands(false)
	, fSectorSize(0)
	, fSectorCount(0)
{
	fRequestSem = create_sem(1, "ahci request");
	fResponseSem = create_sem(1, "ahci response");
}
				

AHCIPort::~AHCIPort()
{
	delete_sem(fRequestSem);
	delete_sem(fResponseSem);
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

	fDevicePresent = (fRegs->ssts & 0xf) == 0x3;

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

	if (fCommandActive)	
		release_sem_etc(fResponseSem, 1, B_RELEASE_IF_WAITING_ONLY | B_DO_NOT_RESCHEDULE);

	// clear interrupts
	fRegs->is = is;
}


status_t
AHCIPort::FillPrdTable(volatile prd *prdTable, int *prdCount, int prdMax, const void *data, size_t dataSize, bool ioc)
{
	int peMax = prdMax + 1;
	physical_entry pe[peMax];
	if (get_memory_map(data, dataSize, pe, peMax ) < B_OK) {
		TRACE("AHCIPort::FillPrdTable get_memory_map failed\n");
		return B_ERROR;
	}
	int peUsed;
	for (peUsed = 0; pe[peUsed].size; peUsed++)
		;
	return FillPrdTable(prdTable, prdCount, prdMax, pe, peUsed, dataSize, ioc);
}


status_t
AHCIPort::FillPrdTable(volatile prd *prdTable, int *prdCount, int prdMax, const physical_entry *sgTable, int sgCount, size_t dataSize, bool ioc)
{
	*prdCount = 0;
	while (sgCount > 0 && dataSize > 0) {
		size_t size = min_c(sgTable->size, dataSize);
		void *address = sgTable->address;
		FLOW("FillPrdTable: sg-entry addr %p, size %lu\n", address, size);
		if ((uint32)address & 1) {
			TRACE("AHCIPort::FillPrdTable: data alignment error\n");
			return B_ERROR;
		}
		dataSize -= size;
		while (size > 0) {
			size_t bytes = min_c(size, PRD_MAX_DATA_LENGTH);
			if (*prdCount == prdMax) {
				TRACE("AHCIPort::FillPrdTable: prd table exhausted\n");
				return B_ERROR;
			}
			FLOW("FillPrdTable: prd-entry %u, addr %p, size %lu\n", *prdCount, address, bytes);
			prdTable->dba  = LO32(address);
			prdTable->dbau = HI32(address);
			prdTable->res  = 0;
			prdTable->dbc  = bytes - 1;
			*prdCount += 1;
			prdTable++;
			address = (char *)address + bytes;
			size -= bytes;
		}
		sgTable++;
		sgCount--;
	}
	if (*prdCount == 0) {
		TRACE("AHCIPort::FillPrdTable: count is 0\n");
		return B_ERROR;
	}
	if (dataSize > 0) {
		TRACE("AHCIPort::FillPrdTable: sg table %ld bytes too small\n", dataSize);
		return B_ERROR;
	}
	if (ioc)
		(prdTable - 1)->dbc |= DBC_I;
	return B_OK;
}


void
AHCIPort::StartTransfer()
{
	acquire_sem(fRequestSem);
	fCommandActive = true;
}
				

status_t
AHCIPort::WaitForTransfer(int *status, bigtime_t timeout)
{
	status_t result = B_OK;
	if (acquire_sem_etc(fResponseSem, 1, B_RELATIVE_TIMEOUT, timeout) < B_OK) {
		result = B_TIMED_OUT;
	}
	fCommandActive = false;
	return result;
}


void
AHCIPort::FinishTransfer()
{
	release_sem(fRequestSem);
}


void
AHCIPort::ScsiTestUnitReady(scsi_ccb *request)
{
	TRACE("AHCIPort::ScsiTestUnitReady port %d\n", fIndex);
	request->subsys_status = SCSI_REQ_CMP;
	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiInquiry(scsi_ccb *request)
{
	TRACE("AHCIPort::ScsiInquiry port %d\n", fIndex);

	scsi_cmd_inquiry *cmd = (scsi_cmd_inquiry *)request->cdb;
	scsi_res_inquiry scsiData;
	ata_res_identify_device	ataData;

	if (cmd->evpd || cmd->page_code) {
		TRACE("invalid request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	memset(&ataData, 0, sizeof(ataData));

	StartTransfer();

	int prdEntrys;
	FillPrdTable(fPRDTable, &prdEntrys, PRD_TABLE_ENTRY_COUNT, &ataData, sizeof(ataData), true);


	TRACE("prdEntrys %d\n", prdEntrys);
	
	memset((void *)fCommandTable->cfis, 0, 5 * 4);
	fCommandTable->cfis[0] = 0x27;
	fCommandTable->cfis[1] = 0x80;
	fCommandTable->cfis[2] = 0xec;


	fCommandList->prdtl_flags_cfl = 0;
	fCommandList->prdtl = prdEntrys;
	fCommandList->cfl = 5;
	fCommandList->prdbc = 0;

	if (wait_until_clear(&fRegs->tfd, ATA_BSY | ATA_DRQ, 4000000) < B_OK) {
		TRACE("device is busy\n");
		FinishTransfer();
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}
	fRegs->ci |= 1;
	FlushPostedWrites();

	int status;
	WaitForTransfer(&status, 100000);

	TRACE("prdbc %ld\n", fCommandList->prdbc);
	TRACE("ci   0x%08lx\n", fRegs->ci);
	TRACE("is   0x%08lx\n", fRegs->is);
	TRACE("serr 0x%08lx\n", fRegs->serr);

/*
	TRACE("ci   0x%08lx\n", fRegs->ci);
	TRACE("ie   0x%08lx\n", fRegs->ie);
	TRACE("is   0x%08lx\n", fRegs->is);
	TRACE("cmd  0x%08lx\n", fRegs->cmd);
	TRACE("ssts 0x%08lx\n", fRegs->ssts);
	TRACE("sctl 0x%08lx\n", fRegs->sctl);
	TRACE("serr 0x%08lx\n", fRegs->serr);
	TRACE("sact 0x%08lx\n", fRegs->sact);
	TRACE("tfd  0x%08lx\n", fRegs->tfd);

	uint8 *data = (uint8*) &ataData;
	for (int i = 0; i < 512; i += 8) {
		TRACE("  %02x %02x %02x %02x %02x %02x %02x %02x\n", data[i], data[i+1], data[i+2], data[i+3], data[i+4], data[i+5], data[i+6], data[i+7]);
	}
*/
	scsiData.device_type = scsi_dev_direct_access;
	scsiData.device_qualifier = scsi_periph_qual_connected;
	scsiData.device_type_modifier = 0;
	scsiData.removable_medium = false;
	scsiData.ansi_version = 2;
	scsiData.ecma_version = 0;
	scsiData.iso_version = 0;
	scsiData.response_data_format = 2;
	scsiData.term_iop = false;
	scsiData.additional_length = sizeof(scsiData) - 4;
	scsiData.soft_reset = false;
	scsiData.cmd_queue = false;
	scsiData.linked = false;
	scsiData.sync = false;
	scsiData.write_bus16 = true;
	scsiData.write_bus32 = false;
	scsiData.relative_address = false;
	memcpy(scsiData.vendor_ident, ataData.model_number, sizeof(scsiData.vendor_ident));
	memcpy(scsiData.product_ident, ataData.model_number + 8, sizeof(scsiData.product_ident));
	memcpy(scsiData.product_rev, ataData.serial_number, sizeof(scsiData.product_rev));

	bool lba			= (ataData.words[49] & (1 << 9)) != 0;
	bool lba48			= (ataData.words[83] & (1 << 10)) != 0;
	uint32 sectors		= *(uint32*)&ataData.words[60];
	uint64 sectors48	= *(uint64*)&ataData.words[100];
	fUse48BitCommands   = lba && lba48;
	fSectorSize			= 512;
	fSectorCount		= !(lba || sectors) ? 0 : lba48 ? sectors48 : sectors;

#if 1
	if (fSectorCount < 0x0fffffff) {
		TRACE("disabling 48 bit commands\n");
		fUse48BitCommands = 0;
	}
#endif

	swap_words(ataData.model_number, sizeof(ataData.model_number));
	swap_words(ataData.serial_number, sizeof(ataData.serial_number));
	swap_words(ataData.firmware_revision, sizeof(ataData.firmware_revision));

	TRACE("model_number: %40s\n", ataData.model_number);
	TRACE("serial_number: %20s\n", ataData.serial_number);
  	TRACE("firmware_revision: %8s\n", ataData.firmware_revision);
	TRACE("lba %d, lba48 %d, fUse48BitCommands %d, sectors %lu, sectors48 %llu, size %llu\n",
		lba, lba48, fUse48BitCommands, sectors, sectors48, fSectorCount * fSectorSize);

	if (sg_memcpy(request->sg_list, request->sg_count, &scsiData, sizeof(scsiData)) < B_OK) {
		request->subsys_status = SCSI_DATA_RUN_ERR;
	} else {
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = request->data_length - sizeof(scsiData);// ???
		request->data_length = sizeof(scsiData); // ???
	}

	fRegs->ci &= ~1;
	FinishTransfer();

	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiReadCapacity(scsi_ccb *request)
{
	TRACE("AHCIPort::ScsiReadCapacity port %d\n", fIndex);

	scsi_cmd_read_capacity *cmd = (scsi_cmd_read_capacity *)request->cdb;
	scsi_res_read_capacity scsiData;

	if (cmd->pmi || cmd->lba) {
		TRACE("invalid request\n");
		return;
	}

	TRACE("SectorSize %lu, SectorCount 0x%llx\n", fSectorSize, fSectorCount);

	if (fSectorCount > 0xffffffff)
		panic("ahci: SCSI emulation doesn't support harddisks larger than 2TB");

	scsiData.block_size = B_HOST_TO_BENDIAN_INT32(fSectorSize);
	scsiData.lba = B_HOST_TO_BENDIAN_INT32(fSectorCount - 1);

	if (sg_memcpy(request->sg_list, request->sg_count, &scsiData, sizeof(scsiData)) < B_OK) {
		request->subsys_status = SCSI_DATA_RUN_ERR;
	} else {
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = request->data_length - sizeof(scsiData);// ???
		request->data_length = sizeof(scsiData); // ???
	}
	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiReadWrite(scsi_ccb *request, uint64 position, size_t length, bool isWrite)
{
	TRACE("ScsiReadWrite: pos %llu, size %lu, isWrite %d\n", position, length, isWrite);

#if 1
	if (isWrite) {
		TRACE("write request ignored\n");
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = 0;
		gSCSI->finished(request, 1);
		return;
	}
#endif

	StartTransfer();

	int prdEntrys;
	FillPrdTable(fPRDTable, &prdEntrys, PRD_TABLE_ENTRY_COUNT, request->sg_list, request->sg_count, length * 512, true);

	FLOW("prdEntrys %d\n", prdEntrys);
	
	memset((void *)fCommandTable->cfis, 0, 5 * 4);

	if (fUse48BitCommands) {
		fCommandTable->cfis[0] = 0x27;
		fCommandTable->cfis[1] = 0x80;
		fCommandTable->cfis[2] = isWrite ? 0x35 : 0x25;
		fCommandTable->cfis[4] = position & 0xff;
		fCommandTable->cfis[5] = (position >> 8) & 0xff;
		fCommandTable->cfis[6] = (position >> 16) & 0xff;
		fCommandTable->cfis[7] = 0x40; 
		fCommandTable->cfis[8] = (position >> 24) & 0xff;
		fCommandTable->cfis[9] = (position >> 32) & 0xff;
		fCommandTable->cfis[10] = (position >> 40) & 0xff;
		fCommandTable->cfis[12] = length & 0xff;
		fCommandTable->cfis[13] = (length >> 8) & 0xff;
	} else {
		if (length > 256)
			panic("ahci: ScsiReadWrite length too large");
		if (position > 0x0fffffff)
			panic("achi: ScsiReadWrite position too large for non-48-bit LBA\n");
		fCommandTable->cfis[0] = 0x27;
		fCommandTable->cfis[1] = 0x80;
		fCommandTable->cfis[2] = isWrite ? 0xca : 0xc8;
		fCommandTable->cfis[4] = position & 0xff;
		fCommandTable->cfis[5] = (position >> 8) & 0xff;
		fCommandTable->cfis[6] = (position >> 16) & 0xff;
		fCommandTable->cfis[7] = 0x40 | ((position >> 24) & 0x0f);
		fCommandTable->cfis[12] = length & 0xff;
	}

	fCommandList->prdtl_flags_cfl = 0;
	fCommandList->prdtl = prdEntrys;
	fCommandList->cfl = 5;
	fCommandList->prdbc = 0;

	if (wait_until_clear(&fRegs->tfd, ATA_BSY | ATA_DRQ, 4000000) < B_OK) {
		TRACE("device is busy\n");
		FinishTransfer();
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}
	fRegs->ci |= 1;
	FlushPostedWrites();

	int status;
	WaitForTransfer(&status, 100000);

	TRACE("prdbc %ld\n", fCommandList->prdbc);
	TRACE("ci   0x%08lx\n", fRegs->ci);
	TRACE("is   0x%08lx\n", fRegs->is);
	TRACE("serr 0x%08lx\n", fRegs->serr);

	request->subsys_status = SCSI_REQ_CMP;
	request->data_resid = request->data_length - fCommandList->prdbc;// ???
	request->data_length = fCommandList->prdbc; // ???

	fRegs->ci &= ~1;
	FinishTransfer();

	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiExecuteRequest(scsi_ccb *request)
{

	TRACE("AHCIPort::ScsiExecuteRequest port %d, opcode 0x%02x, length %u\n", fIndex, request->cdb[0], request->cdb_length);

	if (request->cdb[0] == SCSI_OP_REQUEST_SENSE) {
		panic("ahci: SCSI_OP_REQUEST_SENSE not yet supported\n");
		return;
	}

	if (!fDevicePresent) {
		TRACE("no such device!\n");
		request->subsys_status = SCSI_DEV_NOT_THERE;
		gSCSI->finished(request, 1);
		return;
	}
	
	request->subsys_status = SCSI_REQ_CMP;

	switch (request->cdb[0]) {
		case SCSI_OP_TEST_UNIT_READY:
			ScsiTestUnitReady(request);
			break;
		case SCSI_OP_INQUIRY:
			ScsiInquiry(request);
			break;
		case SCSI_OP_READ_CAPACITY:
			ScsiReadCapacity(request);
			break;
		case SCSI_OP_READ_6:
		case SCSI_OP_WRITE_6:
		{
			scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->cdb;
			uint32 position = ((uint32)cmd->high_lba << 16) | ((uint32)cmd->mid_lba << 8) | (uint32)cmd->low_lba;
			size_t length = cmd->length != 0 ? cmd->length : 256;
			bool isWrite = request->cdb[0] == SCSI_OP_WRITE_6;
			ScsiReadWrite(request, position, length, isWrite);
			break;
		}
		case SCSI_OP_READ_10:
		case SCSI_OP_WRITE_10:
		{
			scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)request->cdb;
			uint32 position = B_BENDIAN_TO_HOST_INT32(cmd->lba);
			size_t length = B_BENDIAN_TO_HOST_INT16(cmd->length);
			bool isWrite = request->cdb[0] == SCSI_OP_WRITE_10;
			if (length) {
				ScsiReadWrite(request, position, length, isWrite);
			} else {
				TRACE("AHCIPort::ScsiExecuteRequest error: transfer without data!\n");
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		}
		case SCSI_OP_READ_12:
		case SCSI_OP_WRITE_12:
		{
			scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)request->cdb;
			uint32 position = B_BENDIAN_TO_HOST_INT32(cmd->lba);
			size_t length = B_BENDIAN_TO_HOST_INT32(cmd->length);
			bool isWrite = request->cdb[0] == SCSI_OP_WRITE_12;
			if (length) {
				ScsiReadWrite(request, position, length, isWrite);
			} else {
				TRACE("AHCIPort::ScsiExecuteRequest error: transfer without data!\n");
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		}
		default:
			TRACE("AHCIPort::ScsiExecuteRequest port %d unsupported request opcode 0x%02x\n", fIndex, request->cdb[0]);
			request->subsys_status = SCSI_REQ_ABORTED;
			gSCSI->finished(request, 1);
	}
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
