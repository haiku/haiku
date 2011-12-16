/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008, 2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "ASIXDevice.h"

#include <stdio.h>

#include "ASIXVendorRequests.h"
#include "Driver.h"
#include "Settings.h"


// frame header used during transfer data
struct TRXHeader {
	uint16	fLength;
	uint16	fInvertedLength;

	TRXHeader(uint16 length = 0) { SetLength(length); }
	bool 	IsValid() { return (fLength ^ fInvertedLength) == 0xffff; }
	uint16	Length()  { return fLength; }
	// TODO: low-endian convertion?
	void	SetLength(uint16 length) {
				fLength = length;
				fInvertedLength = ~fLength;
			}
};


ASIXDevice::ASIXDevice(usb_device device, DeviceInfo& deviceInfo)
		:
		fDevice(device),
		fStatus(B_ERROR),
		fOpen(false),
		fRemoved(false),
		fHasConnection(false),
		fNonBlocking(false),
		fInsideNotify(0),
		fFrameSize(0),
		fNotifyEndpoint(0),
		fReadEndpoint(0),
		fWriteEndpoint(0),
		fActualLengthRead(0),
		fActualLengthWrite(0),
		fStatusRead(B_OK),
		fStatusWrite(B_OK),
		fNotifyReadSem(-1),
		fNotifyWriteSem(-1),
		fNotifyBuffer(NULL),
		fNotifyBufferLength(0),
		fLinkStateChangeSem(-1),
		fUseTRXHeader(false),
		fReadNodeIDRequest(kInvalidRequest)
{
	fDeviceInfo = deviceInfo;

	fIPG[0] = 0x15;
	fIPG[1] = 0x0c;
	fIPG[2] = 0x12;

	memset(&fMACAddress, 0, sizeof(fMACAddress));

	fNotifyReadSem = create_sem(0, DRIVER_NAME"_notify_read");
	if (fNotifyReadSem < B_OK) {
		TRACE_ALWAYS("Error of creating read notify semaphore:%#010x\n",
															fNotifyReadSem);
		return;
	}

	fNotifyWriteSem = create_sem(0, DRIVER_NAME"_notify_write");
	if (fNotifyWriteSem < B_OK) {
		TRACE_ALWAYS("Error of creating write notify semaphore:%#010x\n",
															fNotifyWriteSem);
		return;
	}

	if (_SetupEndpoints() != B_OK) {
		return;
	}

	// must be set in derived class constructor
	// fStatus = B_OK;
}


ASIXDevice::~ASIXDevice()
{
	if (fNotifyReadSem >= B_OK)
		delete_sem(fNotifyReadSem);
	if (fNotifyWriteSem >= B_OK)
		delete_sem(fNotifyWriteSem);

	if (!fRemoved) // ???
		gUSBModule->cancel_queued_transfers(fNotifyEndpoint);

	if (fNotifyBuffer)
		free(fNotifyBuffer);
}


status_t
ASIXDevice::Open(uint32 flags)
{
	if (fOpen)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	status_t result = StartDevice();
	if (result != B_OK) {
		return result;
	}

	// setup state notifications
	result = gUSBModule->queue_interrupt(fNotifyEndpoint, fNotifyBuffer,
								fNotifyBufferLength, _NotifyCallback, this);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of requesting notify interrupt:%#010x\n", result);
		return result;
	}

	fNonBlocking = (flags & O_NONBLOCK) == O_NONBLOCK;
	fOpen = true;
	return result;
}


status_t
ASIXDevice::Close()
{
	if (fRemoved) {
		fOpen = false;
		return B_OK;
	}

	// wait until possible notification handling finished...
	while (atomic_add(&fInsideNotify, 0) != 0)
		snooze(100);
	gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
	gUSBModule->cancel_queued_transfers(fReadEndpoint);
	gUSBModule->cancel_queued_transfers(fWriteEndpoint);

	fOpen = false;

	return StopDevice();
}


status_t
ASIXDevice::Free()
{
	return B_OK;
}


