/*
 *	Davicom DM9601 USB 1.1 Ethernet Driver.
 *	Copyright (c) 2008, 2011 Siarzhuk Zharski <imker@gmx.li>
 *	Copyright (c) 2009 Adrien Destugues <pulkomandy@gmail.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 */


#include "DavicomDevice.h"

#include <stdio.h>
#include <net/if_media.h>

#include "Driver.h"
#include "Settings.h"


const int kFrameSize = 1522;

enum VendorRequests {
	ReqReadRegister			= 0,
	ReqWriteRegister		= 1,
	ReqWriteRegisterByte	= 3,
};


enum DM9601Registers {
	RegNCR	= 0x00,	// Network Control Register
		NCRExtPHY	= 0x80,	// Select External PHY
		NCRFullDX	= 0x08,	// Full duplex
		NCRLoopback	= 0x06,	// Internal PHY analog loopback

	RegNSR	= 0x01,	// Network Status Register
		NSRSpeed10	= 0x80,	// 0 = 100MBps, 1 = 10MBps (internal PHY)
		NSRLinkUp	= 0x40,	// 1 = link up (internal PHY)
		NSRTXFull	= 0x10,	// TX FIFO full
		NSRRXOver	= 0x08,	// RX FIFO overflow

	RegRCR	= 0x05,	// RX Control Register
		RCRDiscardLong	= 0x20,	// Discard long packet (over 1522 bytes)
		RCRDiscardCRC	= 0x10,	// Discard CRC error packet
		RCRAllMulticast	= 0x08,	// Pass all multicast
		RCRPromiscuous	= 0x02,	// Promiscuous
		RCRRXEnable		= 0x01,	// RX enable

	RegEPCR	= 0x0b,	// EEPROM & PHY Control Register
		EPCROpSelect	= 0x08,	// EEPROM or PHY Operation Select
		EPCRRegRead		= 0x04,	// EEPROM or PHY Register Read Command
		EPCRRegWrite	= 0x02,	// EEPROM or PHY Register Write Command

	RegEPAR	= 0x0c,	// EEPROM & PHY Address Register
		EPARIntPHY		= 0x40,	// [7:6] force to 01 if Internal PHY is selected
		EPARMask		= 0x1f,	// mask [0:5]

	RegEPDRL = 0x0d, // EEPROM & PHY Low Byte Data Register

	RegEPDRH = 0x0e, // EEPROM & PHY Low Byte Data Register

	RegPAR	= 0x10,	// [0x10 - 0x15] Physical Address Register
	
	RegMAR	= 0x16,	// [0x16 - 0x1d] Multicast Address Register

	RegGPCR	= 0x1E,	// General Purpose Control Register
		GPCRPowerDown	= 0x01,	// [0:6] Define in/out direction of GPCR
								// GPIO0 - is output for Power Down function

	RegGPR	= 0x1F,	// General Purpose Register
		GPRPowerDownInPHY = 0x01,	// Power down Internal PHY

	RegUSBC	= 0xf4, // USB Control Register
		USBCIntAck		= 0x20,	// ACK with 8-bytes of data on interrupt EP
		USBCIntNAck		= 0x10,	// Supress ACK on interrupt EP

};


enum MIIRegisters {
	RegBMCR	= 0x00,
		BMCRIsolate	= 0x0400,
		BMCRReset	= 0x8000,

	RegBMSR	= 0x01,
	RegPHYID1	= 0x02,
	RegPHYID2	= 0x03,
};

#define MII_OUI(id1, id2)       (((id1) << 6) | ((id2) >> 10))
#define MII_MODEL(id2)          (((id2) & 0x03f0) >> 4)
#define MII_REV(id2)             ((id2) & 0x000f)


DavicomDevice::DavicomDevice(usb_device device, DeviceInfo& deviceInfo)
	:	fDevice(device),
		fStatus(B_ERROR),
		fOpen(false),
		fRemoved(false),
		fHasConnection(false),
		fTXBufferFull(false),
		fNonBlocking(false),
		fInsideNotify(0),
		fNotifyEndpoint(0),
		fReadEndpoint(0),
		fWriteEndpoint(0),
		fMaxTXPacketSize(0),
		fActualLengthRead(0),
		fActualLengthWrite(0),
		fStatusRead(0),
		fStatusWrite(0),
		fNotifyReadSem(-1),
		fNotifyWriteSem(-1),
		fLinkStateChangeSem(-1),
		fNotifyData(NULL)
{
	fDeviceInfo = deviceInfo;

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

	fNotifyData = new DM9601NotifyData();
	if (fNotifyData == NULL) {
		TRACE_ALWAYS("Error allocating notify buffer\n");
		return;
	}

	if (_SetupEndpoints() != B_OK) {
		return;
	}

	_InitMII();

	fStatus = B_OK;
	TRACE("Created!\n");
}


