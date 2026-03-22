/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "VMBusDevicePrivate.h"


VMBusDevice::VMBusDevice(device_node* node)
	:
	fNode(node),
	fStatus(B_NO_INIT),
	fChannelID(0),
	fReferenceCounterSupported(false),
	fDPCHandle(NULL),
	fIsOpen(false),
	fRingGPADL(0),
	fRingBuffer(NULL),
	fRingBufferLength(0),
	fTXRing(NULL),
	fTXRingLength(0),
	fRXRing(NULL),
	fRXRingLength(0),
	fCallback(NULL),
	fCallbackData(NULL),
	fVMBus(NULL),
	fVMBusCookie(NULL)
{
	CALLED();

	fStatus = gDeviceManager->get_attr_uint32(fNode, HYPERV_CHANNEL_ID_ITEM, &fChannelID, false);
	if (fStatus != B_OK) {
		ERROR("Failed to get channel ID\n");
		return;
	}

	device_node* parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, (driver_module_info**)&fVMBus, (void**)&fVMBusCookie);
	gDeviceManager->put_node(parent);

	mutex_init(&fLock, "vmbus device lock");
	B_INITIALIZE_SPINLOCK(&fTXLock);
	B_INITIALIZE_SPINLOCK(&fRXLock);

	fReferenceCounterSupported = _IsReferenceCounterSupported();
	TRACE("Reference counter: %s\n", fReferenceCounterSupported ? "yes" : "no");
}


VMBusDevice::~VMBusDevice()
{
	CALLED();

	mutex_destroy(&fLock);
	if (fDPCHandle != NULL)
		gDPC->delete_dpc_queue(fDPCHandle);
}


uint32
VMBusDevice::GetBusVersion()
{
	return fVMBus->get_version(fVMBusCookie);
}


status_t
VMBusDevice::Open(uint32 txLength, uint32 rxLength, hyperv_device_callback callback,
	void* callbackData)
{
	// Ring lengths must be page-aligned.
	if (txLength == 0 || rxLength == 0 || txLength != HV_PAGE_ALIGN(txLength)
			|| rxLength != HV_PAGE_ALIGN(rxLength))
		return B_BAD_VALUE;

	MutexLocker locker(fLock);
	if (fIsOpen)
		return B_BUSY;

	uint32 txTotalLength = sizeof(vmbus_ring_buffer) + txLength;
	uint32 rxTotalLength = sizeof(vmbus_ring_buffer) + rxLength;
	uint32 fRingBufferLength = txTotalLength + rxTotalLength;

	TRACE("Open channel %u tx length 0x%X rx length 0x%X\n", fChannelID, txLength, rxLength);

	// Create the GPADL used for the ring buffers
	status_t status = AllocateGPADL(fRingBufferLength, &fRingBuffer, &fRingGPADL);
	if (status != B_OK) {
		ERROR("Failed to allocate GPADL while opening channel %u (%s)\n", fChannelID,
			strerror(status));
		return status;
	}

	memset(fRingBuffer, 0, fRingBufferLength);

	fTXRing = reinterpret_cast<vmbus_ring_buffer*>(fRingBuffer);
	fTXRingLength = txLength;
	fRXRing = reinterpret_cast<vmbus_ring_buffer*>(static_cast<uint8*>(fRingBuffer)
		+ txTotalLength);
	fRXRingLength = rxLength;

	// Callback must be in place prior to channel being opened, some devices will begin
	// to receive data immediately afterwards
	fCallback = callback;
	fCallbackData = callbackData;
	if (fCallback != NULL) {
		status = gDPC->new_dpc_queue(&fDPCHandle, "hyperv vmbus device", B_NORMAL_PRIORITY);
		if (status != B_OK)
			return status;
	}

	status = fVMBus->open_channel(fVMBusCookie, fChannelID, fRingGPADL, txTotalLength,
		(hyperv_device_callback)((fCallback != NULL) ? _CallbackHandler : NULL),
		(fCallback != NULL) ? this : NULL);
	if (status != B_OK) {
		ERROR("Failed to open channel %u (%s)\n", fChannelID, strerror(status));
		return status;
	}

	fIsOpen = true;
	return B_OK;
}