status_t
ASIXDevice::Read(uint8 *buffer, size_t *numBytes)
{
	size_t numBytesToRead = *numBytes;
	*numBytes = 0;

	if (fRemoved) {
		TRACE_ALWAYS("Error of receiving %d bytes from removed device.\n",
															numBytesToRead);
		return B_DEVICE_NOT_FOUND;
	}

	TRACE_FLOW("Request %d bytes.\n", numBytesToRead);

	TRXHeader header;
	iovec rxData[] = {
		{ &header, sizeof(TRXHeader) },
		{ buffer,  numBytesToRead }
	};

	size_t startIndex = fUseTRXHeader ? 0 : 1 ;
	size_t chunkCount = fUseTRXHeader ? 2 : 1 ;

	status_t result = gUSBModule->queue_bulk_v(fReadEndpoint,
						&rxData[startIndex], chunkCount, _ReadCallback, this);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of queue_bulk_v request:%#010x\n", result);
		return result;
	}

	uint32 flags = B_CAN_INTERRUPT | (fNonBlocking ? B_TIMEOUT : 0);
	result = acquire_sem_etc(fNotifyReadSem, 1, flags, 0);
	if (result < B_OK) {
		TRACE_ALWAYS("Error of acquiring notify semaphore:%#010x.\n", result);
		return result;
	}

	if (fStatusRead != B_OK && fStatusRead != B_CANCELED && !fRemoved) {
		TRACE_ALWAYS("Device status error:%#010x\n", fStatusRead);
		result = gUSBModule->clear_feature(fReadEndpoint,
			USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK) {
			TRACE_ALWAYS("Error during clearing of HALT state:%#010x.\n",
					result);
			return result;
		}
	}

	if (fUseTRXHeader) {
		if (fActualLengthRead < sizeof(TRXHeader)) {
			TRACE_ALWAYS("Error: no place for TRXHeader:only %d of %d bytes.\n",
										fActualLengthRead, sizeof(TRXHeader));
			return B_ERROR; // TODO: ???
		}

		if (!header.IsValid()) {
			TRACE_ALWAYS("Error:TRX Header is invalid: len:%#04x; ilen:%#04x\n",
							header.fLength, header.fInvertedLength);
			return B_ERROR; // TODO: ???
		}

		*numBytes = header.Length();

		if (fActualLengthRead - sizeof(TRXHeader) > header.Length()) {
			TRACE_ALWAYS("MISMATCH of the frame length: hdr %d; received:%d\n",
						header.Length(), fActualLengthRead - sizeof(TRXHeader));
		}

	} else {

		*numBytes = fActualLengthRead;
	}

	TRACE_FLOW("Read %d bytes.\n", *numBytes);
	return B_OK;
}


status_t
ASIXDevice::Write(const uint8 *buffer, size_t *numBytes)
{
	size_t numBytesToWrite = *numBytes;
	*numBytes = 0;

	if (fRemoved) {
		TRACE_ALWAYS("Error of writing %d bytes to removed device.\n",
														numBytesToWrite);
		return B_DEVICE_NOT_FOUND;
	}

	TRACE_FLOW("Write %d bytes.\n", numBytesToWrite);

	TRXHeader header(numBytesToWrite);
	iovec txData[] = {
		{ &header, sizeof(TRXHeader) },
		{ (uint8*)buffer, numBytesToWrite }
	};

	size_t startIndex = fUseTRXHeader ? 0 : 1 ;
	size_t chunkCount = fUseTRXHeader ? 2 : 1 ;

	status_t result = gUSBModule->queue_bulk_v(fWriteEndpoint,
						&txData[startIndex], chunkCount, _WriteCallback, this);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of queue_bulk_v request:%#010x\n", result);
		return result;
	}

	result = acquire_sem_etc(fNotifyWriteSem, 1, B_CAN_INTERRUPT, 0);

	if (result < B_OK) {
		TRACE_ALWAYS("Error of acquiring notify semaphore:%#010x.\n", result);
		return result;
	}

	if (fStatusWrite != B_OK && fStatusWrite != B_CANCELED && !fRemoved) {
		TRACE_ALWAYS("Device status error:%#010x\n", fStatusWrite);
		result = gUSBModule->clear_feature(fWriteEndpoint,
			USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK) {
			TRACE_ALWAYS("Error during clearing of HALT state:%#010x\n", result);
			return result;
		}
	}

	if (fUseTRXHeader) {
		*numBytes = fActualLengthWrite - sizeof(TRXHeader);
	} else {
		*numBytes = fActualLengthWrite;
	}

	TRACE_FLOW("Written %d bytes.\n", *numBytes);
	return B_OK;
}