DavicomDevice::~DavicomDevice()
{
	if (fNotifyReadSem >= B_OK)
		delete_sem(fNotifyReadSem);
	if (fNotifyWriteSem >= B_OK)
		delete_sem(fNotifyWriteSem);

	if (!fRemoved) // ???
		gUSBModule->cancel_queued_transfers(fNotifyEndpoint);

	delete fNotifyData;
	TRACE("Deleted!\n");
}


status_t
DavicomDevice::Open(uint32 flags)
{
	if (fOpen)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	status_t result = _StartDevice();
	if (result != B_OK) {
		return result;
	}

	// setup state notifications
	result = gUSBModule->queue_interrupt(fNotifyEndpoint, fNotifyData,
		sizeof(DM9601NotifyData), _NotifyCallback, this);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of requesting notify interrupt:%#010x\n", result);
		return result;
	}

	result = _EnableInterrupts(true);

	fNonBlocking = (flags & O_NONBLOCK) == O_NONBLOCK;
	fOpen = true;
	TRACE("Opened: %#010x!\n", result);
	return result;
}


status_t
DavicomDevice::Close()
{
	if (fRemoved) {
		fOpen = false;
		return B_OK;
	}

	_EnableInterrupts(false);

	// wait until possible notification handling finished...
	while (atomic_add(&fInsideNotify, 0) != 0)
		snooze(100);
	gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
	gUSBModule->cancel_queued_transfers(fReadEndpoint);
	gUSBModule->cancel_queued_transfers(fWriteEndpoint);

	fOpen = false;

	status_t result = _StopDevice();
	TRACE("Closed: %#010x!\n", result);
	return result;
}


status_t
DavicomDevice::Free()
{
	TRACE("Freed!\n");
	return B_OK;
}


status_t
DavicomDevice::Read(uint8 *buffer, size_t *numBytes)
{
	size_t numBytesToRead = *numBytes;
	*numBytes = 0;

	if (fRemoved) {
		TRACE_ALWAYS("Error of receiving %d bytes from removed device.\n",
			numBytesToRead);
		return B_DEVICE_NOT_FOUND;
	}

	TRACE_RX("Request %d bytes.\n", numBytesToRead);

	struct _RXHeader {
		uint	FOE	:1;
		uint	CE	:1;
		uint	LE	:1;
		uint	PLE	:1;
		uint	RWTO:1;
		uint	LCS	:1;
		uint	MF	:1;
		uint	RF	:1;
		uint	countLow	:8;
		uint	countHigh	:8;

		uint8	Errors() { return 0xbf & *(uint8*)this; }
	} __attribute__((__packed__));

	_RXHeader header = { 0 };

	iovec rxData[] = {
		{ &header, sizeof(header) },
		{ buffer,  numBytesToRead }
	};

	status_t result = gUSBModule->queue_bulk_v(fReadEndpoint,
		rxData, 2, _ReadCallback, this);
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
		return fStatusRead;
	}

	if (fActualLengthRead < sizeof(_RXHeader)) {
		TRACE_ALWAYS("Error: no place for RXHeader: only %d of %d bytes.\n",
			fActualLengthRead, sizeof(_RXHeader));
		return B_ERROR;
	}

	if (header.Errors() != 0) {
		TRACE_ALWAYS("RX header errors %#04x detected!\n", header.Errors());
	}

	TRACE_STATS("FOE:%d CE:%d LE:%d PLE:%d rwTO:%d LCS:%d MF:%d RF:%d\n",
			header.FOE, header.CE, header.LE, header.PLE,
			header.RWTO, header.LCS, header.MF, header.RF);

	*numBytes = header.countLow | ( header.countHigh << 8 );

	if (fActualLengthRead - sizeof(_RXHeader) > *numBytes) {
		TRACE_ALWAYS("MISMATCH of the frame length: hdr %d; received:%d\n",
			*numBytes, fActualLengthRead - sizeof(_RXHeader));
	}

	TRACE_RX("Read %d bytes.\n", *numBytes);
	return B_OK;
}


