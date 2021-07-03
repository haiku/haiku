/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2008, Marcus Overhagen.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002-2003, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT License.
 */

#include "ATAPrivate.h"


ATADevice::ATADevice(ATAChannel *channel, uint8 index)
	:
	fChannel(channel),
	fRegisterMask(0),
	fUseDMA(channel->UseDMA()),
	fDMAMode(0),
	fDMAFailures(0),
	fTotalSectors(0),
	fBlockSize(512),
	fPhysicalBlockSize(512),
	fBlockOffset(0),
	fIndex(index),
	fUse48Bits(false)
{
	memset(&fInfoBlock, 0, sizeof(fInfoBlock));
	memset(&fTaskFile, 0, sizeof(fTaskFile));
}


ATADevice::~ATADevice()
{
}


status_t
ATADevice::TestUnitReady(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	fRegisterMask = 0;
	fTaskFile.write.command = ATA_COMMAND_GET_MEDIA_STATUS;

	request->SetTimeout(15 * 1000 * 1000);
	status_t result = fChannel->SendRequest(request, ATA_DEVICE_READY_REQUIRED);
	if (result != B_OK) {
		TRACE_ERROR("failed to send test unit ready request\n");
		return result;
	}

	return fChannel->FinishRequest(request, ATA_WAIT_FINISH
		| ATA_DEVICE_READY_REQUIRED, ATA_ERROR_NO_MEDIA | ATA_ERROR_ABORTED
		| ATA_ERROR_MEDIA_CHANGE_REQUESTED | ATA_ERROR_MEDIUM_CHANGED);
}


status_t
ATADevice::SynchronizeCache(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	// we should also ask for FLUSH CACHE support, but everyone denies it
	// (looks like they cheat to gain some performance advantage, but
	//  that's pretty useless: everyone does it...)
	if (!fInfoBlock.write_cache_supported)
		return B_OK;

	fRegisterMask = 0;
	fTaskFile.lba.command
		= fUse48Bits ? ATA_COMMAND_FLUSH_CACHE_EXT : ATA_COMMAND_FLUSH_CACHE;

	request->SetTimeout(60 * 1000 * 1000);
	status_t result = fChannel->SendRequest(request, ATA_DEVICE_READY_REQUIRED);
	if (result != B_OK) {
		TRACE_ERROR("failed to send synchronize cache request\n");
		return result;
	}

	return fChannel->FinishRequest(request, ATA_WAIT_FINISH
		| ATA_DEVICE_READY_REQUIRED, ATA_ERROR_ABORTED);
}


status_t
ATADevice::Eject(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	fRegisterMask = 0;
	fTaskFile.lba.command = ATA_COMMAND_MEDIA_EJECT;

	request->SetTimeout(15 * 1000 * 1000);
	status_t result = fChannel->SendRequest(request, ATA_DEVICE_READY_REQUIRED);
	if (result != B_OK) {
		TRACE_ERROR("failed to send eject request\n");
		return result;
	}

	return fChannel->FinishRequest(request, ATA_WAIT_FINISH
		| ATA_DEVICE_READY_REQUIRED, ATA_ERROR_ABORTED | ATA_ERROR_NO_MEDIA);
}


