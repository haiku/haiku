/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2008, Marcus Overhagen.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002-2003, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT License.
 */

#include "ATAPrivate.h"

ATAPIDevice::ATAPIDevice(ATAChannel *channel, uint8 index)
	:	ATADevice(channel, index)
{
}


ATAPIDevice::~ATAPIDevice()
{
}


status_t
ATAPIDevice::SendPacket(ATARequest *request)
{
	TRACE_FUNCTION("%p\n", request);

	// only READ/WRITE commands can use DMA
	// (the device may support it always, but IDE controllers don't
	// report how much data is transmitted, and this information is
	// crucial for the SCSI protocol)
	// special offer: let READ_CD commands use DMA too
	uint8 command = fPacket[0];
	request->SetUseDMA(UseDMA()
		&& (command == SCSI_OP_READ_6 || command == SCSI_OP_WRITE_6
		|| command == SCSI_OP_READ_10 || command == SCSI_OP_WRITE_10
		|| command == SCSI_OP_READ_12 || command == SCSI_OP_WRITE_12
		|| command == SCSI_OP_READ_CD)
		&& fChannel->PrepareDMA(request) == B_OK);
	TRACE("using dma: %s\n", request->UseDMA() ? "yes" : "no");

	if (!request->UseDMA())
		request->PrepareSGInfo();

	if (_FillTaskFilePacket(request) != B_OK) {
		TRACE_ERROR("failed to setup transfer request\n");
		if (request->UseDMA())
			fChannel->FinishDMA();
		return B_ERROR;
	}

	status_t result = fChannel->SendRequest(request, 0);
	if (result != B_OK) {
		TRACE_ERROR("failed to send packet request\n");
		if (request->UseDMA())
			fChannel->FinishDMA();
		return result;
	}

	// wait for device to get ready for packet transmission
	if (fChannel->Wait(ATA_STATUS_DATA_REQUEST, ATA_STATUS_BUSY,
		ATA_CHECK_ERROR_BIT | ATA_CHECK_DEVICE_FAULT, 100 * 1000) != B_OK) {
		TRACE_ERROR("timeout waiting for data request\n");
		if (request->UseDMA())
			fChannel->FinishDMA();

		request->SetStatus(SCSI_SEQUENCE_FAIL);
		return B_TIMED_OUT;
	}

	// make sure device really asks for command packet
	fRegisterMask = ATA_MASK_INTERRUPT_REASON;
	fChannel->ReadRegs(this);

	if (!fTaskFile.packet_res.cmd_or_data
		|| fTaskFile.packet_res.input_or_output) {
		TRACE_ERROR("device doesn't ask for packet\n");
		if (request->UseDMA())
			fChannel->FinishDMA();

		request->SetStatus(SCSI_SEQUENCE_FAIL);
		return B_ERROR;
	}

	// some old drives need a delay before submitting the packet
	spin(10);

	// write packet
	if (fChannel->WritePIO(fPacket, sizeof(fPacket)) != B_OK) {
		TRACE_ERROR("failed to write packet\n");
		if (request->UseDMA())
			fChannel->FinishDMA();

		request->SetStatus(SCSI_HBA_ERR);
		return B_ERROR;
	}

	if (!request->HasData())
		return _FinishRequest(request, ATA_WAIT_FINISH);

	if (request->UseDMA()) {
		fChannel->PrepareWaitingForInterrupt();
		fChannel->StartDMA();

		result = fChannel->WaitForInterrupt(request->Timeout());
		status_t dmaResult = fChannel->FinishDMA();
		if (result != B_OK) {
			request->SetStatus(SCSI_CMD_TIMEOUT);
			return B_TIMED_OUT;
		}

		result = _FinishRequest(request, ATA_WAIT_FINISH);
		if (result != B_OK) {
			TRACE_ERROR("device indicates transfer error after dma\n");
			return result;
		}

		// for ATAPI it's ok for the device to send too much
		if (dmaResult == B_OK || dmaResult == B_DEV_DATA_OVERRUN) {
			fDMAFailures = 0;
			request->CCB()->data_resid = 0;
			return B_OK;
		}

		TRACE_ERROR("dma transfer failed\n");
		request->SetSense(SCSIS_KEY_HARDWARE_ERROR,
			SCSIS_ASC_LUN_COM_FAILURE);
		fDMAFailures++;
		if (fDMAFailures >= ATA_MAX_DMA_FAILURES) {
			TRACE_ALWAYS("disabling DMA after %u failures\n", fDMAFailures);
			fUseDMA = false;
		}

		return B_ERROR;
	}

	result = fChannel->Wait(ATA_STATUS_DATA_REQUEST, ATA_STATUS_BUSY,
		ATA_CHECK_ERROR_BIT | ATA_CHECK_DEVICE_FAULT, request->Timeout());
	if (result != B_OK) {
		if (result == B_TIMED_OUT) {
			TRACE_ERROR("timeout waiting for device to request data\n");
			request->SetStatus(SCSI_CMD_TIMEOUT);
			return B_TIMED_OUT;
		} else
			return _FinishRequest(request, 0);
	}

	// PIO data transfer
	while (true) {
		fRegisterMask = ATA_MASK_INTERRUPT_REASON | ATA_MASK_BYTE_COUNT;
		fChannel->ReadRegs(this);

		if (fTaskFile.packet_res.cmd_or_data) {
			TRACE_ERROR("device expecting command instead of data\n");
			request->SetStatus(SCSI_SEQUENCE_FAIL);
			return B_ERROR;
		}

		size_t length = fTaskFile.packet_res.byte_count_0_7
			| ((size_t)fTaskFile.packet_res.byte_count_8_15 << 8);
		TRACE("about to transfer %lu bytes\n", length);

		request->SetBytesLeft(length);
		fChannel->ExecutePIOTransfer(request);

		result = fChannel->Wait(0, ATA_STATUS_BUSY, 0, request->Timeout());
		if (result != B_OK) {
			if (result == B_TIMED_OUT) {
				TRACE_ERROR("timeout waiting for device to finish transfer\n");
				request->SetStatus(SCSI_CMD_TIMEOUT);
				return B_TIMED_OUT;
			} else
				return _FinishRequest(request, 0);
		}

		if ((fChannel->AltStatus() & ATA_STATUS_DATA_REQUEST) == 0) {
			// transfer complete
			TRACE("pio transfer complete\n");
			break;
		}
	}

	return _FinishRequest(request, ATA_WAIT_FINISH);
}