status_t
DavicomDevice::Write(const uint8 *buffer, size_t *numBytes)
{
	size_t numBytesToWrite = *numBytes;
	*numBytes = 0;

	if (fRemoved) {
		TRACE_ALWAYS("Error of writing %d bytes to removed device.\n",
			numBytesToWrite);
		return B_DEVICE_NOT_FOUND;
	}

	if (!fHasConnection) {
		TRACE_ALWAYS("Error of writing %d bytes to device while down.\n",
			numBytesToWrite);
		return B_ERROR;
	}

	if (fTXBufferFull) {
		TRACE_ALWAYS("Error of writing %d bytes to device: TX buffer full.\n",
			numBytesToWrite);
		return B_ERROR;
	}

	TRACE_TX("Write %d bytes.\n", numBytesToWrite);

	// additional padding byte must be transmitted in case data size
	// to be send is multiple of pipe's max packet size
	uint16 length = numBytesToWrite;
	size_t count = 2;
	if (((numBytesToWrite + 2) % fMaxTXPacketSize) == 0) {
		length++;
		count++;
	}

	struct _TXHeader {
		uint	countLow	:8;
		uint	countHigh	:8;
	} __attribute__((__packed__));

	_TXHeader header = { length & 0xff, length >> 8 & 0xff };

	uint8 padding = 0;

	iovec txData[] = {
		{ &header, sizeof(_TXHeader) },
		{ (uint8*)buffer, numBytesToWrite },
		{ &padding, 1 }
	};

	status_t result = gUSBModule->queue_bulk_v(fWriteEndpoint,
		txData, count, _WriteCallback, this);
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
		return fStatusWrite;
	}

	*numBytes = fActualLengthWrite - sizeof(_TXHeader);

	TRACE_TX("Written %d bytes.\n", *numBytes);
	return B_OK;
}


status_t
DavicomDevice::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case ETHER_INIT:
			return B_OK;

		case ETHER_GETADDR:
			memcpy(buffer, &fMACAddress, sizeof(fMACAddress));
			return B_OK;

		case ETHER_GETFRAMESIZE:
			*(uint32 *)buffer = kFrameSize;
			return B_OK;

		case ETHER_NONBLOCK:
			TRACE("ETHER_NONBLOCK\n");
			fNonBlocking = *((uint8*)buffer);
			return B_OK;

		case ETHER_SETPROMISC:
			TRACE("ETHER_SETPROMISC\n");
			return _SetPromiscuousMode(*((uint8*)buffer));

		case ETHER_ADDMULTI:
			TRACE("ETHER_ADDMULTI\n");
			return _ModifyMulticastTable(true, (ether_address_t*)buffer);

		case ETHER_REMMULTI:
			TRACE("ETHER_REMMULTI\n");
			return _ModifyMulticastTable(false, (ether_address_t*)buffer);

		case ETHER_SET_LINK_STATE_SEM:
			fLinkStateChangeSem = *(sem_id *)buffer;
			return B_OK;

		case ETHER_GET_LINK_STATE:
			return _GetLinkState((ether_link_state *)buffer);

		default:
			TRACE_ALWAYS("Unhandled IOCTL catched: %#010x\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


void
DavicomDevice::Removed()
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
DavicomDevice::SetupDevice(bool deviceReplugged)
{
	ether_address address;
	status_t result = _ReadMACAddress(&address);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading MAC address:%#010x\n", result);
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
				fMACAddress.ebyte[0], fMACAddress.ebyte[1],
				fMACAddress.ebyte[2], fMACAddress.ebyte[3],
				fMACAddress.ebyte[4], fMACAddress.ebyte[5]);
			return B_BAD_VALUE; // is not the same
		}
	} else
		fMACAddress = address;

	return B_OK;
}