void
VMBusDevice::Close()
{
	MutexLocker locker(fLock);

	if (!fIsOpen)
		return;
	fIsOpen = false;

	status_t status = fVMBus->close_channel(fVMBusCookie, fChannelID);
	if (status != B_OK)
		ERROR("Failed to close channel %u (%s)\n", fChannelID, strerror(status));

	status = FreeGPADL(fRingGPADL);
	if (status != B_OK)
		ERROR("Failed to free ring GPADL for channel %u (%s)\n", fChannelID, strerror(status));

	if (fDPCHandle != NULL) {
		gDPC->delete_dpc_queue(fDPCHandle);
		fDPCHandle = NULL;
	}
}


status_t
VMBusDevice::WritePacket(uint16 type, const void* buffer, uint32 length, bool responseRequired,
	uint64 transactionID)
{
	TRACE_TX("Channel %u TX pkt %u len 0x%X resp %u tran %" B_PRIu64 "\n", fChannelID, type,
		length, responseRequired, transactionID);

	vmbus_pkt_header header;
	uint32 totalLength = sizeof(header) + length;
	uint32 totalLengthAligned = VMBUS_PKT_ALIGN(totalLength);

	header.type = type;
	header.header_length = static_cast<uint16>(sizeof(header) >> VMBUS_PKT_SIZE_SHIFT);
	header.total_length = static_cast<uint16>(totalLengthAligned >> VMBUS_PKT_SIZE_SHIFT);
	header.flags = responseRequired ? VMBUS_PKT_FLAGS_RESPONSE_REQUIRED : 0;
	header.transaction_id = transactionID;

	iovec data[3];
	uint64 padding = 0;
	data[0].iov_base = &header;
	data[0].iov_len = sizeof(header);
	data[1].iov_base = (void*)buffer;
	data[1].iov_len = length;
	data[2].iov_base = &padding;
	data[2].iov_len = totalLengthAligned - totalLength;

	return _WriteTXData(data, 3);
}


status_t
VMBusDevice::WriteGPAPacket(uint32 rangeCount, const vmbus_gpa_range* rangesList,
	uint32 rangesLength, const void* buffer, uint32 length, bool responseRequired,
	uint64 transactionID)
{
	TRACE_TX("Channel %u TX gpa pkt cnt %u gpa len 0x%X len 0x%X resp %u tran %" B_PRIu64 "\n",
		fChannelID, rangeCount, rangesLength, length, responseRequired, transactionID);

	vmbus_pkt_gpa_header gpa;
	uint32 headerLength = sizeof(gpa) + rangesLength;
	uint32 totalLength = headerLength + length;
	uint32 totalLengthAligned = VMBUS_PKT_ALIGN(totalLength);

	gpa.header.type = VMBUS_PKTTYPE_DATA_USING_GPA_DIRECT;
	gpa.header.header_length = static_cast<uint16>(headerLength >> VMBUS_PKT_SIZE_SHIFT);
	gpa.header.total_length = static_cast<uint16>(totalLengthAligned >> VMBUS_PKT_SIZE_SHIFT);
	gpa.header.flags = responseRequired ? VMBUS_PKT_FLAGS_RESPONSE_REQUIRED : 0;
	gpa.header.transaction_id = transactionID;

	gpa.reserved = 0;
	gpa.range_count = rangeCount;

	iovec data[4];
	uint64 padding = 0;
	data[0].iov_base = &gpa;
	data[0].iov_len = sizeof(gpa);
	data[1].iov_base = (void*)rangesList;
	data[1].iov_len = rangesLength;
	data[2].iov_base = (void*)buffer;
	data[2].iov_len = length;
	data[3].iov_base = &padding;
	data[3].iov_len = totalLengthAligned - totalLength;

	return _WriteTXData(data, 4);
}


