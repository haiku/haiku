/*
 * Copyright 2008-2021 Haiku, Inc. All rights reserved.
 * Copyright 2007-2009, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		David Sebek, dasebek@gmail.com
 */


#include "ahci_port.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <ATACommands.h>
#include <ATAInfoBlock.h>
#include <AutoDeleter.h>

#include "ahci_controller.h"
#include "ahci_tracing.h"
#include "sata_request.h"
#include "scsi_cmds.h"
#include "util.h"


//#define TRACE_AHCI
#ifdef TRACE_AHCI
#	define TRACE(a...) dprintf("ahci: " a)
#else
#	define TRACE(a...)
#endif

#define ERROR(a...) dprintf("ahci: " a)
//#define FLOW(a...)	dprintf("ahci: " a)
//#define RWTRACE(a...) dprintf("ahci: " a)
#define FLOW(a...)
#define RWTRACE(a...)


#define INQUIRY_BASE_LENGTH 36


// DATA SET MANAGEMENT command limits

#define DSM_MAX_COUNT_48			UINT16_MAX
	// max number of 512-byte blocks (48-bit command)
#define DSM_MAX_COUNT_28			UINT8_MAX
	// max number of 512-byte blocks (28-bit command)
#define DSM_RANGE_BLOCK_ENTRIES		64
	// max entries in a 512-byte block (512 / 8)
#define DSM_MAX_RANGE_VALUE			UINT16_C(0xffff)
#define DSM_MAX_LBA_VALUE			UINT64_C(0xffffffffffff)


AHCIPort::AHCIPort(AHCIController* controller, int index)
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
	fPortReset(false),
	fError(false),
	fTrimSupported(false),
	fTrimReturnsZeros(false),
	fMaxTrimRangeBlocks(0)
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

	char* virtAddr;
	phys_addr_t physAddr;
	char name[32];
	snprintf(name, sizeof(name), "AHCI port %d", fIndex);

	fArea = alloc_mem((void**)&virtAddr, &physAddr, size, 0, name);
	if (fArea < B_OK) {
		TRACE("failed allocating memory for port %d\n", fIndex);
		return fArea;
	}
	memset(virtAddr, 0, size);

	fCommandList = (command_list_entry*)virtAddr;
	virtAddr += sizeof(command_list_entry) * COMMAND_LIST_ENTRY_COUNT;
	fFIS = (fis*)virtAddr;
	virtAddr += sizeof(fis);
	fCommandTable = (command_table*)virtAddr;
	virtAddr += sizeof(command_table);
	fPRDTable = (prd*)virtAddr;
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
	fRegs->sctl |= (SCTL_PORT_IPM_NOPART | SCTL_PORT_IPM_NOSLUM);

	// clear IRQ status bits
	fRegs->is = fRegs->is;

	// clear error bits
	_ClearErrorRegister();

	// power up device
	fRegs->cmd |= PORT_CMD_POD;

	// spin up device
	fRegs->cmd |= PORT_CMD_SUD;

	// activate link
	fRegs->cmd = (fRegs->cmd & ~PORT_CMD_ICC_MASK) | PORT_CMD_ICC_ACTIVE;

	// enable FIS receive (enabled when fb set, only to be disabled when unset)
	fRegs->cmd |= PORT_CMD_FRE;

	FlushPostedWrites();

	return B_OK;
}


// called with global interrupts enabled
status_t
AHCIPort::Init2()
{
	TRACE("AHCIPort::Init2 port %d\n", fIndex);

	// enable port
	Enable();

	// enable interrupts
	fRegs->ie = PORT_INT_MASK;

	FlushPostedWrites();

	// reset port and probe info
	ResetDevice();

	DumpHBAState();

	TRACE("%s: port %d, device %s\n", __func__, fIndex,
		fDevicePresent ? "present" : "absent");

	return B_OK;
}


void
AHCIPort::Uninit()
{
	TRACE("AHCIPort::Uninit port %d\n", fIndex);

	// Spec v1.3.1, §10.3.2 - Shut down port before unsetting FRE

	// shutdown the port
	if (!Disable()) {
		ERROR("%s: port %d error, unable to shutdown before FRE clear!\n",
			__func__, fIndex);
		return;
	}

	// Clear FRE and wait for completion
	fRegs->cmd &= ~PORT_CMD_FRE;
	if (wait_until_clear(&fRegs->cmd, PORT_CMD_FR, 500000) < B_OK)
		ERROR("%s: port %d error FIS rx still running\n", __func__, fIndex);

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
	// perform a hard reset
	if (PortReset() != B_OK) {
		ERROR("%s: port %d unable to hard reset device\n", __func__, fIndex);
		return;
	}

	if (wait_until_set(&fRegs->ssts, SSTS_PORT_DET_NODEV, 100000) < B_OK)
		TRACE("AHCIPort::ResetDevice port %d no device detected\n", fIndex);

	_ClearErrorRegister();

	if (fRegs->ssts & SSTS_PORT_DET_NOPHY) {
		if (wait_until_set(&fRegs->ssts, 0x3, 500000) < B_OK) {
			TRACE("AHCIPort::ResetDevice port %d device present but no phy "
				"communication\n", fIndex);
		}
	}

	_ClearErrorRegister();
}


status_t
AHCIPort::PortReset()
{
	TRACE("AHCIPort::PortReset port %d\n", fIndex);

	if (!Disable()) {
		ERROR("%s: port %d unable to shutdown!\n", __func__, fIndex);
		return B_ERROR;
	}

	_ClearErrorRegister();

	// Wait for BSY and DRQ to clear (idle port)
	if (wait_until_clear(&fRegs->tfd, ATA_STATUS_BUSY | ATA_STATUS_DATA_REQUEST,
		1000000) < B_OK) {
		// If we can't clear busy, do a full comreset

		// Spec v1.3.1, §10.4.2 Port Reset
		// Physical comm between HBA and port disabled. More Intrusive
		ERROR("%s: port %d undergoing COMRESET\n", __func__, fIndex);

		// Notice we're throwing out all other control flags.
		fRegs->sctl = (SSTS_PORT_IPM_ACTIVE | SSTS_PORT_IPM_PARTIAL
			| SCTL_PORT_DET_INIT);
		FlushPostedWrites();
		spin(1100);
		// You must wait 1ms at minimum
		fRegs->sctl = (fRegs->sctl & ~HBA_PORT_DET_MASK) | SCTL_PORT_DET_NOINIT;
		FlushPostedWrites();
	}

	Enable();

	if (wait_until_set(&fRegs->ssts, SSTS_PORT_DET_PRESENT, 500000) < B_OK) {
		TRACE("%s: port %d: no device detected\n", __func__, fIndex);
		fDevicePresent = false;
		return B_OK;
	}

	return Probe();
}