status_t
DavicomDevice::CompareAndReattach(usb_device device)
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error getting USB device descriptor.\n");
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
DavicomDevice::_SetupEndpoints()
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
		TRACE_ALWAYS("Error: not all USB endpoints were found: notify:%d; "
			"read:%d; write:%d\n", notifyEndpoint, readEndpoint, writeEndpoint);
		return B_ERROR;
	}

	gUSBModule->set_configuration(fDevice, config);

	fNotifyEndpoint = interface->endpoint[notifyEndpoint].handle;
	fReadEndpoint   = interface->endpoint[readEndpoint  ].handle;
	fWriteEndpoint  = interface->endpoint[writeEndpoint ].handle;
	fMaxTXPacketSize = interface->endpoint[writeEndpoint].descr->max_packet_size;

	return B_OK;
}


status_t
DavicomDevice::_ReadMACAddress(ether_address_t *address)
{
	status_t result = _ReadRegister(RegPAR,
							sizeof(ether_address), (uint8*)address);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading MAC address:%#010x\n", result);
		return result;
	}

	return B_OK;
}


status_t
DavicomDevice::_StartDevice()
{
	uint8 control = 0;

	// disable loopback
	status_t result = _ReadRegister(RegNCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading NCR: %#010x.\n", result);
		return result;
	}

	if (control & NCRExtPHY)
		TRACE_ALWAYS("Device uses external PHY\n");

	control &= ~NCRLoopback;
	result = _Write1Register(RegNCR, control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to NCR: %#010x.\n", control, result);
		return result;
	}

	// Initialize RX control register, enable RX and activate multicast
	result = _ReadRegister(RegRCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading RCR: %#010x.\n", result);
		return result;
	}

	control &= ~RCRPromiscuous;
	control |= RCRDiscardLong | RCRDiscardCRC | RCRRXEnable | RCRAllMulticast;
	result = _Write1Register(RegRCR, control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to RCR: %#010x.\n", control, result);
		return result;
	}

	// clear POWER_DOWN state of internal PHY
	result = _ReadRegister(RegGPCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading GPCR: %#010x.\n", result);
		return result;
	}

	control |= GPCRPowerDown;
	result = _Write1Register(RegGPCR, control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to GPCR: %#010x.\n", control, result);
		return result;
	}

	result = _ReadRegister(RegGPR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading GPR: %#010x.\n", result);
		return result;
	}

	control &= ~GPRPowerDownInPHY;
	result = _Write1Register(RegGPR, control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to GPR: %#010x.\n", control, result);
		return result;
	}

	return B_OK;
}


status_t
DavicomDevice::_StopDevice()
{
	uint8 control = 0;

	// disable RX
	status_t result = _ReadRegister(RegRCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading RCR: %#010x.\n", result);
		return result;
	}

	control &= ~RCRRXEnable;
	result = _Write1Register(RegRCR, control);
	if (result != B_OK)
		TRACE_ALWAYS("Error writing %#02X to RCR: %#010x.\n", control, result);

	return result;
}


status_t
DavicomDevice::_SetPromiscuousMode(bool on)
{
	uint8 control = 0;

	status_t result = _ReadRegister(RegRCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading RCR: %#010x.\n", result);
		return result;
	}

	if (on)
		control |= RCRPromiscuous;
	else
		control &= ~RCRPromiscuous;

	result = _Write1Register(RegRCR, control);
	if (result != B_OK)
		TRACE_ALWAYS("Error writing %#02X to RCR: %#010x.\n", control, result);

	return result;
}


