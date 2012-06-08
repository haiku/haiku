/*
 * Copyright 2007-2009, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ahci_port.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <ATAInfoBlock.h>

#include "ahci_controller.h"
#include "ahci_tracing.h"
#include "sata_request.h"
#include "scsi_cmds.h"
#include "util.h"


#define TRACE_AHCI
#ifdef TRACE_AHCI
#	define TRACE(a...) dprintf("ahci: " a)
#else
#	define TRACE(a...)
#endif
//#define FLOW(a...)	dprintf("ahci: " a)
//#define RWTRACE(a...) dprintf("ahci: " a)
#define FLOW(a...)
#define RWTRACE(a...)


AHCIPort::AHCIPort(AHCIController *controller, int index)
	:
	fController(controller),
	fIndex(index),
	fRegs(&controller->fRegs->port[index]),
	fArea(-1),
	fCommandsActive(0),
	fRequestSem(-1),
	fResponseSem(-1),
	fDevicePresent(false),
	fUse48BitCommands(false),
	fSectorSize(0),
	fSectorCount(0),
	fIsATAPI(false),
	fTestUnitReadyActive(false),
	fResetPort(false),
	fError(false),
	fTrim(false)
{
	B_INITIALIZE_SPINLOCK(&fSpinlock);
	fRequestSem = create_sem(1, "ahci request");
	fResponseSem = create_sem(0, "ahci response");
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

	size_t size = sizeof(command_list_entry) * COMMAND_LIST_ENTRY_COUNT
		+ sizeof(fis) + sizeof(command_table)
		+ sizeof(prd) * PRD_TABLE_ENTRY_COUNT;

	char *virtAddr;
	phys_addr_t physAddr;

	fArea = alloc_mem((void **)&virtAddr, &physAddr, size, 0,
		"some AHCI port");
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
	TRACE("PRD table is at %p\n", fPRDTable);

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

	ResetPort(true);

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
		TRACE("AHCIPort::Uninit port %d error DMA engine still running\n",
			fIndex);
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


void
AHCIPort::ResetDevice()
{
	if (fRegs->cmd & PORT_CMD_ST)
		TRACE("AHCIPort::ResetDevice PORT_CMD_ST set, behaviour undefined\n");

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
			TRACE("AHCIPort::ResetDevice port %d device present but no phy "
				"communication\n", fIndex);
		}
	}

	// clear error bits
	fRegs->serr = fRegs->serr;
	FlushPostedWrites();
}



status_t
AHCIPort::ResetPort(bool forceDeviceReset)
{
	if (!fTestUnitReadyActive)
		TRACE("AHCIPort::ResetPort port %d\n", fIndex);

	// stop DMA engine
	fRegs->cmd &= ~PORT_CMD_ST;
	FlushPostedWrites();

	if (wait_until_clear(&fRegs->cmd, PORT_CMD_CR, 500000) < B_OK) {
		TRACE("AHCIPort::ResetPort port %d error DMA engine doesn't stop\n",
			fIndex);
	}

	bool deviceBusy = fRegs->tfd & (ATA_BSY | ATA_DRQ);

	if (!fTestUnitReadyActive) {
		TRACE("AHCIPort::ResetPort port %d, deviceBusy %d, "
			"forceDeviceReset %d\n", fIndex, deviceBusy, forceDeviceReset);
	}

	if (deviceBusy || forceDeviceReset)
		ResetDevice();

	// start DMA engine
	fRegs->cmd |= PORT_CMD_ST;
	FlushPostedWrites();

	return PostReset();
}


status_t
AHCIPort::PostReset()
{
	if (!fTestUnitReadyActive)
		TRACE("AHCIPort::PostReset port %d\n", fIndex);

	if ((fRegs->ssts & 0xf) != 0x3 || (fRegs->tfd & 0xff) == 0x7f) {
		TRACE("AHCIPort::PostReset port %d: no device\n", fIndex);
		return B_OK;
	}

	if ((fRegs->tfd & 0xff) == 0xff)
		snooze(200000);

	if ((fRegs->tfd & 0xff) == 0xff) {
		TRACE("AHCIPort::PostReset port %d: invalid task file status 0xff\n",
			fIndex);
		return B_ERROR;
	}

	wait_until_clear(&fRegs->tfd, ATA_BSY, 31000000);

	fIsATAPI = fRegs->sig == 0xeb140101;

	if (fIsATAPI)
		fRegs->cmd |= PORT_CMD_ATAPI;
	else
		fRegs->cmd &= ~PORT_CMD_ATAPI;
	FlushPostedWrites();

	if (!fTestUnitReadyActive) {
		TRACE("device signature 0x%08lx (%s)\n", fRegs->sig,
			(fRegs->sig == 0xeb140101) ? "ATAPI" : (fRegs->sig == 0x00000101) ?
				"ATA" : "unknown");
	}

	return B_OK;
}


void
AHCIPort::DumpD2HFis()
{
	TRACE("D2H FIS:\n");
	TRACE("  DW0  %02x %02x %02x %02x\n", fFIS->rfis[3], fFIS->rfis[2],
		fFIS->rfis[1], fFIS->rfis[0]);
	TRACE("  DW1  %02x %02x %02x %02x\n", fFIS->rfis[7], fFIS->rfis[6],
		fFIS->rfis[5], fFIS->rfis[4]);
	TRACE("  DW2  %02x %02x %02x %02x\n", fFIS->rfis[11], fFIS->rfis[10],
		fFIS->rfis[9], fFIS->rfis[8]);
	TRACE("  DW3  %02x %02x %02x %02x\n", fFIS->rfis[15], fFIS->rfis[14],
		fFIS->rfis[13], fFIS->rfis[12]);
	TRACE("  DW4  %02x %02x %02x %02x\n", fFIS->rfis[19], fFIS->rfis[18],
		fFIS->rfis[17], fFIS->rfis[16]);
}


void
AHCIPort::Interrupt()
{
	uint32 is = fRegs->is;
	fRegs->is = is; // clear interrupts

	if (is & PORT_INT_ERROR) {
		InterruptErrorHandler(is);
		return;
	}

	uint32 ci = fRegs->ci;

	RWTRACE("[%lld] %ld AHCIPort::Interrupt port %d, fCommandsActive 0x%08lx, "
		"is 0x%08lx, ci 0x%08lx\n", system_time(), find_thread(NULL),
		fIndex, fCommandsActive, is, ci);

	acquire_spinlock(&fSpinlock);
	if ((fCommandsActive & 1) && !(ci & 1)) {
		fCommandsActive &= ~1;
		release_sem_etc(fResponseSem, 1, B_DO_NOT_RESCHEDULE);
	}
	release_spinlock(&fSpinlock);
}


void
AHCIPort::InterruptErrorHandler(uint32 is)
{
	uint32 ci = fRegs->ci;

	if (!fTestUnitReadyActive) {
		TRACE("AHCIPort::InterruptErrorHandler port %d, "
			"fCommandsActive 0x%08lx, is 0x%08lx, ci 0x%08lx\n", fIndex,
			fCommandsActive, is, ci);

		TRACE("ssts 0x%08lx\n", fRegs->ssts);
		TRACE("sctl 0x%08lx\n", fRegs->sctl);
		TRACE("serr 0x%08lx\n", fRegs->serr);
		TRACE("sact 0x%08lx\n", fRegs->sact);
	}

	// read and clear SError
	uint32 serr = fRegs->serr;
	fRegs->serr = serr;

	if (is & PORT_INT_TFE) {
		if (!fTestUnitReadyActive)
			TRACE("Task File Error\n");

		fResetPort = true;
		fError = true;
	}
	if (is & PORT_INT_HBF) {
		TRACE("Host Bus Fatal Error\n");
		fResetPort = true;
		fError = true;
	}
	if (is & PORT_INT_HBD) {
		TRACE("Host Bus Data Error\n");
		fResetPort = true;
		fError = true;
	}
	if (is & PORT_INT_IF) {
		TRACE("Interface Fatal Error\n");
		fResetPort = true;
		fError = true;
	}
	if (is & PORT_INT_INF) {
		TRACE("Interface Non Fatal Error\n");
	}
	if (is & PORT_INT_OF) {
		TRACE("Overflow");
		fResetPort = true;
		fError = true;
	}
	if (is & PORT_INT_IPM) {
		TRACE("Incorrect Port Multiplier Status");
	}
	if (is & PORT_INT_PRC) {
		TRACE("PhyReady Change\n");
//		fResetPort = true;
	}
	if (is & PORT_INT_PC) {
		TRACE("Port Connect Change\n");
//		fResetPort = true;
	}
	if (is & PORT_INT_UF) {
		TRACE("Unknown FIS\n");
		fResetPort = true;
	}

	if (fError) {
		acquire_spinlock(&fSpinlock);
		if ((fCommandsActive & 1)) {
			fCommandsActive &= ~1;
			release_sem_etc(fResponseSem, 1, B_DO_NOT_RESCHEDULE);
		}
		release_spinlock(&fSpinlock);
	}
}


status_t
AHCIPort::FillPrdTable(volatile prd *prdTable, int *prdCount, int prdMax,
	const void *data, size_t dataSize)
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
	return FillPrdTable(prdTable, prdCount, prdMax, pe, peUsed, dataSize);
}


status_t
AHCIPort::FillPrdTable(volatile prd *prdTable, int *prdCount, int prdMax,
	const physical_entry *sgTable, int sgCount, size_t dataSize)
{
	*prdCount = 0;
	while (sgCount > 0 && dataSize > 0) {
		size_t size = min_c(sgTable->size, dataSize);
		phys_addr_t address = sgTable->address;
		T_PORT(AHCIPortPrdTable(fController, fIndex, address, size));
		FLOW("FillPrdTable: sg-entry addr %#" B_PRIxPHYSADDR ", size %lu\n",
			address, size);
		if (address & 1) {
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
			FLOW("FillPrdTable: prd-entry %u, addr %p, size %lu\n",
				*prdCount, address, bytes);

			prdTable->dba  = LO32(address);
			prdTable->dbau = HI32(address);
			prdTable->res  = 0;
			prdTable->dbc  = bytes - 1;
			*prdCount += 1;
			prdTable++;
			address = address + bytes;
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
		TRACE("AHCIPort::FillPrdTable: sg table %ld bytes too small\n",
			dataSize);
		return B_ERROR;
	}
	return B_OK;
}


void
AHCIPort::StartTransfer()
{
	acquire_sem(fRequestSem);
}


status_t
AHCIPort::WaitForTransfer(int *tfd, bigtime_t timeout)
{
	status_t result = acquire_sem_etc(fResponseSem, 1, B_RELATIVE_TIMEOUT,
		timeout);
	if (result < B_OK) {
		cpu_status cpu = disable_interrupts();
		acquire_spinlock(&fSpinlock);
		fCommandsActive &= ~1;
		release_spinlock(&fSpinlock);
		restore_interrupts(cpu);

		result = B_TIMED_OUT;
	} else if (fError) {
		*tfd = fRegs->tfd;
		result = B_ERROR;
		fError = false;
	} else {
		*tfd = fRegs->tfd;
	}
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
	ata_device_infoblock ataData;

	ASSERT(sizeof(ataData) == 512);

	if (cmd->evpd || cmd->page_code || request->data_length < sizeof(scsiData)) {
		TRACE("invalid request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	sata_request sreq;
	sreq.set_data(&ataData, sizeof(ataData));
	sreq.set_ata_cmd(fIsATAPI ? 0xa1 : 0xec); // Identify (Packet) Device
	ExecuteSataRequest(&sreq);
	sreq.wait_for_completition();

	if (sreq.completition_status() & ATA_ERR) {
		TRACE("identify device failed\n");
		request->subsys_status = SCSI_REQ_CMP_ERR;
		gSCSI->finished(request, 1);
		return;
	}

/*
	uint8 *data = (uint8*) &ataData;
	for (int i = 0; i < 512; i += 8) {
		TRACE("  %02x %02x %02x %02x %02x %02x %02x %02x\n", data[i], data[i+1],
			data[i+2], data[i+3], data[i+4], data[i+5], data[i+6], data[i+7]);
	}
*/

	scsiData.device_type = fIsATAPI ? scsi_dev_CDROM : scsi_dev_direct_access;
	scsiData.device_qualifier = scsi_periph_qual_connected;
	scsiData.device_type_modifier = 0;
	scsiData.removable_medium = fIsATAPI;
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
	memcpy(scsiData.vendor_ident, ataData.model_number,
		sizeof(scsiData.vendor_ident));
	memcpy(scsiData.product_ident, ataData.model_number + 8,
		sizeof(scsiData.product_ident));
	memcpy(scsiData.product_rev, ataData.serial_number,
		sizeof(scsiData.product_rev));

	if (!fIsATAPI) {
		bool lba = ataData.dma_supported != 0;
		bool lba48 = ataData.lba48_supported != 0;
		uint32 sectors = ataData.lba_sector_count;
		uint64 sectors48 = ataData.lba48_sector_count;
		fUse48BitCommands = lba && lba48;
		fSectorSize = 512;
		fSectorCount = !(lba || sectors) ? 0 : lba48 ? sectors48 : sectors;
		fTrim = ataData.data_set_management_support;
		TRACE("lba %d, lba48 %d, fUse48BitCommands %d, sectors %lu, "
			"sectors48 %llu, size %llu\n",
			lba, lba48, fUse48BitCommands, sectors, sectors48,
			fSectorCount * fSectorSize);
	}