status_t
ATADevice::Inquiry(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	scsi_ccb *ccb = request->CCB();
	scsi_cmd_inquiry *command = (scsi_cmd_inquiry *)ccb->cdb;
	if (command->evpd || command->page_code) {
		request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return B_ERROR;
	}

	scsi_res_inquiry data;
	memset(&data, 0, sizeof(data));

	data.device_type = IsATAPI()
		? fInfoBlock.word_0.atapi.command_packet_set : scsi_dev_direct_access;
	data.device_qualifier = scsi_periph_qual_connected;

	data.device_type_modifier = 0;
	data.removable_medium = fInfoBlock.word_0.ata.removable_media_device;

	data.ansi_version = 2;
	data.ecma_version = 0;
	data.iso_version = 0;

	data.response_data_format = 2;
	data.term_iop = false;
		// to be changed if we support TERM I/O

	data.additional_length = sizeof(scsi_res_inquiry) - 4;

	data.soft_reset = false;
	data.cmd_queue = 0;
	data.linked = false;

	// these values are free-style
	data.sync = false;
	data.write_bus16 = true;
	data.write_bus32 = false;

	data.relative_address = false;

	// the following fields are *much* to small, sigh...
	memcpy(data.vendor_ident, fInfoBlock.model_number,
		sizeof(data.vendor_ident));
	swap_words(data.vendor_ident, sizeof(data.vendor_ident));

	memcpy(data.product_ident, fInfoBlock.model_number + 8,
		sizeof(data.product_ident));
	swap_words(data.product_ident, sizeof(data.product_ident));

	memcpy(data.product_rev, "    ", sizeof(data.product_rev));

	uint32 allocationLength = command->allocation_length;
	copy_sg_data(ccb, 0, allocationLength, &data, sizeof(data), false);
	ccb->data_resid = ccb->data_length - MIN(MIN(sizeof(data),
		allocationLength), ccb->data_length);
	return B_OK;
}


status_t
ATADevice::ReadCapacity(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	scsi_ccb *ccb = request->CCB();
	scsi_cmd_read_capacity *command = (scsi_cmd_read_capacity *)ccb->cdb;
	if (command->pmi || command->lba) {
		request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return B_ERROR;
	}

	scsi_res_read_capacity data;
	memset(&data, 0, sizeof(data));

	data.block_size = B_HOST_TO_BENDIAN_INT32(fBlockSize);

	if (fTotalSectors <= UINT_MAX) {
		uint32 lastBlock = fTotalSectors - 1;
		data.lba = B_HOST_TO_BENDIAN_INT32(lastBlock);
	} else
		data.lba = UINT_MAX;
	TRACE("returning last block: %" B_PRIu32 "\n",
		B_BENDIAN_TO_HOST_INT32(data.lba));

	copy_sg_data(ccb, 0, ccb->data_length, &data, sizeof(data), false);
	ccb->data_resid = MAX(ccb->data_length - sizeof(data), 0);
	return B_OK;
}


status_t
ATADevice::ReadCapacity16(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	scsi_ccb *ccb = request->CCB();
	scsi_cmd_read_capacity_long *command
		= (scsi_cmd_read_capacity_long *)ccb->cdb;
	if (command->pmi || command->lba) {
		request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return B_ERROR;
	}

	uint32 allocationLength = B_BENDIAN_TO_HOST_INT32(command->alloc_length);

	scsi_res_read_capacity_long data;
	memset(&data, 0, sizeof(data));

	data.block_size = B_HOST_TO_BENDIAN_INT32(fBlockSize);

	uint64 lastBlock = fTotalSectors - 1;
	data.lba = B_HOST_TO_BENDIAN_INT64(lastBlock);
	TRACE("returning last block: %" B_PRIu64 "\n",
		B_BENDIAN_TO_HOST_INT64(data.lba));

	size_t copySize = min_c(allocationLength, sizeof(data));

	copy_sg_data(ccb, 0, ccb->data_length, &data, copySize, false);
	ccb->data_resid = MAX(ccb->data_length - copySize, 0);
	return B_OK;
}