status_t
ATAPIDevice::ExecuteIO(ATARequest *request)
{
	scsi_ccb *ccb = request->CCB();
	if (ccb->target_lun != 0) {
		TRACE_ERROR("invalid target lun %d\n", ccb->target_lun);
		request->SetStatus(SCSI_SEL_TIMEOUT);
		return B_BAD_INDEX;
	}

	// ATAPI command packets are 12 bytes long;
	// if the command is shorter, remaining bytes must be padded with zeros
	memset(fPacket, 0, sizeof(fPacket));
	memcpy(fPacket, ccb->cdb, ccb->cdb_length);

	request->SetDevice(this);
	request->SetIsWrite((ccb->flags & SCSI_DIR_MASK) == SCSI_DIR_OUT);
	return SendPacket(request);
}


status_t
ATAPIDevice::Configure()
{
	if (fInfoBlock.word_0.atapi.atapi_device != ATA_WORD_0_ATAPI_DEVICE) {
		TRACE_ERROR("infoblock indicates non-atapi device\n");
		return B_ERROR;
	}

	fTaskFile.packet.lun = 0;

	status_t result = ConfigureDMA();
	if (result != B_OK)
		return result;

	result = DisableCommandQueueing();
	if (result != B_OK)
		return result;

	return B_OK;
}


status_t
ATAPIDevice::_FillTaskFilePacket(ATARequest *request)
{
	scsi_ccb *ccb = request->CCB();
	fRegisterMask = ATA_MASK_FEATURES | ATA_MASK_BYTE_COUNT;
	fTaskFile.packet.dma = request->UseDMA() ? 1 : 0;
	fTaskFile.packet.ovl = 0;
	fTaskFile.packet.byte_count_0_7 = ccb->data_length & 0xff;
	fTaskFile.packet.byte_count_8_15 = ccb->data_length >> 8;
	fTaskFile.packet.command = ATA_COMMAND_PACKET;
	return B_OK;
}


status_t
ATAPIDevice::_FinishRequest(ATARequest *request, uint32 flags)
{
	if (fChannel->FinishRequest(request, flags
		| ATA_CHECK_DEVICE_FAULT, 0) != B_OK) {
		// when we get an error from a packet device, we instruct the
		// scsi layer to do a request sense. but since we don't want to
		// return an emulated sense coming from ata, we clear our sense
		// key first so that the next request sense will go to the packet
		// device directly (as a packet command).
		request->ClearSense();
		request->SetStatus(SCSI_REQ_CMP_ERR);
		request->CCB()->device_status = SCSI_STATUS_CHECK_CONDITION;
		return B_ERROR;
	}

	return B_OK;
}