status_t
ASIXDevice::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case ETHER_INIT:
			return B_OK;

		case ETHER_GETADDR:
			memcpy(buffer, &fMACAddress, sizeof(fMACAddress));
			return B_OK;

		case ETHER_GETFRAMESIZE:
			*(uint32 *)buffer = fFrameSize;
			return B_OK;

		case ETHER_NONBLOCK:
			TRACE("ETHER_NONBLOCK\n");
			fNonBlocking = *((uint8*)buffer);
			return B_OK;

		case ETHER_SETPROMISC:
			TRACE("ETHER_SETPROMISC\n");
			return SetPromiscuousMode(*((uint8*)buffer));

		case ETHER_ADDMULTI:
			TRACE("ETHER_ADDMULTI\n");
			return ModifyMulticastTable(true, (ether_address_t*)buffer);

		case ETHER_REMMULTI:
			TRACE("ETHER_REMMULTI\n");
			return ModifyMulticastTable(false, (ether_address_t*)buffer);

		case ETHER_SET_LINK_STATE_SEM:
			fLinkStateChangeSem = *(sem_id *)buffer;
			return B_OK;

		case ETHER_GET_LINK_STATE:
			return GetLinkState((ether_link_state *)buffer);

		default:
			TRACE_ALWAYS("Unhandled IOCTL catched: %#010x\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


void
ASIXDevice::Removed()
{
	fRemoved = true;
	fHasConnection = false;

	// the notify hook is different from the read and write hooks as it does
	// itself schedule traffic (while the other hooks only release a semaphore
	// to notify another thread which in turn safly checks for the removed
	// case) - so we must ensure that we are not inside the notify hook anymore
	// before returning, as we would otherwise violate the promise not to use
	// any of the pipes after returning from the removed hook
	while (atomic_add(&fInsideNotify, 0) != 0)
		snooze(100);

	gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
	gUSBModule->cancel_queued_transfers(fReadEndpoint);
	gUSBModule->cancel_queued_transfers(fWriteEndpoint);

	if (fLinkStateChangeSem >= B_OK)
		release_sem_etc(fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
}


status_t
ASIXDevice::SetupDevice(bool deviceReplugged)
{
	ether_address address;
	status_t result = ReadMACAddress(&address);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading MAC address:%#010x\n", result);
		return result;
	}

	TRACE("MAC address is:%02x:%02x:%02x:%02x:%02x:%02x\n",
				address.ebyte[0], address.ebyte[1], address.ebyte[2],
				address.ebyte[3], address.ebyte[4], address.ebyte[5]);

	if (deviceReplugged) {
		// this might be the same device that was replugged - read the MAC
		// address (which should be at the same index) to make sure
		if (memcmp(&address, &fMACAddress, sizeof(address)) != 0) {
			TRACE_ALWAYS("Cannot replace device with MAC address:"
											"%02x:%02x:%02x:%02x:%02x:%02x\n",
				fMACAddress.ebyte[0], fMACAddress.ebyte[1], fMACAddress.ebyte[2],
				fMACAddress.ebyte[3], fMACAddress.ebyte[4], fMACAddress.ebyte[5]);
			return B_BAD_VALUE; // is not the same
		}
	} else
		fMACAddress = address;

	return B_OK;
}


status_t
ASIXDevice::CompareAndReattach(usb_device device)
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error of getting USB device descriptor.\n");
		return B_ERROR;
	}

	if (deviceDescriptor->vendor_id != fDeviceInfo.VendorId()
		&& deviceDescriptor->product_id != fDeviceInfo.ProductId()) {
		// this certainly isn't the same device
		return B_BAD_VALUE;
	}

	// this is the same device that was replugged - clear the removed state,
	// re-setup the endpoints and transfers and open the device if it was
	// previously opened
	fDevice = device;
	fRemoved = false;
	status_t result = _SetupEndpoints();
	if (result != B_OK) {
		fRemoved = true;
		return result;
	}

	// we need to setup hardware on device replug
	result = SetupDevice(true);
	if (result != B_OK) {
		return result;
	}

	if (fOpen) {
		fOpen = false;
		result = Open(fNonBlocking ? O_NONBLOCK : 0);
	}

	return result;
}