status_t
ATADevice::ExecuteIO(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	scsi_ccb *ccb = request->CCB();
	request->SetDevice(this);

	// ATA devices have one LUN only
	if (ccb->target_lun != 0) {
		TRACE_ERROR("invalid target lun %d for ATA device\n", ccb->target_lun);
		request->SetStatus(SCSI_SEL_TIMEOUT);
		return B_BAD_INDEX;
	}

	TRACE("request: 0x%02x\n", ccb->cdb[0]);

	switch (ccb->cdb[0]) {
		case SCSI_OP_TEST_UNIT_READY:
			return TestUnitReady(request);

		case SCSI_OP_FORMAT: /* FORMAT UNIT */
			// we could forward ccb to disk, but modern disks cannot
			// be formatted anyway, so we just refuse ccb
			// (exceptions are removable media devices, but to my knowledge
			// they don't have to be formatted as well)
			request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			return B_ERROR;

		case SCSI_OP_INQUIRY:
 			return Inquiry(request);

		case SCSI_OP_START_STOP:
		{
			scsi_cmd_ssu *command = (scsi_cmd_ssu *)ccb->cdb;

			// with no LoEj bit set, we should only allow/deny further access
			// we ignore that (unsupported for ATA)
			// with LoEj bit set, we should additionally either load or eject
			// the medium (start = 0 - eject; start = 1 - load)

			if (!command->start) {
				// we must always flush cache if start = 0
				SynchronizeCache(request);
			}

			if (command->load_eject) {
				if (!command->start)
					return Eject(request);
				else {
					request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST,
						SCSIS_ASC_PARAM_NOT_SUPPORTED);
					return B_ERROR;
				}
			}

			return B_OK;
		}

		case SCSI_OP_READ_CAPACITY:
			return ReadCapacity(request);

		case SCSI_OP_SERVICE_ACTION_IN:
			if ((ccb->cdb[1] & 0x1f) == SCSI_SAI_READ_CAPACITY_16)
				return ReadCapacity16(request);
			break;

		case SCSI_OP_SYNCHRONIZE_CACHE:
			// we ignore range and immediate bit, we always immediately
			// flush everything
			return SynchronizeCache(request);

		// sadly, there are two possible read/write operation codes;
		// at least, the third one, read/write(12), is not valid for DAS
		case SCSI_OP_READ_6:
		case SCSI_OP_WRITE_6:
		{
			scsi_cmd_rw_6 *command = (scsi_cmd_rw_6 *)ccb->cdb;
			uint32 address = ((uint32)command->high_lba << 16)
				| ((uint32)command->mid_lba << 8) | (uint32)command->low_lba;

			request->SetIsWrite(command->opcode == SCSI_OP_WRITE_6);
			return ExecuteReadWrite(request, address, command->length != 0
				? command->length : 256);
		}

		case SCSI_OP_READ_10:
		case SCSI_OP_WRITE_10:
		{
			scsi_cmd_rw_10 *command = (scsi_cmd_rw_10 *)ccb->cdb;
			uint32 address = B_BENDIAN_TO_HOST_INT32(command->lba);
			uint32 sectorCount = B_BENDIAN_TO_HOST_INT16(command->length);

			request->SetIsWrite(command->opcode == SCSI_OP_WRITE_10);
			if (sectorCount > 0)
				return ExecuteReadWrite(request, address, sectorCount);
			else {
				// we cannot transfer zero blocks (apart from LBA48)
				request->SetStatus(SCSI_REQ_CMP);
				return B_OK;
			}
		}

		case SCSI_OP_READ_12:
		case SCSI_OP_WRITE_12:
		{
			scsi_cmd_rw_12 *command = (scsi_cmd_rw_12 *)ccb->cdb;
			uint32 address = B_BENDIAN_TO_HOST_INT32(command->lba);
			uint32 sectorCount = B_BENDIAN_TO_HOST_INT32(command->length);

			request->SetIsWrite(command->opcode == SCSI_OP_WRITE_12);
			if (sectorCount > 0)
				return ExecuteReadWrite(request, address, sectorCount);
			else {
				// we cannot transfer zero blocks (apart from LBA48)
				request->SetStatus(SCSI_REQ_CMP);
				return B_OK;
			}
		}

		case SCSI_OP_READ_16:
		case SCSI_OP_WRITE_16:
		{
			scsi_cmd_rw_16 *command = (scsi_cmd_rw_16 *)ccb->cdb;
			uint64 address = B_BENDIAN_TO_HOST_INT64(command->lba);
			uint32 sectorCount = B_BENDIAN_TO_HOST_INT32(command->length);

			request->SetIsWrite(command->opcode == SCSI_OP_WRITE_16);
			if (sectorCount > 0)
				return ExecuteReadWrite(request, address, sectorCount);
			else {
				// we cannot transfer zero blocks (apart from LBA48)
				request->SetStatus(SCSI_REQ_CMP);
				return B_OK;
			}
		}
	}

	TRACE("command not implemented\n");
	request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
	return B_ERROR;
}