#if 0
	if (fSectorCount < 0x0fffffff) {
		TRACE("disabling 48 bit commands\n");
		fUse48BitCommands = 0;
	}
#endif

	char modelNumber[sizeof(ataData.model_number) + 1];
	char serialNumber[sizeof(ataData.serial_number) + 1];
	char firmwareRev[sizeof(ataData.firmware_revision) + 1];

	strlcpy(modelNumber, ataData.model_number, sizeof(modelNumber));
	strlcpy(serialNumber, ataData.serial_number, sizeof(serialNumber));
	strlcpy(firmwareRev, ataData.firmware_revision, sizeof(firmwareRev));

	swap_words(modelNumber, sizeof(modelNumber) - 1);
	swap_words(serialNumber, sizeof(serialNumber) - 1);
	swap_words(firmwareRev, sizeof(firmwareRev) - 1);

	TRACE("model number: %s\n", modelNumber);
	TRACE("serial number: %s\n", serialNumber);
	TRACE("firmware rev.: %s\n", firmwareRev);
	TRACE("trim support: %s\n", fTrim ? "yes" : "no");

	if (sg_memcpy(request->sg_list, request->sg_count, &scsiData,
			sizeof(scsiData)) < B_OK) {
		request->subsys_status = SCSI_DATA_RUN_ERR;
	} else {
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = request->data_length - sizeof(scsiData);
	}
	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiSynchronizeCache(scsi_ccb *request)
{
	//TRACE("AHCIPort::ScsiSynchronizeCache port %d\n", fIndex);

	sata_request *sreq = new(std::nothrow) sata_request(request);
	if (sreq == NULL) {
		TRACE("out of memory when allocating sync request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	sreq->set_ata_cmd(fUse48BitCommands ? 0xea : 0xe7); // Flush Cache
	ExecuteSataRequest(sreq);
}


void
AHCIPort::ScsiReadCapacity(scsi_ccb *request)
{
	TRACE("AHCIPort::ScsiReadCapacity port %d\n", fIndex);

	scsi_cmd_read_capacity *cmd = (scsi_cmd_read_capacity *)request->cdb;
	scsi_res_read_capacity scsiData;

	if (cmd->pmi || cmd->lba || request->data_length < sizeof(scsiData)) {
		TRACE("invalid request\n");
		return;
	}

	TRACE("SectorSize %lu, SectorCount 0x%llx\n", fSectorSize, fSectorCount);

	if (fSectorCount > 0xffffffff)
		panic("ahci: SCSI emulation doesn't support harddisks larger than 2TB");

	scsiData.block_size = B_HOST_TO_BENDIAN_INT32(fSectorSize);
	scsiData.lba = B_HOST_TO_BENDIAN_INT32(fSectorCount - 1);

	if (sg_memcpy(request->sg_list, request->sg_count, &scsiData,
			sizeof(scsiData)) < B_OK) {
		request->subsys_status = SCSI_DATA_RUN_ERR;
	} else {
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = request->data_length - sizeof(scsiData);
	}
	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiReadWrite(scsi_ccb *request, uint64 lba, size_t sectorCount,
	bool isWrite)
{
	RWTRACE("[%lld] %ld ScsiReadWrite: position %llu, size %lu, isWrite %d\n",
		system_time(), find_thread(NULL), lba * 512, sectorCount * 512,
		isWrite);

#if 0
	if (isWrite) {
		TRACE("write request ignored\n");
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = 0;
		gSCSI->finished(request, 1);
		return;
	}
#endif

	ASSERT(request->data_length == sectorCount * 512);
	sata_request *sreq = new(std::nothrow) sata_request(request);
	if (sreq == NULL) {
		TRACE("out of memory when allocating read/write request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
	}

	if (fUse48BitCommands) {
		if (sectorCount > 65536) {
			panic("ahci: ScsiReadWrite length too large, %lu sectors",
				sectorCount);
		}
		if (lba > MAX_SECTOR_LBA_48)
			panic("achi: ScsiReadWrite position too large for 48-bit LBA\n");
		sreq->set_ata48_cmd(isWrite ? 0x35 : 0x25, lba, sectorCount);
	} else {
		if (sectorCount > 256) {
			panic("ahci: ScsiReadWrite length too large, %lu sectors",
				sectorCount);
		}
		if (lba > MAX_SECTOR_LBA_28)
			panic("achi: ScsiReadWrite position too large for normal LBA\n");
		sreq->set_ata28_cmd(isWrite ? 0xca : 0xc8, lba, sectorCount);
	}

	ExecuteSataRequest(sreq, isWrite);
}


void
AHCIPort::ExecuteSataRequest(sata_request *request, bool isWrite)
{
	FLOW("ExecuteAtaRequest port %d\n", fIndex);

	StartTransfer();

	int prdEntrys;

	if (request->ccb() && request->ccb()->data_length) {
		FillPrdTable(fPRDTable, &prdEntrys, PRD_TABLE_ENTRY_COUNT,
			request->ccb()->sg_list, request->ccb()->sg_count,
			request->ccb()->data_length);
	} else if (request->data() && request->size()) {
		FillPrdTable(fPRDTable, &prdEntrys, PRD_TABLE_ENTRY_COUNT,
			request->data(), request->size());
	} else
		prdEntrys = 0;

	FLOW("prdEntrys %d\n", prdEntrys);

	fCommandList->prdtl_flags_cfl = 0;
	fCommandList->cfl = 5; // 20 bytes, length in DWORDS
	memcpy((char *)fCommandTable->cfis, request->fis(), 20);

	fTestUnitReadyActive = request->is_test_unit_ready();
	if (request->is_atapi()) {
		// ATAPI PACKET is a 12 or 16 byte SCSI command
		memset((char *)fCommandTable->acmd, 0, 32);
		memcpy((char *)fCommandTable->acmd, request->ccb()->cdb,
			request->ccb()->cdb_length);
		fCommandList->a = 1;
	}

	if (isWrite)
		fCommandList->w = 1;
	fCommandList->prdtl = prdEntrys;
	fCommandList->prdbc = 0;

	if (wait_until_clear(&fRegs->tfd, ATA_BSY | ATA_DRQ, 1000000) < B_OK) {
		TRACE("ExecuteAtaRequest port %d: device is busy\n", fIndex);
		ResetPort();
		FinishTransfer();
		request->abort();
		return;
	}

	cpu_status cpu = disable_interrupts();
	acquire_spinlock(&fSpinlock);
	fCommandsActive |= 1;
	fRegs->ci = 1;
	FlushPostedWrites();
	release_spinlock(&fSpinlock);
	restore_interrupts(cpu);

	int tfd;
	status_t status = WaitForTransfer(&tfd, 20000000);

	FLOW("tfd %#x\n", tfd);
	FLOW("prdbc %ld\n", fCommandList->prdbc);
	FLOW("ci   0x%08lx\n", fRegs->ci);
	FLOW("is   0x%08lx\n", fRegs->is);
	FLOW("serr 0x%08lx\n", fRegs->serr);

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
*/

	if (fResetPort || status == B_TIMED_OUT) {
		fResetPort = false;
		ResetPort();
	}

	size_t bytesTransfered = fCommandList->prdbc;

	FinishTransfer();

	if (status == B_TIMED_OUT) {
		TRACE("ExecuteAtaRequest port %d: device timeout\n", fIndex);
		request->abort();
	} else {
		request->finish(tfd, bytesTransfered);
	}
}


void
AHCIPort::ScsiExecuteRequest(scsi_ccb *request)
{
//	TRACE("AHCIPort::ScsiExecuteRequest port %d, opcode 0x%02x, length %u\n", fIndex, request->cdb[0], request->cdb_length);

	if (fIsATAPI) {
		bool isWrite = false;
		switch (request->flags & SCSI_DIR_MASK) {
			case SCSI_DIR_NONE:
				ASSERT(request->data_length == 0);
				break;
			case SCSI_DIR_IN:
				ASSERT(request->data_length > 0);
				break;
			case SCSI_DIR_OUT:
				isWrite = true;
				ASSERT(request->data_length > 0);
				break;
			default:
				panic("CDB has invalid direction mask");
		}

//		TRACE("AHCIPort::ScsiExecuteRequest ATAPI: port %d, opcode 0x%02x, length %u\n", fIndex, request->cdb[0], request->cdb_length);

		sata_request *sreq = new(std::nothrow) sata_request(request);
		if (sreq == NULL) {
			TRACE("out of memory when allocating atapi request\n");
			request->subsys_status = SCSI_REQ_ABORTED;
			gSCSI->finished(request, 1);
			return;
		}

		sreq->set_atapi_cmd(request->data_length);
//		uint8 *data = (uint8*) sreq->ccb()->cdb;
//		for (int i = 0; i < 16; i += 8) {
//			TRACE("  %02x %02x %02x %02x %02x %02x %02x %02x\n", data[i], data[i+1], data[i+2], data[i+3], data[i+4], data[i+5], data[i+6], data[i+7]);
//		}
		ExecuteSataRequest(sreq, isWrite);
		return;
	}

	if (request->cdb[0] == SCSI_OP_REQUEST_SENSE) {
		panic("ahci: SCSI_OP_REQUEST_SENSE not yet supported\n");
		return;
	}

	if (!fDevicePresent) {
		TRACE("no device present on port %d\n", fIndex);
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
		case SCSI_OP_SYNCHRONIZE_CACHE:
			ScsiSynchronizeCache(request);
			break;
		case SCSI_OP_READ_6:
		case SCSI_OP_WRITE_6:
		{
			scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->cdb;
			uint32 position = ((uint32)cmd->high_lba << 16)
				| ((uint32)cmd->mid_lba << 8) | (uint32)cmd->low_lba;
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
				TRACE("AHCIPort::ScsiExecuteRequest error: transfer without "
					"data!\n");
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		}
		case SCSI_OP_READ_12:
		case SCSI_OP_WRITE_12:
		{
			scsi_cmd_rw_12 *cmd = (scsi_cmd_rw_12 *)request->cdb;
			uint32 position = B_BENDIAN_TO_HOST_INT32(cmd->lba);
			size_t length = B_BENDIAN_TO_HOST_INT32(cmd->length);
			bool isWrite = request->cdb[0] == SCSI_OP_WRITE_12;
			if (length) {
				ScsiReadWrite(request, position, length, isWrite);
			} else {
				TRACE("AHCIPort::ScsiExecuteRequest error: transfer without "
					"data!\n");
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		}
		case SCSI_OP_WRITE_SAME_16:
		{
			scsi_cmd_wsame_16 *cmd = (scsi_cmd_wsame_16 *)request->cdb;

			// SCSI unmap is used for trim, otherwise we don't support it
			if (!cmd->unmap) {
				TRACE("%s port %d: unsupported request opcode 0x%02x\n",
					__func__, fIndex, request->cdb[0]);
				request->subsys_status = SCSI_REQ_ABORTED;
				gSCSI->finished(request, 1);
				break;
			}

			if (!fTrim) {
				// Drive doesn't support trim (or atapi)
				// Just say it was successful and quit
				request->subsys_status = SCSI_REQ_CMP;
			} else {
				TRACE("%s unimplemented: TRIM call\n", __func__);
				// TODO: Make Serial ATA (sata_request?) trim call here.
				request->subsys_status = SCSI_REQ_ABORTED;
			}
			gSCSI->finished(request, 1);
			break;
		}
		default:
			TRACE("AHCIPort::ScsiExecuteRequest port %d unsupported request "
				"opcode 0x%02x\n", fIndex, request->cdb[0]);
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


void
AHCIPort::ScsiGetRestrictions(bool *isATAPI, bool *noAutoSense,
	uint32 *maxBlocks)
{
	*isATAPI = fIsATAPI;
	*noAutoSense = fIsATAPI; // emulated auto sense for ATA, but not ATAPI
	*maxBlocks = fUse48BitCommands ? 65536 : 256;
	TRACE("AHCIPort::ScsiGetRestrictions port %d: isATAPI %d, noAutoSense %d, "
		"maxBlocks %lu\n", fIndex, *isATAPI, *noAutoSense, *maxBlocks);
}