status_t
VMBusDevice::ReadPacket(void* buffer, uint32* bufferLength, uint32* _headerLength,
	uint32* _dataLength)
{
	if (*bufferLength < sizeof(vmbus_pkt_header))
		return B_BAD_VALUE;

	InterruptsSpinLocker locker(fRXLock);

	// Should have at least the standard header and the shifted read index present on the ring
	if (_AvailableRX() < sizeof(vmbus_pkt_header) + sizeof(uint64))
		return B_DEV_NOT_READY;

	uint32 readIndexNew = atomic_get((int32*)&fRXRing->read_index);
	TRACE_RX("Channel %u RX old read idx 0x%X write idx 0x%X\n", fChannelID, readIndexNew,
		atomic_get((int32*)&fRXRing->write_index));

	// Read in the standard header and determine the length of the remainder of the data
	vmbus_pkt_header* header = reinterpret_cast<vmbus_pkt_header*>(buffer);
	readIndexNew = _ReadRX(readIndexNew, header, sizeof(*header));

	uint32 headerLength = header->header_length << VMBUS_PKT_SIZE_SHIFT;
	uint32 totalLength = header->total_length << VMBUS_PKT_SIZE_SHIFT;
	if (headerLength < sizeof(*header) || totalLength < headerLength) {
		ERROR("Channel %u RX invalid pkt hdr len 0x%X tot len 0x%X\n", fChannelID, headerLength,
			totalLength);
		return B_BAD_DATA;
	}
	void* dataBuffer = reinterpret_cast<uint8*>(buffer) + headerLength;
	uint32 dataLength = totalLength - headerLength;

	TRACE_RX("Channel %u RX pkt %u hdr len 0x%X data len 0x%X tran %" B_PRIu64 "\n", fChannelID,
		header->type, headerLength, dataLength, header->transaction_id);

	// Ensure provided buffer is large enough
	if (*bufferLength < totalLength) {
		*bufferLength = totalLength;
		return B_NO_MEMORY;
	}
	*bufferLength = totalLength;

	uint32 readLength = totalLength + sizeof(uint64);
	if (_AvailableRX() < readLength)
		return B_DEV_NOT_READY;

	// Standard header was already read above; read remainder of header and data
	// Shifted index is discarded
	readIndexNew = _ReadRX(readIndexNew, header + 1, headerLength - sizeof(*header));
	readIndexNew = _ReadRX(readIndexNew, dataBuffer, dataLength);
	readIndexNew = _SeekRX(readIndexNew, sizeof(uint64));
	memory_write_barrier();

	atomic_set((int32*)&fRXRing->read_index, (int32)readIndexNew);
	TRACE_RX("Channel %u RX new read idx 0x%X write idx 0x%X\n", fChannelID,
		atomic_get((int32*)&fRXRing->read_index), atomic_get((int32*)&fRXRing->write_index));

	locker.Unlock();

	*_headerLength = headerLength;
	*_dataLength = dataLength;

	// Signal Hyper-V if required; signaling is only needed if the RX ring buffer was previously
	// completely full, and there is now enough space to write pending data (if supported)
	memory_read_barrier();
	if (fRXRing->features.pending_send_size_supported) {
		uint32 pendingSendLength = atomic_get((int32*)&fRXRing->pending_send_size);
		if (pendingSendLength > 0) {
			uint32 writeableLength = fRXRingLength - _AvailableRX();
			if ((writeableLength - readLength) <= pendingSendLength
					&& writeableLength > pendingSendLength) {
				atomic_add64((int64*)&fRXRing->guest_to_host_interrupt_count, 1);
				fVMBus->signal_channel(fVMBusCookie, fChannelID);
			}
		}
	}

	return B_OK;
}


status_t
VMBusDevice::AllocateGPADL(uint32 length, void** _buffer, uint32* _gpadl)
{
	return fVMBus->allocate_gpadl(fVMBusCookie, fChannelID, length, _buffer, _gpadl);
}


status_t
VMBusDevice::FreeGPADL(uint32 gpadl)
{
	return fVMBus->free_gpadl(fVMBusCookie, fChannelID, gpadl);
}


/*static*/ void
VMBusDevice::_CallbackHandler(void* arg)
{
	VMBusDevice* vmbusDevice = reinterpret_cast<VMBusDevice*>(arg);
	gDPC->queue_dpc(vmbusDevice->fDPCHandle, _DPCHandler, arg);
}


/*static*/ void
VMBusDevice::_DPCHandler(void* arg)
{
	VMBusDevice* vmbusDevice = reinterpret_cast<VMBusDevice*>(arg);
	vmbusDevice->fCallback(vmbusDevice->fCallbackData);
}


inline uint32
VMBusDevice::_AvailableTX()
{
	uint32 readIndex = atomic_get((int32*)&fTXRing->read_index);
	uint32 writeIndex = atomic_get((int32*)&fTXRing->write_index);

	return (writeIndex >= readIndex)
		? (fTXRingLength - (writeIndex - readIndex))
		: (readIndex - writeIndex);
}