void
ATADevice::GetRestrictions(bool *noAutoSense, uint32 *maxBlocks)
{
	if (IsATAPI())
		*noAutoSense = true;
	else {
		if (fUse48Bits)
			*maxBlocks = 0xffff;
		else
			*maxBlocks = 0x100;
	}
}


status_t
ATADevice::Control(uint32 opcode, void *buffer, size_t length)
{
	if (opcode == B_GET_DEVICE_NAME) {
		// Swap words
		char name[sizeof(fInfoBlock.model_number)];
		memcpy(name, fInfoBlock.model_number, sizeof(name));
		swap_words(name, sizeof(name));

		// Remove trailing spaces
		int32 nameLength = sizeof(name) - 2;
		while (nameLength > 0 && name[nameLength - 1] == ' ')
			nameLength--;

		// TODO: make string prettier, ie. "WDC" -> "Western Digital", ...
		return user_strlcpy((char*)buffer, name,
			min_c((size_t)nameLength + 1, length)) >= 0 ? B_OK : B_BAD_ADDRESS;
	}
	return B_BAD_VALUE;
}


status_t
ATADevice::Select()
{
	status_t err = fChannel->SelectDevice(fIndex);
#if 1
    // for debugging only
	if (fChannel->SelectedDevice() != fIndex) {
		TRACE_ERROR("device %d not selected!\n", fIndex);
		return B_ERROR;
	}
#endif
	return err;
}


status_t
ATADevice::SetFeature(int feature)
{
	TRACE("device_set_feature: feature %d\n", feature);

	ATARequest request(false);
	request.SetDevice(this);
	request.SetTimeout(1 * 1000 * 1000);

	fTaskFile.write.features = feature;
	fTaskFile.write.command = ATA_COMMAND_SET_FEATURES;
	fRegisterMask = ATA_MASK_FEATURES;

	status_t result = fChannel->SendRequest(&request, ATA_DEVICE_READY_REQUIRED);
	if (result != B_OK) {
		TRACE_ERROR("sending set feature request failed\n");
		return result;
	}

	result = fChannel->FinishRequest(&request,
		ATA_WAIT_FINISH | ATA_DEVICE_READY_REQUIRED, ATA_ERROR_ABORTED);
	if (result != B_OK) {
		TRACE_ERROR("set feature request failed\n");
		return result;
	}

	return B_OK;
}


status_t
ATADevice::DisableCommandQueueing()
{
	if (!fInfoBlock.read_write_dma_queued_supported)
		return B_OK;

	if (fInfoBlock.release_interrupt_supported) {
		status_t result = SetFeature(
			ATA_COMMAND_SET_FEATURES_DISABLE_RELEASE_INT);
		if (result != B_OK) {
			TRACE_ERROR("failed to disable release interrupt\n");
			return result;
		}
	}

	if (fInfoBlock.service_interrupt_supported) {
		status_t result = SetFeature(
			ATA_COMMAND_SET_FEATURES_DISABLE_SERVICE_INT);
		if (result != B_OK) {
			TRACE_ERROR("failed to disable service interrupt\n");
			return result;
		}
	}

	return B_OK;
}