uint32
DavicomDevice::_EthernetCRC32(const uint8* buffer, size_t length)
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
DavicomDevice::_ModifyMulticastTable(bool join, ether_address_t *group)
{
	char groupName[6 * 3 + 1] = { 0 };
	sprintf(groupName, "%02x:%02x:%02x:%02x:%02x:%02x",
		group->ebyte[0], group->ebyte[1], group->ebyte[2],
		group->ebyte[3], group->ebyte[4], group->ebyte[5]);
	TRACE("%s multicast group %s\n", join ? "Joining" : "Leaving", groupName);

	uint32 hash = _EthernetCRC32(group->ebyte, 6);
	bool isInTable = fMulticastHashes.Find(hash) != fMulticastHashes.End();
	
	if (isInTable && join)
		return B_OK; // already listed - nothing to do

	if (!isInTable && !join) {
		TRACE_ALWAYS("Cannot leave unlisted multicast group %s!\n", groupName);
		return B_ERROR;
	}

	const size_t hashLength = 8;
	uint8 hashTable[hashLength] = { 0 };
	hashTable[hashLength - 1] |= 0x80; // broadcast address

	status_t result = _WriteRegister(RegMAR, hashLength, hashTable);
	if (result != B_OK) {
		TRACE_ALWAYS("Error initializing MAR: %#010x.\n", result);
		return result;
	}
	
	if (join)
		fMulticastHashes.PushBack(hash);
	else
		fMulticastHashes.Remove(hash);

	for (int32 i = 0; i < fMulticastHashes.Count(); i++) {
		uint32 hash = fMulticastHashes[i] >> 26;
		hashTable[hash / 8] |= 1 << (hash % 8);
	}

	// clear/set pass all multicast bit as required
	uint8 control = 0;
	result = _ReadRegister(RegRCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading RCR: %#010x.\n", result);
		return result;
	}

	if (fMulticastHashes.Count() > 0)
		control &= ~RCRAllMulticast;
	else
		control |= RCRAllMulticast;

	result = _Write1Register(RegRCR, control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to RCR: %#010x.\n", control, result);
		return result;
	}

	result = _WriteRegister(RegMAR, hashLength, hashTable);
	if (result != B_OK)
		TRACE_ALWAYS("Error writing hash table in MAR: %#010x.\n", result);

	return result;
}


void
DavicomDevice::_ReadCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_RX("ReadCB: %d bytes; status:%#010x\n", actualLength, status);
	DavicomDevice *device = (DavicomDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	device->fStats.readCount++;
	release_sem_etc(device->fNotifyReadSem, 1, B_DO_NOT_RESCHEDULE);
}


void
DavicomDevice::_WriteCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_TX("WriteCB: %d bytes; status:%#010x\n", actualLength, status);
	DavicomDevice *device = (DavicomDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	device->fStats.writeCount++;
	release_sem_etc(device->fNotifyWriteSem, 1, B_DO_NOT_RESCHEDULE);
}


void
DavicomDevice::_NotifyCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	DavicomDevice *device = (DavicomDevice *)cookie;
	atomic_add(&device->fInsideNotify, 1);
	if (status == B_CANCELED || device->fRemoved) {
		atomic_add(&device->fInsideNotify, -1);
		return;
	}

	if (status == B_OK)
		device->_OnNotify(actualLength);
	else
		TRACE_ALWAYS("Status error:%#010x; length:%d\n", status, actualLength);

	// schedule next notification buffer
	gUSBModule->queue_interrupt(device->fNotifyEndpoint, device->fNotifyData,
		sizeof(DM9601NotifyData), _NotifyCallback, device);
	atomic_add(&device->fInsideNotify, -1);
}