status_t
ASIXDevice::_SetupEndpoints()
{
	const usb_configuration_info *config
		= gUSBModule->get_nth_configuration(fDevice, 0);

	if (config == NULL) {
		TRACE_ALWAYS("Error of getting USB device configuration.\n");
		return B_ERROR;
	}

	if (config->interface_count <= 0) {
		TRACE_ALWAYS("Error:no interfaces found in USB device configuration\n");
		return B_ERROR;
	}

	usb_interface_info *interface = config->interface[0].active;
	if (interface == 0) {
		TRACE_ALWAYS("Error:invalid active interface in "
												"USB device configuration\n");
		return B_ERROR;
	}

	int notifyEndpoint = -1;
	int readEndpoint   = -1;
	int writeEndpoint  = -1;

	for (size_t ep = 0; ep < interface->endpoint_count; ep++) {
		usb_endpoint_descriptor *epd = interface->endpoint[ep].descr;
		if ((epd->attributes & USB_ENDPOINT_ATTR_MASK)
				== USB_ENDPOINT_ATTR_INTERRUPT)
		{
			notifyEndpoint = ep;
			continue;
		}

		if ((epd->attributes & USB_ENDPOINT_ATTR_MASK)
				!= USB_ENDPOINT_ATTR_BULK)
		{
			TRACE_ALWAYS("Error: USB endpoint type %#04x is unknown.\n",
					epd->attributes);
			continue;
		}

		if ((epd->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
				== USB_ENDPOINT_ADDR_DIR_IN)
		{
			readEndpoint = ep;
			continue;
		}

		if ((epd->endpoint_address & USB_ENDPOINT_ADDR_DIR_OUT)
				== USB_ENDPOINT_ADDR_DIR_OUT)
		{
			writeEndpoint = ep;
			continue;
		}
	}

	if (notifyEndpoint == -1 || readEndpoint == -1 || writeEndpoint == -1) {
		TRACE_ALWAYS("Error: not all USB endpoints were found: "
							"notify:%d; read:%d; write:%d\n",
							notifyEndpoint, readEndpoint, writeEndpoint);
		return B_ERROR;
	}

	gUSBModule->set_configuration(fDevice, config);

	fNotifyEndpoint = interface->endpoint[notifyEndpoint].handle;
	fReadEndpoint   = interface->endpoint[readEndpoint  ].handle;
	fWriteEndpoint  = interface->endpoint[writeEndpoint ].handle;

	return B_OK;
}


status_t
ASIXDevice::ReadMACAddress(ether_address_t *address)
{
	size_t actual_length = 0;
	status_t result = gUSBModule->send_request(fDevice,
								USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
								fReadNodeIDRequest, 0, 0, sizeof(ether_address),
								address, &actual_length);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading MAC address:%#010x\n", result);
		return result;
	}

	if (actual_length != sizeof(ether_address)) {
		TRACE_ALWAYS("Mismatch of NODE ID data size: %d instead of %d bytes\n",
										actual_length, sizeof(ether_address));
		return B_ERROR;
	}

	return B_OK;
}


status_t
ASIXDevice::ReadRXControlRegister(uint16 *rxcontrol)
{
	size_t actual_length = 0;
	*rxcontrol = 0;

	status_t result = gUSBModule->send_request(fDevice,
					USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
					READ_RX_CONTROL, 0, 0,
					sizeof(*rxcontrol), rxcontrol, &actual_length);

	if (sizeof(*rxcontrol) != actual_length) {
		TRACE_ALWAYS("Mismatch during reading RX control register."
											"Read %d bytes instead of %d.\n",
											actual_length, sizeof(*rxcontrol));
	}

	return result;
}


status_t
ASIXDevice::WriteRXControlRegister(uint16 rxcontrol)
{
	status_t result = gUSBModule->send_request(fDevice,
							USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
							WRITE_RX_CONTROL, rxcontrol, 0, 0, 0, 0);
	return result;
}


status_t
ASIXDevice::StopDevice()
{
	status_t result = WriteRXControlRegister(0);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of writing %#04x RX Control:%#010x\n", 0, result);
	}

	TRACE_RET(result);
	return result;
}


status_t
ASIXDevice::SetPromiscuousMode(bool on)
{
	uint16 rxcontrol = 0;

	status_t result = ReadRXControlRegister(&rxcontrol);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading RX Control:%#010x\n", result);
		return result;
	}

	if (on)
		rxcontrol |= RXCTL_PROMISCUOUS;
	else
		rxcontrol &= ~RXCTL_PROMISCUOUS;

	result = WriteRXControlRegister(rxcontrol);

	if (result != B_OK ) {
		TRACE_ALWAYS("Error of writing %#04x RX Control:%#010x\n",
				rxcontrol, result);
	}

	TRACE_RET(result);
	return result;
}