status_t
ATADevice::ConfigureDMA()
{
	if (!fUseDMA)
		return B_OK;

	if (!fInfoBlock.dma_supported) {
		TRACE_ALWAYS("DMA not supported by device\n");
		fUseDMA = false;
		return B_OK;
	}

	#define CHECK_DMA_MODE(element, mode) \
		if (fInfoBlock.element) { \
			fDMAMode = mode; \
			modeCount++; \
		}

	uint32 modeCount = 0;

	CHECK_DMA_MODE(multiword_dma_0_selected, 0x00);
	CHECK_DMA_MODE(multiword_dma_1_selected, 0x01);
	CHECK_DMA_MODE(multiword_dma_2_selected, 0x02);

	if (fInfoBlock.word_88_valid) {
		CHECK_DMA_MODE(ultra_dma_0_selected, 0x10);
		CHECK_DMA_MODE(ultra_dma_1_selected, 0x11);
		CHECK_DMA_MODE(ultra_dma_2_selected, 0x12);
		CHECK_DMA_MODE(ultra_dma_3_selected, 0x13);
		CHECK_DMA_MODE(ultra_dma_4_selected, 0x14);
		CHECK_DMA_MODE(ultra_dma_5_selected, 0x15);
		CHECK_DMA_MODE(ultra_dma_6_selected, 0x16);
	}

	#undef CHECK_DMA_MODE

	if (modeCount != 1) {
		TRACE_ERROR("more than one DMA mode selected, not using DMA\n");
		fUseDMA = false;
		return B_OK;
	}

	TRACE_ALWAYS("using DMA mode 0x%02x\n", fDMAMode);
	return B_OK;
}


status_t
ATADevice::Configure()
{
	// warning: ata == 0 means "this is ata"...
	if (fInfoBlock.word_0.ata.ata_device != ATA_WORD_0_ATA_DEVICE) {
		// CF has either magic header or CFA bit set
		// we merge it to "CFA bit set" for easier (later) testing
		if (fInfoBlock.word_0.raw == ATA_WORD_0_CFA_MAGIC)
			fInfoBlock.compact_flash_assoc_supported = true;
		else {
			TRACE_ERROR("infoblock indicates non-ata device\n");
			return B_ERROR;
		}
	}

	if (!fInfoBlock.lba_supported || (fInfoBlock.lba_sector_count == 0
		&& fInfoBlock.lba48_sector_count == 0)) {
		TRACE_ERROR("non-lba devices not supported\n");
		return B_ERROR;
	}

	fTotalSectors = fInfoBlock.SectorCount(fUse48Bits, false);
	fBlockSize = fInfoBlock.SectorSize();
	fPhysicalBlockSize = fInfoBlock.PhysicalSectorSize();
	fBlockOffset = fInfoBlock.BlockOffset();

	fTaskFile.lba.mode = ATA_MODE_LBA;
	fTaskFile.lba.device = fIndex;

	status_t result = ConfigureDMA();
	if (result != B_OK)
		return result;

	result = DisableCommandQueueing();
	if (result != B_OK)
		return result;

	return B_OK;
}