status_t
DavicomDevice::_OnNotify(uint32 actualLength)
{
	if (actualLength != sizeof(DM9601NotifyData)) {
		TRACE_ALWAYS("Data underrun error. %d of %d bytes received\n",
			actualLength, sizeof(DM9601NotifyData));
		return B_BAD_DATA;
	}

	bool linkIsUp = fNotifyData->LINKST != 0;
	fTXBufferFull = fNotifyData->TXFULL != 0;
	bool rxOverflow = fNotifyData->RXOV != 0;

	bool linkStateChange = (linkIsUp != fHasConnection);
	fHasConnection = linkIsUp;

	if (linkStateChange) {
		if (fHasConnection) {
			TRACE("Link is now up at %s Mb/s\n",
				fNotifyData->SPEED ? "10" : "100");
		} else
			TRACE("Link is now down");
	}

#ifdef UDAV_TRACE
	if (gTraceStats) {
		if (fNotifyData->TXFULL)
			fStats.txFull++;
		if (fNotifyData->RXOV)
			fStats.rxOverflow++;

		if (fNotifyData->ROC)
			fStats.rxOvCount += fNotifyData->ROC;

		if (fNotifyData->RT)
			fStats.runtFrames++;
		if (fNotifyData->LCS)
			fStats.lateRXCollisions++;
		if (fNotifyData->RWTO)
			fStats.rwTOs++;
		if (fNotifyData->PLE)
			fStats.physLayerErros++;
		if (fNotifyData->AE)
			fStats.alignmentErros++;
		if (fNotifyData->CE)
			fStats.crcErrors++;
		if (fNotifyData->FOE)
			fStats.overErrors++;

		if (fNotifyData->TSR1.LC)
			fStats.lateTXCollisions++;
		if (fNotifyData->TSR1.LCR)
			fStats.lostOfCarrier++;
		if (fNotifyData->TSR1.NC)
			fStats.noCarrier++;
		if (fNotifyData->TSR1.COL)
			fStats.txCollisions++;
		if (fNotifyData->TSR1.EC)
			fStats.excCollisions++;

		if (fNotifyData->TSR2.LC)
			fStats.lateTXCollisions++;
		if (fNotifyData->TSR2.LCR)
			fStats.lostOfCarrier++;
		if (fNotifyData->TSR2.NC)
			fStats.noCarrier++;
		if (fNotifyData->TSR2.COL)
			fStats.txCollisions++;
		if (fNotifyData->TSR2.EC)
			fStats.excCollisions++;

		fStats.notifyCount++;
	}
#endif

	if (rxOverflow)
		TRACE("RX buffer overflow. %d packets dropped\n", fNotifyData->ROC);

	uint8 tsr = 0xfc & *(uint8*)&fNotifyData->TSR1;
	if (tsr != 0)
		TRACE("TX packet 1: Status %#04x is not OK.\n", tsr);

	tsr = 0xfc & *(uint8*)&fNotifyData->TSR2;
	if (tsr != 0)
		TRACE("TX packet 2: Status %#04x is not OK.\n", tsr);

	if (linkStateChange && fLinkStateChangeSem >= B_OK)
		release_sem_etc(fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
	return B_OK;
}


status_t
DavicomDevice::_GetLinkState(ether_link_state *linkState)
{
	uint8 registerValue = 0;
	status_t result = _ReadRegister(RegNSR, 1, &registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading NSR register! %x\n", result);
		return result;
	}

	if (registerValue & NSRSpeed10)
		linkState->speed = 10000000;
	else
		linkState->speed = 100000000;

	linkState->quality = 1000;

	linkState->media = IFM_ETHER | IFM_100_TX;
	if (fHasConnection) {
		linkState->media |= IFM_ACTIVE;
		result = _ReadRegister(RegNCR, 1, &registerValue);
		if (result != B_OK) {
			TRACE_ALWAYS("Error reading NCR register! %x\n", result);
			return result;
		}

		if (registerValue & NCRFullDX)
			linkState->media |= IFM_FULL_DUPLEX;
		else
			linkState->media |= IFM_HALF_DUPLEX;

		if (registerValue & NCRLoopback)
			linkState->media |= IFM_LOOP;
	}

	TRACE_STATE("Medium state: %s, %lld MBit/s, %s duplex.\n",
						(linkState->media & IFM_ACTIVE) ? "active" : "inactive",
						linkState->speed / 1000000,
						(linkState->media & IFM_FULL_DUPLEX) ? "full" : "half");

	TRACE_STATS("tx:%d rx:%d rxCn:%d rtF:%d lRxC:%d rwTO:%d PLE:%d AE:%d CE:%d "
				"oE:%d ltxC:%d lCR:%d nC:%d txC:%d exC:%d r:%d w:%d n:%d\n",
					fStats.txFull, fStats.rxOverflow, fStats.rxOvCount,
					fStats.runtFrames, fStats.lateRXCollisions, fStats.rwTOs,
					fStats.physLayerErros, fStats.alignmentErros,
					fStats.crcErrors, fStats.overErrors,
					fStats.lateTXCollisions, fStats.lostOfCarrier,
					fStats.noCarrier, fStats.txCollisions, fStats.excCollisions,
					fStats.readCount, fStats.writeCount, fStats.notifyCount);
	return B_OK;
}


status_t
DavicomDevice::_ReadRegister(uint8 reg, size_t size, uint8* buffer)
{
	if (size > 255)
		return B_BAD_VALUE;

	size_t actualLength = 0;
	status_t result = gUSBModule->send_request(fDevice,
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
		ReqReadRegister, 0, reg, size, buffer, &actualLength);

	if (size != actualLength) {
		TRACE_ALWAYS("Size mismatch reading register ! asked %d got %d",
			size, actualLength);
	}

	return result;
}


status_t
DavicomDevice::_WriteRegister(uint8 reg, size_t size, uint8* buffer)
{
	if (size > 255)
		return B_BAD_VALUE;

	size_t actualLength = 0;

	status_t result = gUSBModule->send_request(fDevice,
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		ReqWriteRegister, 0, reg, size, buffer, &actualLength);

	return result;
}


status_t
DavicomDevice::_Write1Register(uint8 reg, uint8 value)
{
	size_t actualLength = 0;

	status_t result = gUSBModule->send_request(fDevice,
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		ReqWriteRegisterByte, value, reg, 0, NULL, &actualLength);

	return result;
}


status_t
DavicomDevice::_ReadMII(uint8 reg, uint16* data)
{
	// select PHY and set PHY register address
	status_t result = _Write1Register(RegEPAR, EPARIntPHY | (reg & EPARMask));
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to set MII address %#x. Error:%#x\n", reg, result);
		return result;
	}

	// select PHY operation and initiate reading
	result = _Write1Register(RegEPCR, EPCROpSelect | EPCRRegRead);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to starting MII reading. Error:%#x\n", result);
		return result;
	}

	// finalize writing
	uint8 control = 0;
	result = _ReadRegister(RegEPCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to read EPCR register. Error:%#x\n", result);
		return result;
	}

	result = _Write1Register(RegEPCR, control & ~EPCRRegRead);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to write EPCR register. Error:%#x\n", result);
		return result;
	}

	// retrieve the result from data registers
	uint8 values[2] = { 0 };
	result = _ReadRegister(RegEPDRL, 2, values);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to retrieve data %#x. Error:%#x\n", data, result);
		return result;
	}

	*data = values[0] | values[1] << 8;
	return result;
}