status_t
AHCIPort::Probe()
{
	if ((fRegs->tfd & 0xff) == 0xff)
		snooze(200000);

	if ((fRegs->tfd & 0xff) == 0xff) {
		TRACE("%s: port %d: invalid task file status 0xff\n", __func__,
			fIndex);
		return B_ERROR;
	}

	if (!fTestUnitReadyActive) {
		switch (fRegs->ssts & HBA_PORT_SPD_MASK) {
			case 0x10:
				ERROR("%s: port %d link speed 1.5Gb/s\n", __func__, fIndex);
				break;
			case 0x20:
				ERROR("%s: port %d link speed 3.0Gb/s\n", __func__, fIndex);
				break;
			case 0x30:
				ERROR("%s: port %d link speed 6.0Gb/s\n", __func__, fIndex);
				break;
			default:
				ERROR("%s: port %d link speed unrestricted\n", __func__, fIndex);
				break;
		}
	}

	wait_until_clear(&fRegs->tfd, ATA_STATUS_BUSY, 31000000);

	fDevicePresent = (fRegs->ssts & HBA_PORT_DET_MASK) == SSTS_PORT_DET_PRESENT;
	fIsATAPI = fRegs->sig == SATA_SIG_ATAPI;

	if (fIsATAPI)
		fRegs->cmd |= PORT_CMD_ATAPI;
	else
		fRegs->cmd &= ~PORT_CMD_ATAPI;
	FlushPostedWrites();

	TRACE("device signature 0x%08" B_PRIx32 " (%s)\n", fRegs->sig,
		fRegs->sig == SATA_SIG_ATAPI ? "ATAPI" : fRegs->sig == SATA_SIG_ATA
			? "ATA" : "unknown");

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
AHCIPort::DumpHBAState()
{
	TRACE("Port %d state:\n", fIndex);
	TRACE("  ie   0x%08" B_PRIx32 "\n", fRegs->ie);
	TRACE("  is   0x%08" B_PRIx32 "\n", fRegs->is);
	TRACE("  cmd  0x%08" B_PRIx32 "\n", fRegs->cmd);
	TRACE("  ssts 0x%08" B_PRIx32 "\n", fRegs->ssts);
	TRACE("  sctl 0x%08" B_PRIx32 "\n", fRegs->sctl);
	TRACE("  serr 0x%08" B_PRIx32 "\n", fRegs->serr);
	TRACE("  sact 0x%08" B_PRIx32 "\n", fRegs->sact);
	TRACE("  tfd  0x%08" B_PRIx32 "\n", fRegs->tfd);
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

	RWTRACE("[%lld] %ld AHCIPort::Interrupt port %d, fCommandsActive 0x%08"
		B_PRIx32 ", is 0x%08" B_PRIx32 ", ci 0x%08" B_PRIx32 "\n",
		system_time(), find_thread(NULL), fIndex, fCommandsActive, is, ci);

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
	TRACE("AHCIPort::InterruptErrorHandler port %d, fCommandsActive 0x%08"
		B_PRIx32 ", is 0x%08" B_PRIx32 ", ci 0x%08" B_PRIx32 "\n", fIndex,
		fCommandsActive, is, fRegs->ci);
	TRACE("ssts 0x%08" B_PRIx32 "\n", fRegs->ssts);
	TRACE("sctl 0x%08" B_PRIx32 "\n", fRegs->sctl);
	TRACE("serr 0x%08" B_PRIx32 "\n", fRegs->serr);
	TRACE("sact 0x%08" B_PRIx32 "\n", fRegs->sact);

	// read and clear SError
	_ClearErrorRegister();

	if (is & PORT_INT_TFE) {
		TRACE("Task File Error\n");

		fPortReset = true;
		fError = true;
	}
	if (is & PORT_INT_HBF) {
		ERROR("Host Bus Fatal Error\n");
		fPortReset = true;
		fError = true;
	}
	if (is & PORT_INT_HBD) {
		ERROR("Host Bus Data Error\n");
		fPortReset = true;
		fError = true;
	}
	if (is & PORT_INT_IF) {
		ERROR("Interface Fatal Error\n");
		fPortReset = true;
		fError = true;
	}
	if (is & PORT_INT_INF) {
		TRACE("Interface Non Fatal Error\n");
	}
	if (is & PORT_INT_OF) {
		TRACE("Overflow\n");
		fPortReset = true;
		fError = true;
	}
	if (is & PORT_INT_IPM) {
		TRACE("Incorrect Port Multiplier Status\n");
	}
	if (is & PORT_INT_PRC) {
		TRACE("PhyReady Change\n");
		//fPortReset = true;
	}
	if (is & PORT_INT_PC) {
		TRACE("Port Connect Change\n");
		// Unsolicited when we had a port connect change without us requesting
		// Spec v1.3, §6.2.2.3 Recovery of Unsolicited COMINIT

		// XXX: This shouldn't be needed here... but we can loop without it
		//_ClearErrorRegister();
	}
	if (is & PORT_INT_UF) {
		TRACE("Unknown FIS\n");
		fPortReset = true;
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
AHCIPort::FillPrdTable(volatile prd* prdTable, int* prdCount, int prdMax,
	const void* data, size_t dataSize)
{
	int maxEntries = prdMax + 1;
	physical_entry entries[maxEntries];
	uint32 entriesUsed = maxEntries;

	status_t status = get_memory_map_etc(B_CURRENT_TEAM, data, dataSize,
		entries, &entriesUsed);
	if (status != B_OK) {
		ERROR("%s: get_memory_map() failed: %s\n", __func__, strerror(status));
		return B_ERROR;
	}

	return FillPrdTable(prdTable, prdCount, prdMax, entries, entriesUsed,
		dataSize);
}


status_t
AHCIPort::FillPrdTable(volatile prd* prdTable, int* prdCount, int prdMax,
	const physical_entry* sgTable, int sgCount, size_t dataSize)
{
	*prdCount = 0;
	while (sgCount > 0 && dataSize > 0) {
		size_t size = min_c(sgTable->size, dataSize);
		phys_addr_t address = sgTable->address;
		T_PORT(AHCIPortPrdTable(fController, fIndex, address, size));
		FLOW("FillPrdTable: sg-entry addr %#" B_PRIxPHYSADDR ", size %lu\n",
			address, size);
		if (address & 1) {
			ERROR("AHCIPort::FillPrdTable: data alignment error\n");
			return B_ERROR;
		}
		dataSize -= size;
		while (size > 0) {
			size_t bytes = min_c(size, PRD_MAX_DATA_LENGTH);
			if (*prdCount == prdMax) {
				ERROR("AHCIPort::FillPrdTable: prd table exhausted\n");
				return B_ERROR;
			}
			FLOW("FillPrdTable: prd-entry %u, addr %p, size %lu\n",
				*prdCount, address, bytes);

			prdTable->dba = LO32(address);
			prdTable->dbau = HI32(address);
			prdTable->res = 0;
			prdTable->dbc = bytes - 1;
			*prdCount += 1;
			prdTable++;
			address = address + bytes;
			size -= bytes;
		}
		sgTable++;
		sgCount--;
	}
	if (*prdCount == 0) {
		ERROR("%s: count is 0\n", __func__);
		return B_ERROR;
	}
	if (dataSize > 0) {
		ERROR("AHCIPort::FillPrdTable: sg table %ld bytes too small\n",
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
AHCIPort::WaitForTransfer(int* tfd, bigtime_t timeout)
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
AHCIPort::ScsiTestUnitReady(scsi_ccb* request)
{
	TRACE("AHCIPort::ScsiTestUnitReady port %d\n", fIndex);
	request->subsys_status = SCSI_REQ_CMP;
	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiVPDInquiry(scsi_ccb* request, ata_device_infoblock* ataData)
{
	TRACE("AHCIPort::ScsiVPDInquiry port %d\n", fIndex);

	const scsi_cmd_inquiry* cmd = (const scsi_cmd_inquiry*)request->cdb;

	size_t vpdDataLength = 0;
	status_t transactionResult = B_ERROR;

	switch (cmd->page_code) {
		case SCSI_PAGE_SUPPORTED_VPD:
		{
			// supported pages should be in ascending numerical order
			const uint8 supportedPages[] = {
				SCSI_PAGE_SUPPORTED_VPD,
				SCSI_PAGE_BLOCK_LIMITS,
				SCSI_PAGE_LB_PROVISIONING
			};

			const size_t bufferLength = sizeof(scsi_page_list)
				+ sizeof(supportedPages) - 1;
			uint8 buffer[bufferLength];

			scsi_page_list* vpdPageData = (scsi_page_list*)buffer;
			memset(vpdPageData, 0, bufferLength);

			vpdPageData->page_code = cmd->page_code;
			// Our supported pages
			vpdPageData->page_length = sizeof(supportedPages);
			memcpy(vpdPageData->pages, supportedPages, sizeof(supportedPages));

			uint8 allocationLength = cmd->allocation_length;
			vpdDataLength = min_c(allocationLength, bufferLength);

			transactionResult = sg_memcpy(request->sg_list, request->sg_count,
				vpdPageData, vpdDataLength);
			break;
		}
		case SCSI_PAGE_BLOCK_LIMITS:
		{
			scsi_page_block_limits vpdPageData;
			memset(&vpdPageData, 0, sizeof(vpdPageData));

			vpdPageData.page_code = cmd->page_code;
			vpdPageData.page_length
				= B_HOST_TO_BENDIAN_INT16(sizeof(vpdPageData) - 4);
			if (fTrimSupported) {
				// We can handle anything as long as we have enough memory
				// (UNMAP structure can realistically be max. 65528 bytes)
				vpdPageData.max_unmap_lba_count
					= B_HOST_TO_BENDIAN_INT32(UINT32_MAX);
				vpdPageData.max_unmap_blk_count
					= B_HOST_TO_BENDIAN_INT32(UINT32_MAX);
			}

			uint8 allocationLength = cmd->allocation_length;
			vpdDataLength = min_c(allocationLength, sizeof(vpdPageData));

			transactionResult = sg_memcpy(request->sg_list, request->sg_count,
				&vpdPageData, vpdDataLength);
			break;
		}
		case SCSI_PAGE_LB_PROVISIONING:
		{
			scsi_page_lb_provisioning vpdPageData;
			memset(&vpdPageData, 0, sizeof(vpdPageData));

			vpdPageData.page_code = cmd->page_code;
			vpdPageData.page_length
				= B_HOST_TO_BENDIAN_INT16(sizeof(vpdPageData) - 4);
			vpdPageData.lbpu = fTrimSupported;
			vpdPageData.lbprz = fTrimReturnsZeros;

			uint8 allocationLength = cmd->allocation_length;
			vpdDataLength = min_c(allocationLength, sizeof(vpdPageData));

			transactionResult = sg_memcpy(request->sg_list, request->sg_count,
				&vpdPageData, vpdDataLength);
			break;
		}
		case SCSI_PAGE_USN:
		case SCSI_PAGE_BLOCK_DEVICE_CHARS:
		case SCSI_PAGE_REFERRALS:
			ERROR("VPD AHCI page %d not yet implemented!\n",
				cmd->page_code);
			//request->subsys_status = SCSI_REQ_CMP;
			request->subsys_status = SCSI_REQ_ABORTED;
			return;
		default:
			ERROR("unknown VPD page code!\n");
			request->subsys_status = SCSI_REQ_ABORTED;
			return;
	}

	if (transactionResult < B_OK) {
		request->subsys_status = SCSI_DATA_RUN_ERR;
	} else {
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = request->data_length
			- vpdDataLength;
	}
}


void
AHCIPort::ScsiInquiry(scsi_ccb* request)
{
	TRACE("AHCIPort::ScsiInquiry port %d\n", fIndex);

	const scsi_cmd_inquiry* cmd = (const scsi_cmd_inquiry*)request->cdb;
	scsi_res_inquiry scsiData;
	ata_device_infoblock ataData;

	ASSERT(sizeof(ataData) == 512);

	if (cmd->evpd) {
		TRACE("VPD inquiry page 0x%X\n", cmd->page_code);
		if (!request->data || request->data_length == 0) {
			ERROR("invalid VPD request\n");
			request->subsys_status = SCSI_REQ_ABORTED;
			gSCSI->finished(request, 1);
			return;
		}
	} else if (cmd->page_code) {
		// page_code without evpd is invalid per SCSI spec
		ERROR("page code 0x%X on non-VPD request\n", cmd->page_code);
		request->subsys_status = SCSI_REQ_ABORTED;
		request->device_status = SCSI_STATUS_CHECK_CONDITION;
		// TODO: Sense ILLEGAL REQUEST + INVALID FIELD IN CDB?
		gSCSI->finished(request, 1);
		return;
	} else if (request->data_length < INQUIRY_BASE_LENGTH) {
		ERROR("invalid request %" B_PRIu32 "\n", request->data_length);
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	memset(&ataData, 0, sizeof(ataData));

	sata_request sreq;
	sreq.SetData(&ataData, sizeof(ataData));
	sreq.SetATACommand(fIsATAPI
		? ATA_COMMAND_IDENTIFY_PACKET_DEVICE : ATA_COMMAND_IDENTIFY_DEVICE);
	ExecuteSataRequest(&sreq);
	sreq.WaitForCompletion();

	if ((sreq.CompletionStatus() & ATA_STATUS_ERROR) != 0) {
		ERROR("identify device failed\n");
		request->subsys_status = SCSI_REQ_CMP_ERR;
		gSCSI->finished(request, 1);
		return;
	}

	if (cmd->evpd) {
		// Simulate SCSI VPD data.
		ScsiVPDInquiry(request, &ataData);
		gSCSI->finished(request, 1);
		return;
	}

/*
	uint8* data = (uint8*)&ataData;
	for (int i = 0; i < 512; i += 8) {
		TRACE("  %02x %02x %02x %02x %02x %02x %02x %02x\n", data[i], data[i+1],
			data[i+2], data[i+3], data[i+4], data[i+5], data[i+6], data[i+7]);
	}
*/

	memset(&scsiData, 0, sizeof(scsiData));

	scsiData.device_type = fIsATAPI
		? ataData.word_0.atapi.command_packet_set : scsi_dev_direct_access;
	scsiData.device_qualifier = scsi_periph_qual_connected;
	scsiData.device_type_modifier = 0;
	scsiData.removable_medium = ataData.word_0.ata.removable_media_device;
	scsiData.ansi_version = 5;
		// Set the version to SPC-3 so that scsi_periph
		// uses READ CAPACITY (16) and attempts to read VPD pages
	scsiData.ecma_version = 0;
	scsiData.iso_version = 0;
	scsiData.response_data_format = 2;
	scsiData.term_iop = false;
	scsiData.additional_length = sizeof(scsi_res_inquiry) - 4;
	scsiData.soft_reset = false;
	scsiData.cmd_queue = false;
	scsiData.linked = false;
	scsiData.sync = false;
	scsiData.write_bus16 = true;
	scsiData.write_bus32 = false;
	scsiData.relative_address = false;

	if (!fIsATAPI) {
		fSectorCount = ataData.SectorCount(fUse48BitCommands, true);
		fSectorSize = ataData.SectorSize();
		fTrimSupported = ataData.data_set_management_support;
		fTrimReturnsZeros = ataData.supports_read_zero_after_trim;
		fMaxTrimRangeBlocks = B_LENDIAN_TO_HOST_INT16(
			ataData.max_data_set_management_lba_range_blocks);
		TRACE("lba %d, lba48 %d, fUse48BitCommands %d, sectors %" B_PRIu32
			", sectors48 %" B_PRIu64 ", size %" B_PRIu64 "\n",
			ataData.dma_supported != 0, ataData.lba48_supported != 0,
			fUse48BitCommands, ataData.lba_sector_count,
			ataData.lba48_sector_count, fSectorCount * fSectorSize);
		if (fTrimSupported) {
			if (fMaxTrimRangeBlocks == 0)
				fMaxTrimRangeBlocks = 1;

			#ifdef TRACE_AHCI
			bool deterministic = ataData.supports_deterministic_read_after_trim;
			TRACE("trim supported, %" B_PRIu32 " ranges blocks, reads are "
				"%sdeterministic%s.\n", fMaxTrimRangeBlocks,
				deterministic ? "" : "non-", deterministic
					? (ataData.supports_read_zero_after_trim
						? ", zero" : ", undefined") : "");
			#endif
		}
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

	// There's not enough space to fit all of the data in. ATA has 40 bytes for
	// the model number, 20 for the serial number and another 8 for the
	// firmware revision. SCSI has room for 8 for vendor ident, 16 for product
	// ident and another 4 for product revision.
	size_t vendorLen = strcspn(modelNumber, " ");
	if (vendorLen >= sizeof(scsiData.vendor_ident))
		vendorLen = strcspn(modelNumber, "-");
	if (vendorLen < sizeof(scsiData.vendor_ident)) {
		// First we try to break things apart smartly.
		snprintf(scsiData.vendor_ident, vendorLen + 1, "%s", modelNumber);
		size_t modelRemain = (sizeof(modelNumber) - vendorLen);
		if (modelRemain > sizeof(scsiData.product_ident))
			modelRemain = sizeof(scsiData.product_ident);
		memcpy(scsiData.product_ident, modelNumber + (vendorLen + 1),
			modelRemain);
	} else {
		// If we're unable to smartly break apart the vendor and model, just
		// dumbly squeeze as much in as possible.
		memcpy(scsiData.vendor_ident, modelNumber, sizeof(scsiData.vendor_ident));
		memcpy(scsiData.product_ident, modelNumber + 8,
			sizeof(scsiData.product_ident));
	}
	// Take the last 4 digits of the serial number as product rev
	size_t serialLen = sizeof(scsiData.product_rev);
	size_t serialOff = sizeof(serialNumber) - serialLen;
	memcpy(scsiData.product_rev, serialNumber + serialOff, serialLen);

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
AHCIPort::ScsiSynchronizeCache(scsi_ccb* request)
{
	//TRACE("AHCIPort::ScsiSynchronizeCache port %d\n", fIndex);

	sata_request* sreq = new(std::nothrow) sata_request(request);
	if (sreq == NULL) {
		ERROR("out of memory when allocating sync request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	sreq->SetATACommand(fUse48BitCommands
		? ATA_COMMAND_FLUSH_CACHE_EXT : ATA_COMMAND_FLUSH_CACHE);
	ExecuteSataRequest(sreq);
}


void
AHCIPort::ScsiReadCapacity(scsi_ccb* request)
{
	TRACE("AHCIPort::ScsiReadCapacity port %d\n", fIndex);

	const scsi_cmd_read_capacity* cmd
		= (const scsi_cmd_read_capacity*)request->cdb;
	scsi_res_read_capacity scsiData;

	if (cmd->pmi || cmd->lba || request->data_length < sizeof(scsiData)) {
		TRACE("invalid request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	TRACE("SectorSize %" B_PRIu32 ", SectorCount 0x%" B_PRIx64 "\n",
		fSectorSize, fSectorCount);

	memset(&scsiData, 0, sizeof(scsiData));

	scsiData.block_size = B_HOST_TO_BENDIAN_INT32(fSectorSize);

	if (fSectorCount <= 0xffffffff)
		scsiData.lba = B_HOST_TO_BENDIAN_INT32(fSectorCount - 1);
	else
		scsiData.lba = 0xffffffff;

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
AHCIPort::ScsiReadCapacity16(scsi_ccb* request)
{
	TRACE("AHCIPort::ScsiReadCapacity16 port %d\n", fIndex);

	const scsi_cmd_read_capacity_long* cmd
		= (const scsi_cmd_read_capacity_long*)request->cdb;
	scsi_res_read_capacity_long scsiData;

	uint32 allocationLength = B_BENDIAN_TO_HOST_INT32(cmd->alloc_length);
	size_t copySize = min_c(allocationLength, sizeof(scsiData));

	if (cmd->pmi || cmd->lba || request->data_length < copySize) {
		TRACE("invalid request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	TRACE("SectorSize %" B_PRIu32 ", SectorCount 0x%" B_PRIx64 "\n",
		fSectorSize, fSectorCount);

	memset(&scsiData, 0, sizeof(scsiData));

	scsiData.block_size = B_HOST_TO_BENDIAN_INT32(fSectorSize);
	scsiData.lba = B_HOST_TO_BENDIAN_INT64(fSectorCount - 1);
	scsiData.rc_basis = 0x01;
	scsiData.lbpme = fTrimSupported;
	scsiData.lbprz = fTrimReturnsZeros;

	if (sg_memcpy(request->sg_list, request->sg_count, &scsiData,
			copySize) < B_OK) {
		request->subsys_status = SCSI_DATA_RUN_ERR;
	} else {
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = request->data_length - copySize;
	}
	gSCSI->finished(request, 1);
}


void
AHCIPort::ScsiReadWrite(scsi_ccb* request, uint64 lba, size_t sectorCount,
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
	sata_request* sreq = new(std::nothrow) sata_request(request);
	if (sreq == NULL) {
		TRACE("out of memory when allocating read/write request\n");
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	if (fUse48BitCommands) {
		if (sectorCount > 65536) {
			panic("ahci: ScsiReadWrite length too large, %lu sectors",
				sectorCount);
		}
		if (lba > MAX_SECTOR_LBA_48)
			panic("achi: ScsiReadWrite position too large for 48-bit LBA\n");
		sreq->SetATA48Command(
			isWrite ? ATA_COMMAND_WRITE_DMA_EXT : ATA_COMMAND_READ_DMA_EXT,
			lba, sectorCount);
	} else {
		if (sectorCount > 256) {
			panic("ahci: ScsiReadWrite length too large, %lu sectors",
				sectorCount);
		}
		if (lba > MAX_SECTOR_LBA_28)
			panic("achi: ScsiReadWrite position too large for normal LBA\n");
		sreq->SetATA28Command(isWrite
			? ATA_COMMAND_WRITE_DMA : ATA_COMMAND_READ_DMA, lba, sectorCount);
	}

	ExecuteSataRequest(sreq, isWrite);
}


void
AHCIPort::ScsiUnmap(scsi_ccb* request, scsi_unmap_parameter_list* unmapBlocks)
{
	if (!fTrimSupported || fMaxTrimRangeBlocks == 0) {
		ERROR("TRIM error: Invalid TRIM support values detected\n");
		return;
	}

	// Determine how many blocks are supposed to be trimmed in total
	uint32 scsiRangeCount = (uint16)B_BENDIAN_TO_HOST_INT16(
		unmapBlocks->block_data_length) / sizeof(scsi_unmap_block_descriptor);

#ifdef DEBUG_TRIM
	dprintf("TRIM: AHCI: received a SCSI UNMAP command (blocks):\n");
	for (uint32 i = 0; i < scsiRangeCount; i++) {
		dprintf("[%3" B_PRIu32 "] %" B_PRIu64 " : %" B_PRIu32 "\n", i,
			(uint64)B_BENDIAN_TO_HOST_INT64(unmapBlocks->blocks[i].lba),
			(uint32)B_BENDIAN_TO_HOST_INT32(
				unmapBlocks->blocks[i].block_count));
	}
#endif

	if (scsiRangeCount == 0) {
		request->subsys_status = SCSI_REQ_CMP;
		request->data_resid = 0;
		request->device_status = SCSI_STATUS_GOOD;
		gSCSI->finished(request, 1);
		return;
	}

	size_t lbaRangeCount = 0;
	for (uint32 i = 0; i < scsiRangeCount; i++) {
		uint32 range
			= B_BENDIAN_TO_HOST_INT32(unmapBlocks->blocks[i].block_count);
		lbaRangeCount += range / DSM_MAX_RANGE_VALUE;
		if (range % DSM_MAX_RANGE_VALUE != 0)
			lbaRangeCount++;
	}
	TRACE("Total number of ATA ranges: %" B_PRIuSIZE "\n", lbaRangeCount);

	size_t lbaRangesAllocatedSize = lbaRangeCount * sizeof(uint64);
	// Request data is transferred in 512-byte blocks
	if (lbaRangesAllocatedSize % 512 != 0) {
		lbaRangesAllocatedSize += 512 - (lbaRangesAllocatedSize % 512);
	}
	// Apply reported device limits
	if (lbaRangesAllocatedSize > (size_t)fMaxTrimRangeBlocks * 512) {
		lbaRangesAllocatedSize = (size_t)fMaxTrimRangeBlocks * 512;
	}
	// Allocate a single buffer and re-use it between requests
	TRACE("Allocating a %" B_PRIuSIZE "-byte buffer for ATA request ranges\n",
		lbaRangesAllocatedSize);
	uint64* lbaRanges = (uint64*)malloc(lbaRangesAllocatedSize);
	if (lbaRanges == NULL) {
		ERROR("out of memory when allocating space for %" B_PRIuSIZE
			" unmap ranges\n", lbaRangesAllocatedSize / sizeof(uint64));
		request->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(request, 1);
		return;
	}

	MemoryDeleter deleter(lbaRanges);

	memset(lbaRanges, 0, lbaRangesAllocatedSize);
		// Entries with range length of 0 will be ignored
	uint32 lbaIndex = 0;
	for (uint32 i = 0; i < scsiRangeCount; i++) {
		uint64 lba = B_BENDIAN_TO_HOST_INT64(unmapBlocks->blocks[i].lba);
		uint64 length = (uint32)B_BENDIAN_TO_HOST_INT32(
			unmapBlocks->blocks[i].block_count);

		if (length == 0)
			continue; // Length of 0 would be ignored by the device anyway

		if (lba > DSM_MAX_LBA_VALUE) {
			ERROR("LBA value is too large!"
				" This unmap range will be skipped.\n");
			continue;
		}

		// Split large ranges if needed.
		// Range length is limited by:
		//   - max value of the range field (DSM_MAX_RANGE_VALUE)
		while (length > 0) {
			uint64 ataRange = min_c(length, DSM_MAX_RANGE_VALUE);
			lbaRanges[lbaIndex++]
				= B_HOST_TO_LENDIAN_INT64((ataRange << 48) | lba);

			// Split into multiple requests if needed.
			// The number of entries in a request is limited by:
			//   - the maximum number of 512-byte blocks reported by the device
			//   - maximum possible value of the COUNT field
			//   - the size of our buffer
			if (lbaIndex >= fMaxTrimRangeBlocks * DSM_RANGE_BLOCK_ENTRIES
				|| (((lbaIndex + 1) * sizeof(uint64) + 511) / 512)
					> DSM_MAX_COUNT_48
				|| lbaIndex >= lbaRangesAllocatedSize / sizeof(uint64)
				|| (i == scsiRangeCount - 1 && length <= DSM_MAX_RANGE_VALUE))
			{
				uint32 lbaRangeCount = lbaIndex;
				if (lbaRangeCount % DSM_RANGE_BLOCK_ENTRIES != 0)
					lbaRangeCount += DSM_RANGE_BLOCK_ENTRIES
						- (lbaRangeCount % DSM_RANGE_BLOCK_ENTRIES);
				uint32 lbaRangesSize = lbaRangeCount * sizeof(uint64);

#ifdef DEBUG_TRIM
				dprintf("TRIM: AHCI: sending a DATA SET MANAGEMENT command"
					" to the device (blocks):\n");
				for (uint32 i = 0; i < lbaRangeCount; i++) {
					uint64 value = B_LENDIAN_TO_HOST_INT64(lbaRanges[i]);
					dprintf("[%3" B_PRIu32 "] %" B_PRIu64 " : %" B_PRIu64 "\n", i,
						value & (((uint64)1 << 48) - 1), value >> 48);
				}
#endif

				ASSERT(lbaRangesSize % 512 == 0);
				ASSERT(lbaRangesSize <= lbaRangesAllocatedSize);

				sata_request sreq;
				sreq.SetATA48Command(ATA_COMMAND_DATA_SET_MANAGEMENT, 0,
					lbaRangesSize / 512);
				sreq.SetFeature(1);
				sreq.SetData(lbaRanges, lbaRangesSize);

				ExecuteSataRequest(&sreq, true);
				sreq.WaitForCompletion();

				if ((sreq.CompletionStatus() & ATA_STATUS_ERROR) != 0) {
					ERROR("trim failed (%" B_PRIu32
						" ATA ranges)!\n", lbaRangeCount);
					request->subsys_status = SCSI_REQ_CMP_ERR;
					request->device_status = SCSI_STATUS_CHECK_CONDITION;
					gSCSI->finished(request, 1);
					return;
				} else
					request->subsys_status = SCSI_REQ_CMP;

				lbaIndex = 0;
				memset(lbaRanges, 0, lbaRangesSize);
			}

			length -= ataRange;
			lba += ataRange;
		}
	}

	request->data_resid = 0;
	request->device_status = SCSI_STATUS_GOOD;
	gSCSI->finished(request, 1);
}


void
AHCIPort::ExecuteSataRequest(sata_request* request, bool isWrite)
{
	FLOW("ExecuteAtaRequest port %d\n", fIndex);

	StartTransfer();

	int prdEntrys;

	if (request->CCB() && request->CCB()->data_length) {
		FillPrdTable(fPRDTable, &prdEntrys, PRD_TABLE_ENTRY_COUNT,
			request->CCB()->sg_list, request->CCB()->sg_count,
			request->CCB()->data_length);
	} else if (request->Data() && request->Size()) {
		FillPrdTable(fPRDTable, &prdEntrys, PRD_TABLE_ENTRY_COUNT,
			request->Data(), request->Size());
	} else
		prdEntrys = 0;

	FLOW("prdEntrys %d\n", prdEntrys);

	fCommandList->prdtl_flags_cfl = 0;
	fCommandList->cfl = 5; // 20 bytes, length in DWORDS
	memcpy((char*)fCommandTable->cfis, request->FIS(), 20);

	// We some hide messages when the test unit ready active is clear
	// as empty removeable media resets constantly.
	fTestUnitReadyActive = request->IsTestUnitReady();

	if (request->IsATAPI()) {
		// ATAPI PACKET is a 12 or 16 byte SCSI command
		memset((char*)fCommandTable->acmd, 0, 32);
		memcpy((char*)fCommandTable->acmd, request->CCB()->cdb,
			request->CCB()->cdb_length);
		fCommandList->a = 1;
	}

	if (isWrite)
		fCommandList->w = 1;
	fCommandList->prdtl = prdEntrys;
	fCommandList->prdbc = 0;

	if (wait_until_clear(&fRegs->tfd, ATA_STATUS_BUSY | ATA_STATUS_DATA_REQUEST,
			1000000) < B_OK) {
		ERROR("ExecuteAtaRequest port %d: device is busy\n", fIndex);
		PortReset();
		FinishTransfer();
		request->Abort();
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

	FLOW("Port %d sata request flow:\n", fIndex);
	FLOW("  tfd %#x\n", tfd);
	FLOW("  prdbc %ld\n", fCommandList->prdbc);
	FLOW("  ci   0x%08" B_PRIx32 "\n", fRegs->ci);
	FLOW("  is   0x%08" B_PRIx32 "\n", fRegs->is);
	FLOW("  serr 0x%08" B_PRIx32 "\n", fRegs->serr);

/*
	TRACE("ci   0x%08" B_PRIx32 "\n", fRegs->ci);
	TRACE("ie   0x%08" B_PRIx32 "\n", fRegs->ie);
	TRACE("is   0x%08" B_PRIx32 "\n", fRegs->is);
	TRACE("cmd  0x%08" B_PRIx32 "\n", fRegs->cmd);
	TRACE("ssts 0x%08" B_PRIx32 "\n", fRegs->ssts);
	TRACE("sctl 0x%08" B_PRIx32 "\n", fRegs->sctl);
	TRACE("serr 0x%08" B_PRIx32 "\n", fRegs->serr);
	TRACE("sact 0x%08" B_PRIx32 "\n", fRegs->sact);
	TRACE("tfd  0x%08" B_PRIx32 "\n", fRegs->tfd);
*/

	if (fPortReset || status == B_TIMED_OUT) {
		fPortReset = false;
		PortReset();
	}

	size_t bytesTransfered = fCommandList->prdbc;

	FinishTransfer();

	if (status == B_TIMED_OUT) {
		ERROR("ExecuteAtaRequest port %d: device timeout\n", fIndex);
		request->Abort();
		return;
	}

	request->Finish(tfd, bytesTransfered);
}


void
AHCIPort::ScsiExecuteRequest(scsi_ccb* request)
{
//	TRACE("AHCIPort::ScsiExecuteRequest port %d, opcode 0x%02x, length %u\n", fIndex, request->cdb[0], request->cdb_length);

	if (fIsATAPI) {
		bool isWrite = false;
		switch (request->flags & SCSI_DIR_MASK) {
			case SCSI_DIR_NONE:
				ASSERT(request->data_length == 0);
				break;
			case SCSI_DIR_IN:
				break;
			case SCSI_DIR_OUT:
				isWrite = true;
				ASSERT(request->data_length > 0);
				break;
			default:
				panic("CDB has invalid direction mask");
		}

//		TRACE("AHCIPort::ScsiExecuteRequest ATAPI: port %d, opcode 0x%02x, length %u\n", fIndex, request->cdb[0], request->cdb_length);

		sata_request* sreq = new(std::nothrow) sata_request(request);
		if (sreq == NULL) {
			ERROR("out of memory when allocating atapi request\n");
			request->subsys_status = SCSI_REQ_ABORTED;
			gSCSI->finished(request, 1);
			return;
		}

		sreq->SetATAPICommand(request->data_length);
//		uint8* data = (uint8*) sreq->ccb()->cdb;
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
		case SCSI_OP_SERVICE_ACTION_IN:
			if ((request->cdb[1] & 0x1f) == SCSI_SAI_READ_CAPACITY_16)
				ScsiReadCapacity16(request);
			else {
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		case SCSI_OP_SYNCHRONIZE_CACHE:
			ScsiSynchronizeCache(request);
			break;
		case SCSI_OP_READ_6:
		case SCSI_OP_WRITE_6:
		{
			const scsi_cmd_rw_6* cmd = (const scsi_cmd_rw_6*)request->cdb;
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
			const scsi_cmd_rw_10* cmd = (const scsi_cmd_rw_10*)request->cdb;
			uint32 position = B_BENDIAN_TO_HOST_INT32(cmd->lba);
			size_t length = B_BENDIAN_TO_HOST_INT16(cmd->length);
			bool isWrite = request->cdb[0] == SCSI_OP_WRITE_10;
			if (length) {
				ScsiReadWrite(request, position, length, isWrite);
			} else {
				ERROR("AHCIPort::ScsiExecuteRequest error: transfer without "
					"data!\n");
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		}
		case SCSI_OP_READ_12:
		case SCSI_OP_WRITE_12:
		{
			const scsi_cmd_rw_12* cmd = (const scsi_cmd_rw_12*)request->cdb;
			uint32 position = B_BENDIAN_TO_HOST_INT32(cmd->lba);
			size_t length = B_BENDIAN_TO_HOST_INT32(cmd->length);
			bool isWrite = request->cdb[0] == SCSI_OP_WRITE_12;
			if (length) {
				ScsiReadWrite(request, position, length, isWrite);
			} else {
				ERROR("AHCIPort::ScsiExecuteRequest error: transfer without "
					"data!\n");
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		}
		case SCSI_OP_READ_16:
		case SCSI_OP_WRITE_16:
		{
			const scsi_cmd_rw_16* cmd = (const scsi_cmd_rw_16*)request->cdb;
			uint64 position = B_BENDIAN_TO_HOST_INT64(cmd->lba);
			size_t length = B_BENDIAN_TO_HOST_INT32(cmd->length);
			bool isWrite = request->cdb[0] == SCSI_OP_WRITE_16;
			if (length) {
				ScsiReadWrite(request, position, length, isWrite);
			} else {
				ERROR("AHCIPort::ScsiExecuteRequest error: transfer without "
					"data!\n");
				request->subsys_status = SCSI_REQ_INVALID;
				gSCSI->finished(request, 1);
			}
			break;
		}
		case SCSI_OP_UNMAP:
		{
			const scsi_cmd_unmap* cmd = (const scsi_cmd_unmap*)request->cdb;

			if (!fTrimSupported) {
				ERROR("%s port %d: unsupported request opcode 0x%02x\n",
					__func__, fIndex, request->cdb[0]);
				request->subsys_status = SCSI_REQ_ABORTED;
				gSCSI->finished(request, 1);
				break;
			}

			scsi_unmap_parameter_list* unmapBlocks
				= (scsi_unmap_parameter_list*)request->data;
			if (unmapBlocks == NULL
				|| (uint16)B_BENDIAN_TO_HOST_INT16(cmd->length)
					!= request->data_length
				|| (uint16)B_BENDIAN_TO_HOST_INT16(unmapBlocks->data_length)
					!= request->data_length
						- offsetof(scsi_unmap_parameter_list, block_data_length)
				|| (uint16)B_BENDIAN_TO_HOST_INT16(
						unmapBlocks->block_data_length)
					!= request->data_length
						- offsetof(scsi_unmap_parameter_list, blocks)) {
				ERROR("%s port %d: invalid unmap parameter data length\n",
					__func__, fIndex);
				request->subsys_status = SCSI_REQ_ABORTED;
				gSCSI->finished(request, 1);
			} else {
				ScsiUnmap(request, unmapBlocks);
			}
			break;
		}
		default:
			ERROR("AHCIPort::ScsiExecuteRequest port %d unsupported request "
				"opcode 0x%02x\n", fIndex, request->cdb[0]);
			request->subsys_status = SCSI_REQ_ABORTED;
			gSCSI->finished(request, 1);
	}
}


uchar
AHCIPort::ScsiAbortRequest(scsi_ccb* request)
{
	return SCSI_REQ_CMP;
}


uchar
AHCIPort::ScsiTerminateRequest(scsi_ccb* request)
{
	return SCSI_REQ_CMP;
}


uchar
AHCIPort::ScsiResetDevice()
{
	return SCSI_REQ_CMP;
}


void
AHCIPort::ScsiGetRestrictions(bool* isATAPI, bool* noAutoSense,
	uint32* maxBlocks)
{
	*isATAPI = fIsATAPI;
	*noAutoSense = fIsATAPI; // emulated auto sense for ATA, but not ATAPI
	*maxBlocks = fUse48BitCommands ? 65536 : 256;
	TRACE("AHCIPort::ScsiGetRestrictions port %d: isATAPI %d, noAutoSense %d, "
		"maxBlocks %" B_PRIu32 "\n", fIndex, *isATAPI, *noAutoSense,
		*maxBlocks);
}


bool
AHCIPort::Enable()
{
	// Spec v1.3.1, §10.3.1 Start (PxCMD.ST)
	TRACE("%s: port %d\n", __func__, fIndex);

	if ((fRegs->cmd & PORT_CMD_ST) != 0) {
		ERROR("%s: Starting port already running!\n", __func__);
		return false;
	}

	if ((fRegs->cmd & PORT_CMD_FRE) == 0) {
		ERROR("%s: Unable to start port without FRE enabled!\n", __func__);
		return false;
	}

	// Clear DMA engine and wait for completion
	if (wait_until_clear(&fRegs->cmd, PORT_CMD_CR, 500000) < B_OK) {
		ERROR("%s: port %d error DMA engine still running\n", __func__,
			fIndex);
		return false;
	}
	// Start port
	fRegs->cmd |= PORT_CMD_ST;
	FlushPostedWrites();
	return true;
}


bool
AHCIPort::Disable()
{
	TRACE("%s: port %d\n", __func__, fIndex);

	if ((fRegs->cmd & PORT_CMD_ST) == 0) {
		// Port already disabled, carry on.
		TRACE("%s: port %d attempting to disable stopped port.\n",
			__func__, fIndex);
	} else {
		// Disable port
		fRegs->cmd &= ~PORT_CMD_ST;
		FlushPostedWrites();
	}

	// Spec v1.3.1, §10.4.2 Port Reset - assume hung after 500 mil.
	// Clear DMA engine and wait for completion
	if (wait_until_clear(&fRegs->cmd, PORT_CMD_CR, 500000) < B_OK) {
		ERROR("%s: port %d error DMA engine still running\n", __func__,
			fIndex);
		return false;
	}

	return true;
}


void
AHCIPort::_ClearErrorRegister()
{
	// clear error bits
	fRegs->serr = fRegs->serr;
	FlushPostedWrites();
}