status_t
ATADevice::Identify()
{
	snprintf(fDebugContext, sizeof(fDebugContext), "%s %" B_PRIu32 "-%u",
		IsATAPI() ? "pi" : "", fChannel->ChannelID(), fIndex);

	ATARequest request(false);
	request.SetDevice(this);
	request.SetTimeout(20 * 1000 * 1000);

	fRegisterMask = 0;
	fTaskFile.write.command = IsATAPI() ? ATA_COMMAND_IDENTIFY_PACKET_DEVICE
		: ATA_COMMAND_IDENTIFY_DEVICE;

	if (fChannel->SendRequest(&request,
			IsATAPI() ? 0 : ATA_DEVICE_READY_REQUIRED) != B_OK) {
		TRACE_ERROR("sending identify request failed\n");
		return B_ERROR;
	}

	if (fChannel->Wait(ATA_STATUS_BUSY | ATA_STATUS_DATA_REQUEST, 0,
			ATA_WAIT_ANY_BIT, 100 * 1000) != B_OK) {
		TRACE_ALWAYS("no data request and not busy within 100ms, assuming "
			"no device present\n");
		return B_TIMED_OUT;
	}

	if (fChannel->Wait(ATA_STATUS_DATA_REQUEST, ATA_STATUS_BUSY,
			ATA_CHECK_ERROR_BIT | ATA_CHECK_DEVICE_FAULT,
			IsATAPI() ? 20 * 1000 * 1000 : 500 * 1000) != B_OK) {
		TRACE_ERROR("timeout waiting for identify request\n");
		return B_TIMED_OUT;
	}

	// get the infoblock
	fChannel->ReadPIO((uint8 *)&fInfoBlock, sizeof(fInfoBlock));

	if (fChannel->WaitDataRequest(false) != B_OK) {
		TRACE_ERROR("device disagrees on info block length\n");
		return B_ERROR;
	}

	if (fChannel->FinishRequest(&request,
			ATA_WAIT_FINISH | (IsATAPI() ? 0 : ATA_DEVICE_READY_REQUIRED),
			ATA_ERROR_ABORTED) != B_OK) {
		TRACE_ERROR("failed to finish identify request\n");
		return B_ERROR;
	}

	if (1) {
		// print device information
		char modelNumber[sizeof(fInfoBlock.model_number) + 1];
		char serialNumber[sizeof(fInfoBlock.serial_number) + 1];
		char firmwareRev[sizeof(fInfoBlock.firmware_revision) + 1];
		strlcpy(modelNumber, fInfoBlock.model_number, sizeof(modelNumber));
		strlcpy(serialNumber, fInfoBlock.serial_number, sizeof(serialNumber));
		strlcpy(firmwareRev, fInfoBlock.firmware_revision, sizeof(firmwareRev));
		swap_words(modelNumber, sizeof(modelNumber) - 1);
		swap_words(serialNumber, sizeof(serialNumber) - 1);
		swap_words(firmwareRev, sizeof(firmwareRev) - 1);
		TRACE_ALWAYS("model number: %s\n", modelNumber);
		TRACE_ALWAYS("serial number: %s\n", serialNumber);
  		TRACE_ALWAYS("firmware rev.: %s\n", firmwareRev);
	}

	return B_OK;
}


status_t
ATADevice::ExecuteReadWrite(ATARequest *request, uint64 address,
	uint32 sectorCount)
{
	request->SetUseDMA(fUseDMA && fChannel->PrepareDMA(request) == B_OK);
	if (!request->UseDMA())
		request->PrepareSGInfo();

	request->SetBytesLeft(sectorCount * fBlockSize);
	if (_FillTaskFile(request, address) != B_OK) {
		TRACE_ERROR("failed to setup transfer request\n");
		if (request->UseDMA())
			fChannel->FinishDMA();
		return B_ERROR;
	}

	status_t result = fChannel->SendRequest(request,
		IsATAPI() ? 0 : ATA_DEVICE_READY_REQUIRED);
	if (result != B_OK) {
		TRACE_ERROR("failed to send transfer request\n");
		if (request->UseDMA())
			fChannel->FinishDMA();
		return result;
	}

	if (request->UseDMA()) {
		fChannel->PrepareWaitingForInterrupt();
		fChannel->StartDMA();

		result = fChannel->WaitForInterrupt(request->Timeout());
		status_t dmaResult = fChannel->FinishDMA();
		if (result == B_OK && dmaResult == B_OK) {
			fDMAFailures = 0;
			request->CCB()->data_resid = 0;
		} else {
			if (dmaResult != B_OK) {
				request->SetSense(SCSIS_KEY_HARDWARE_ERROR,
					SCSIS_ASC_LUN_COM_FAILURE);
				fDMAFailures++;
				if (fDMAFailures >= ATA_MAX_DMA_FAILURES) {
					TRACE_ALWAYS("disabling DMA after %u failures\n",
						fDMAFailures);
					fUseDMA = false;
				}
			} else {
				// timeout
				request->SetStatus(SCSI_CMD_TIMEOUT);
			}
		}
	} else {
		if (fChannel->Wait(ATA_STATUS_DATA_REQUEST, 0, ATA_CHECK_ERROR_BIT
				| ATA_CHECK_DEVICE_FAULT, request->Timeout()) != B_OK) {
			TRACE_ERROR("timeout waiting for device to request data\n");
			request->SetStatus(SCSI_CMD_TIMEOUT);
			return B_TIMED_OUT;
		}

		if (fChannel->ExecutePIOTransfer(request) != B_OK) {
			TRACE_ERROR("executing pio transfer failed\n");
			request->SetStatus(SCSI_SEQUENCE_FAIL);
		}
	}

	return fChannel->FinishRequest(request, ATA_WAIT_FINISH
		| ATA_DEVICE_READY_REQUIRED, ATA_ERROR_ALL);
}