uint32
ASIXDevice::EthernetCRC32(const uint8* buffer, size_t length)
{
	uint32 result = 0xffffffff;
	for (size_t i = 0; i < length; i++) {
		uint8 data = *buffer++;
		for (int bit = 0; bit < 8; bit++, data >>= 1) {
			uint32 carry = ((result & 0x80000000) ? 1 : 0) ^ (data & 0x01);
			result <<= 1;
			if (carry != 0)
				result = (result ^ 0x04c11db6) | carry;
		}
	}
	return result;
}


status_t
ASIXDevice::ModifyMulticastTable(bool join, ether_address_t* group)
{
	char groupName[6 * 3 + 1] = { 0 };
	sprintf(groupName, "%02x:%02x:%02x:%02x:%02x:%02x",
		group->ebyte[0], group->ebyte[1], group->ebyte[2],
		group->ebyte[3], group->ebyte[4], group->ebyte[5]);
	TRACE("%s multicast group %s\n", join ? "Joining" : "Leaving", groupName);

	uint32 hash = EthernetCRC32(group->ebyte, 6);
	bool isInTable = fMulticastHashes.Find(hash) != fMulticastHashes.End();

	if (isInTable && join)
		return B_OK; // already listed - nothing to do

	if (!isInTable && !join) {
		TRACE_ALWAYS("Cannot leave unlisted multicast group %s!\n", groupName);
		return B_ERROR;
	}

	const size_t hashLength = 8;
	uint8 hashTable[hashLength] = { 0 };

	if (join)
		fMulticastHashes.PushBack(hash);
	else
		fMulticastHashes.Remove(hash);

	for (int32 i = 0; i < fMulticastHashes.Count(); i++) {
		uint32 hash = fMulticastHashes[i] >> 26;
		hashTable[hash / 8] |= 1 << (hash % 8);
	}

	uint16 rxcontrol = 0;

	status_t result = ReadRXControlRegister(&rxcontrol);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading RX Control:%#010x\n", result);
		return result;
	}

	if (fMulticastHashes.Count() > 0)
		rxcontrol |= RXCTL_MULTICAST;
	else
		rxcontrol &= ~RXCTL_MULTICAST;

	// write multicast hash table
	size_t actualLength = 0;
	result = gUSBModule->send_request(fDevice,
							USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
							WRITE_MF_ARRAY, 0, 0,
							hashLength, hashTable, &actualLength);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing hash table in MAR: %#010x.\n", result);
		return result;
	}

	if (actualLength != hashLength)
		TRACE_ALWAYS("Incomplete writing of hash table: %d bytes of %d\n",
						actualLength, hashLength);

	result = WriteRXControlRegister(rxcontrol);
	if (result != B_OK)
		TRACE_ALWAYS("Error writing %#02X to RXC:%#010x.\n", rxcontrol, result);

	return result;
}


void
ASIXDevice::_ReadCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("ReadCB: %d bytes; status:%#010x\n", actualLength, status);
	ASIXDevice *device = (ASIXDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fNotifyReadSem, 1, B_DO_NOT_RESCHEDULE);
}


void
ASIXDevice::_WriteCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("WriteCB: %d bytes; status:%#010x\n", actualLength, status);
	ASIXDevice *device = (ASIXDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fNotifyWriteSem, 1, B_DO_NOT_RESCHEDULE);
}


void
ASIXDevice::_NotifyCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	ASIXDevice *device = (ASIXDevice *)cookie;
	atomic_add(&device->fInsideNotify, 1);
	if (status == B_CANCELED || device->fRemoved) {
		atomic_add(&device->fInsideNotify, -1);
		return;
	}

	if (status != B_OK) {
		TRACE_ALWAYS("Device status error:%#010x\n", status);
		status_t result = gUSBModule->clear_feature(device->fNotifyEndpoint,
													USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK)
			TRACE_ALWAYS("Error during clearing of HALT state:%#010x.\n",
				result);
	}

	// parse data in overriden class
	device->OnNotify(actualLength);

	// schedule next notification buffer
	gUSBModule->queue_interrupt(device->fNotifyEndpoint, device->fNotifyBuffer,
		device->fNotifyBufferLength, _NotifyCallback, device);
	atomic_add(&device->fInsideNotify, -1);
}