status_t
DavicomDevice::_WriteMII(uint8 reg, uint16 data)
{
	// select PHY and set PHY register address
	status_t result = _Write1Register(RegEPAR, EPARIntPHY | (reg & EPARMask));
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to set MII address %#x. Error:%#x\n", reg, result);
		return result;
	}

	// put the value to data register
	uint8 values[] = { data & 0xff, ( data >> 8 ) & 0xff };
	result = _WriteRegister(RegEPDRL, sizeof(uint16), values);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to put data %#x. Error:%#x\n", data, result);
		return result;
	}

	// select PHY operation and initiate writing
	result = _Write1Register(RegEPCR, EPCROpSelect | EPCRRegWrite);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to starting MII wrintig. Error:%#x\n", result);
		return result;
	}

	// finalize writing
	uint8 control = 0;
	result = _ReadRegister(RegEPCR, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to read EPCR register. Error:%#x\n", result);
		return result;
	}

	result = _Write1Register(RegEPCR, control & ~EPCRRegWrite);
	if (result != B_OK)
		TRACE_ALWAYS("Failed to write EPCR register. Error:%#x\n", result);

	return result;
}


status_t
DavicomDevice::_InitMII()
{
	uint16 control = 0;
	status_t result = _ReadMII(RegBMCR, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to read MII BMCR register. Error:%#x\n", result);
		return result;
	}

	result = _WriteMII(RegBMCR, control & ~BMCRIsolate);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to clear isolate PHY. Error:%#x\n", result);
		return result;
	}

	result = _WriteMII(0, BMCRReset);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to reset BMCR register. Error:%#x\n", result);
		return result;
	}

	uint16 id01 = 0, id02 = 0;
	result = _ReadMII(RegPHYID1, &id01);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to read PHY ID 0. Error:%#x\n", result);
		return result;
	}

	result = _ReadMII(RegPHYID2, &id02);
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to read PHY ID 1. Error:%#x\n", result);
		return result;
	}

	TRACE_ALWAYS("MII Info: OUI:%04x; Model:%04x; rev:%02x.\n",
			MII_OUI(id01, id02), MII_MODEL(id02), MII_REV(id02));

	return result;
}


status_t
DavicomDevice::_EnableInterrupts(bool enable)
{
	uint8 control = 0;
	status_t result = _ReadRegister(RegUSBC, 1, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading USB control register:%#010x\n", result);
		return result;
	}

	if (enable) {
		control |= USBCIntAck;
		control &= ~USBCIntNAck;
	} else {
		control &= ~USBCIntAck;
	}

	result = _Write1Register(RegUSBC, control);
	if (result != B_OK)
		TRACE_ALWAYS("Error of setting USB control register:%#010x\n", result);

	return result;
}