status_t
ATADevice::_FillTaskFile(ATARequest *request, uint64 address)
{
	// list of LBA48 opcodes
	static const uint8 s48BitCommands[2][2] = {
		{ ATA_COMMAND_READ_SECTORS_EXT, ATA_COMMAND_WRITE_SECTORS_EXT },
		{ ATA_COMMAND_READ_DMA_EXT, ATA_COMMAND_WRITE_DMA_EXT }
	};

	// list of normal LBA opcodes
	static const uint8 s28BitCommands[2][2] = {
		{ ATA_COMMAND_READ_SECTORS, ATA_COMMAND_WRITE_SECTORS },
		{ ATA_COMMAND_READ_DMA, ATA_COMMAND_WRITE_DMA }
	};

	uint32 sectorCount = *request->BytesLeft() / fBlockSize;
	TRACE("about to transfer %lu sectors\n", sectorCount);

	if (fUse48Bits
		&& (address + sectorCount > 0xfffffff || sectorCount > 0x100)) {
		// use LBA48 only if necessary
		if (sectorCount > 0xffff) {
			TRACE_ERROR("invalid sector count %lu\n", sectorCount);
			request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST,
				SCSIS_ASC_INV_CDB_FIELD);
			return B_ERROR;
		}

		fRegisterMask = ATA_MASK_SECTOR_COUNT_48
			| ATA_MASK_LBA_LOW_48
			| ATA_MASK_LBA_MID_48
			| ATA_MASK_LBA_HIGH_48;

		fTaskFile.lba48.sector_count_0_7 = sectorCount & 0xff;
		fTaskFile.lba48.sector_count_8_15 = (sectorCount >> 8) & 0xff;
		fTaskFile.lba48.lba_0_7 = address & 0xff;
		fTaskFile.lba48.lba_8_15 = (address >> 8) & 0xff;
		fTaskFile.lba48.lba_16_23 = (address >> 16) & 0xff;
		fTaskFile.lba48.lba_24_31 = (address >> 24) & 0xff;
		fTaskFile.lba48.lba_32_39 = (address >> 32) & 0xff;
		fTaskFile.lba48.lba_40_47 = (address >> 40) & 0xff;
		fTaskFile.lba48.command = s48BitCommands[request->UseDMA()
			? 1 : 0][request->IsWrite() ? 1 : 0];
	} else {
		// normal LBA
		if (sectorCount > 0x100) {
			TRACE_ERROR("invalid sector count %lu\n", sectorCount);
			request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST,
				SCSIS_ASC_INV_CDB_FIELD);
			return B_ERROR;
		}

		fRegisterMask = ATA_MASK_SECTOR_COUNT
			| ATA_MASK_LBA_LOW
			| ATA_MASK_LBA_MID
			| ATA_MASK_LBA_HIGH
			| ATA_MASK_DEVICE_HEAD;

		fTaskFile.lba.sector_count = sectorCount & 0xff;
		fTaskFile.lba.lba_0_7 = address & 0xff;
		fTaskFile.lba.lba_8_15 = (address >> 8) & 0xff;
		fTaskFile.lba.lba_16_23 = (address >> 16) & 0xff;
		fTaskFile.lba.lba_24_27 = (address >> 24) & 0xf;
		fTaskFile.lba.command = s28BitCommands[request->UseDMA()
			? 1 : 0][request->IsWrite() ? 1 : 0];
	}

	return B_OK;
}