inline uint32
VMBusDevice::_WriteTX(uint32 writeIndex, const void* buffer, uint32 length)
{
	// Copy data to the TX ring, accounting for wraparound if needed
	if (length > fTXRingLength - writeIndex) {
		uint32 fragmentLength = fTXRingLength - writeIndex;
		TRACE("Channel %u TX wraparound by %u bytes\n", fChannelID, fragmentLength);

		memcpy(&fTXRing->buffer[writeIndex], buffer, fragmentLength);
		memcpy(&fTXRing->buffer[0], static_cast<const uint8*>(buffer) + fragmentLength,
			length - fragmentLength);
	} else {
		memcpy(&fTXRing->buffer[writeIndex], buffer, length);
	}

	// Advance the write index accounting for the wraparound
	return (writeIndex + length) % fTXRingLength;
}


status_t
VMBusDevice::_WriteTXData(const iovec txData[], size_t txDataCount)
{
	uint64 writeIndexOldShifted;
	uint32 length = sizeof(writeIndexOldShifted);
	for (uint32 i = 0; i < txDataCount; i++)
		length += txData[i].iov_len;

	InterruptsSpinLocker locker(fTXLock);

	// Ensure there is enough space in the TX ring; the write index
	// cannot equal the read index as this normally indicates there is no data in the ring
	if (length > _AvailableTX())
		return B_DEV_NOT_READY;

	uint32 writeIndexOld = atomic_get((int32*)&fTXRing->write_index);
	TRACE_TX("Channel %u TX old write idx 0x%X read idx 0x%X\n", fChannelID, writeIndexOld,
		atomic_get((int32*)&fTXRing->read_index));

	// Copy the data to the TX ring
	uint32 writeIndexNew = writeIndexOld;
	for (uint32 i = 0; i < txDataCount; i++)
		writeIndexNew = _WriteTX(writeIndexNew, txData[i].iov_base, txData[i].iov_len);

	// Write the old write index immediately after the newly written data, shifted over by 32 bits
	writeIndexOldShifted = static_cast<uint64>(writeIndexOld) << 32;
	writeIndexNew = _WriteTX(writeIndexNew, &writeIndexOldShifted, sizeof(writeIndexOldShifted));
	memory_write_barrier();

	atomic_set((int32*)&fTXRing->write_index, (int32)writeIndexNew);
	TRACE_TX("Channel %u TX new write idx 0x%X read idx 0x%X\n", fChannelID,
		atomic_get((int32*)&fTXRing->write_index), atomic_get((int32*)&fTXRing->read_index));

	locker.Unlock();

	// Signal Hyper-V if required; signaling is only needed if the ring buffer is changing state
	// from empty to having some amount of data. It does not need a signal if the buffer already
	// has some amount of data, and we are just adding more
	memory_read_barrier();
	if (fTXRing->interrupt_mask == 0
			&& writeIndexOld == (uint32)atomic_get((int32*)&fTXRing->read_index)) {
		atomic_add64((int64*)&fTXRing->guest_to_host_interrupt_count, 1);
		fVMBus->signal_channel(fVMBusCookie, fChannelID);
	}

	return B_OK;
}


inline uint32
VMBusDevice::_AvailableRX()
{
	uint32 readIndex = atomic_get((int32*)&fRXRing->read_index);
	uint32 writeIndex = atomic_get((int32*)&fRXRing->write_index);

	return fRXRingLength - ((writeIndex >= readIndex)
		? (fRXRingLength - (writeIndex - readIndex))
		: (readIndex - writeIndex));
}


inline uint32
VMBusDevice::_SeekRX(uint32 readIndex, uint32 length)
{
	// Advance the read index accounting for the wraparound
	return (readIndex + length) % fRXRingLength;
}


inline uint32
VMBusDevice::_ReadRX(uint32 readIndex, void* buffer, uint32 length)
{
	// Copy data from the RX ring, accounting for wraparound if needed
	if (length > fRXRingLength - readIndex) {
		uint32 fragmentLength = fRXRingLength - readIndex;
		TRACE("Channel %u RX wraparound by %u bytes\n", fChannelID, fragmentLength);

		memcpy(buffer, &fRXRing->buffer[readIndex], fragmentLength);
		memcpy(static_cast<uint8*>(buffer) + fragmentLength, &fRXRing->buffer[0],
			length - fragmentLength);
	} else {
		memcpy(buffer, &fRXRing->buffer[readIndex], length);
	}

	return _SeekRX(readIndex, length);
}
