/*
 *	Davicom DM9601 USB 1.1 Ethernet Driver.
 *	Copyright (c) 2009 Adrien Destugues <pulkomandy@gmail.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */

#include "Driver.h"
#include "Settings.h"
#include "DavicomDevice.h"


// Vendor commands
#define READ_REGISTER 0
#define WRITE_REGISTER 1
#define WRITE1_REGISTER 3
#define READ_MEMORY 2
#define WRITE_MEMORY 5
#define WRITE1_MEMORY 7


// Registers
#define NCR 0x00	// Network control
#define NSR	0x01	// Network status
#define RCR 0x05	// RX Control
#define PAR 0x10	// 6 bits - Physical address (MAC)
#define GPCR 0x1E	// GPIO pins direction
#define GPR 0x1F	// GPIO pins data
#define VID 0x28	// Vendor ID (16bit)
#define PID 0x2A	// Product ID (16bit)
#define CHIPR 0x2C	// Chip revision

#define NCR_EXT_PHY		0x80	// External PHY
#define NCR_FDX			0x08	// Full duplex
#define NCR_LBK			0x06	// Loopback mode

#define NSR_SPEED		0x80	// 0 = 100MBps, 1 = 10MBps
#define NSR_LINKST		0x40	// 1 = link up
#define NSR_TXFULL		0x10	// TX FIFO full
#define NSR_RXOV		0x08	// RX Overflow

#define RCR_DIS_LONG	0x20	// Discard long packet
#define RCR_DIS_CRC		0x10	// Discard CRC error packet
#define RCR_ALL			0x08	// Pass all multicast
#define RCR_PRMSC		0x02	// Promiscuous
#define RCR_RXEN		0x01	// RX enable

#define GPCR_GEP_CNTL0	0x01	// Power Down function

#define GPR_GEP_GEPIO0	0x01	// Power down

//TODO: multicast support
//TODO: set media state support


status_t
DavicomDevice::_ReadRegister(uint8 reg, size_t size, void* buffer)
{
	if (size > 255) return B_BAD_VALUE;
	size_t actualLength;
	status_t result = gUSBModule->send_request(fDevice,
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
		READ_REGISTER, 0, reg, size, buffer, &actualLength);
	if (size != actualLength) {
		TRACE_ALWAYS("Size mismatch reading register ! asked %d got %d",
			size, actualLength);
	}
	return result;
}


status_t
DavicomDevice::_WriteRegister(uint8 reg, size_t size, uint8* buffer)
{
	if (size > 255) return B_BAD_VALUE;
	size_t actualLength;
	status_t result = gUSBModule->send_request(fDevice,
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		WRITE_REGISTER, 0, reg, size, buffer, &actualLength);
	return result;
}


status_t
DavicomDevice::_Write1Register(uint8 reg, uint8 value)
{
	size_t actualLength;
	status_t result = gUSBModule->send_request(fDevice,
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		WRITE1_REGISTER, 0, reg, 1, &value, &actualLength);
	return result;
}


DavicomDevice::DavicomDevice(usb_device device, const char *description)
	:	fStatus(B_ERROR),
		fOpen(false),
		fRemoved(false),
		fInsideNotify(0),
		fDevice(device),
		fDescription(description),
		fNonBlocking(false),
		fNotifyEndpoint(0),
		fReadEndpoint(0),
		fWriteEndpoint(0),
		fNotifyReadSem(-1),
		fNotifyWriteSem(-1),
		fNotifyBuffer(NULL),
		fLinkStateChangeSem(-1),
		fHasConnection(false)
{
	const usb_device_descriptor
			*deviceDescriptor = gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error of getting USB device descriptor.\n");
		return;
	}

	fVendorID = deviceDescriptor->vendor_id;
	fProductID = deviceDescriptor->product_id;

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

	fNotifyBuffer = (uint8*)malloc(kNotifyBufferSize);
	if (fNotifyBuffer == NULL) {
		TRACE_ALWAYS("Error allocating notify buffer\n");
		return;
	}

	if (_SetupEndpoints() != B_OK) {
		return;
	}

	// TODO : others inits here ?

	fStatus = B_OK;
}


DavicomDevice::~DavicomDevice()
{
	if (fNotifyReadSem >= B_OK)
		delete_sem(fNotifyReadSem);
	if (fNotifyWriteSem >= B_OK)
		delete_sem(fNotifyWriteSem);

	if (!fRemoved) //???
		gUSBModule->cancel_queued_transfers(fNotifyEndpoint);

	if(fNotifyBuffer)
		free(fNotifyBuffer);
}


status_t
DavicomDevice::Open(uint32 flags)
{
	if (fOpen)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	status_t result = StartDevice();
	if (result != B_OK) {
		return result;
	}

	fNonBlocking = (flags & O_NONBLOCK) == O_NONBLOCK;
	fOpen = true;
	return result;
}


status_t
DavicomDevice::Close()
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
DavicomDevice::Free()
{
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

	TRACE_FLOW("Request %d bytes.\n", numBytesToRead);

	uint8 header[kRXHeaderSize];
	iovec rxData[] = {
		{ &header, kRXHeaderSize },
		{ buffer,  numBytesToRead }
	};

	status_t result = gUSBModule->queue_bulk_v(fReadEndpoint,
		rxData, 1, _ReadCallback, this);
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

	if(fActualLengthRead < kRXHeaderSize) {
		TRACE_ALWAYS("Error: no place for TRXHeader:only %d of %d bytes.\n",
			fActualLengthRead, kRXHeaderSize);
		return B_ERROR; //TODO: ???
	}

	/*
	 * TODO :see what the first byte holds ?
	if(!header.IsValid()) {
		TRACE_ALWAYS("Error:TRX Header is invalid: len:%#04x; ilen:%#04x\n",
			header.fLength, header.fInvertedLength);
		return B_ERROR; //TODO: ???
	}
	*/

	*numBytes = header[1] | ( header[2] << 8 );

	if (header[0] & 0xBF ) {
		TRACE_ALWAYS("RX error %d occured !\n", header[0]);
	}

	if(fActualLengthRead - kRXHeaderSize > *numBytes) {
		TRACE_ALWAYS("MISMATCH of the frame length: hdr %d; received:%d\n",
			*numBytes, fActualLengthRead - kRXHeaderSize);
	}

	TRACE_FLOW("Read %d bytes.\n", *numBytes);
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
		TRACE_ALWAYS("Error of writing %d bytes to device while TX buffer full.\n",
			numBytesToWrite);
		return B_ERROR;
	}

	TRACE_FLOW("Write %d bytes.\n", numBytesToWrite);

	uint8 header[kTXHeaderSize];
	header[0] = *numBytes & 0xFF;
	header[1] = *numBytes >> 8;

	iovec txData[] = {
		{ &header, kTXHeaderSize },
		{ (uint8*)buffer, numBytesToWrite }
	};

	status_t result = gUSBModule->queue_bulk_v(fWriteEndpoint,
		txData, 2, _WriteCallback, this);
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

	*numBytes = fActualLengthWrite - kTXHeaderSize;;

	TRACE_FLOW("Written %d bytes.\n", *numBytes);
	return B_OK;
}


status_t
DavicomDevice::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case ETHER_INIT:
			TRACE_ALWAYS("ETHER_INIT\n");
			return B_OK;

		case ETHER_GETADDR:
			TRACE_ALWAYS("ETHER_GETADDR\n");
			memcpy(buffer, &fMACAddress, sizeof(fMACAddress));
			return B_OK;

		case ETHER_GETFRAMESIZE:
			*(uint32 *)buffer = 1518 /* fFrameSize */;
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
			return ModifyMulticastTable(true, *((uint8*)buffer));

		case ETHER_REMMULTI:
			TRACE("ETHER_REMMULTI\n");
			return ModifyMulticastTable(false, *((uint8*)buffer));

#if HAIKU_TARGET_PLATFORM_HAIKU
		case ETHER_SET_LINK_STATE_SEM:
			fLinkStateChangeSem = *(sem_id *)buffer;
			return B_OK;

		case ETHER_GET_LINK_STATE:
			TRACE_ALWAYS("ETHER_GET_LINK_STATE\n");
			return GetLinkState((ether_link_state *)buffer);
#endif

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
	/* First of all, we need to know the MAC address */
	ether_address address;
	status_t result = ReadMACAddress(&address);
	if(result != B_OK) {
		return result;
	}

	TRACE("MAC address is:%02x:%02x:%02x:%02x:%02x:%02x\n",
				address.ebyte[0], address.ebyte[1], address.ebyte[2],
				address.ebyte[3], address.ebyte[4], address.ebyte[5]);

	if(deviceReplugged) {
		// this might be the same device that was replugged - read the MAC
		// address (which should be at the same index) to make sure
		if(memcmp(&address, &fMACAddress, sizeof(address)) != 0) {
			TRACE_ALWAYS("Cannot replace device with MAC address:"
				"%02x:%02x:%02x:%02x:%02x:%02x\n",
				fMACAddress.ebyte[0], fMACAddress.ebyte[1], 
				fMACAddress.ebyte[2], fMACAddress.ebyte[3],
				fMACAddress.ebyte[4], fMACAddress.ebyte[5]);
			return B_BAD_VALUE; // is not the same
		}
	} else
		fMACAddress = address;


	/* Read the product ID, vendor ID, and chip revision (not used so far, but
	I feel the quirks coming in sooner or later !) */

	uint16 vidpid[3];
	vidpid[2] = 0; // We overwrite only the fist byte of this one.

	result = _ReadRegister(VID, 5, vidpid);
	if (result != B_OK)
		TRACE_ALWAYS("Error reading CHIPR: %#010x.\n", result);
	else
		TRACE_ALWAYS("Chip %#04x:%#04x revision %d\n", vidpid[0], vidpid[1],
			vidpid[2]);

	// setup state notifications (we need this to get linkup/linkdown events)
	result = gUSBModule->queue_interrupt(fNotifyEndpoint, fNotifyBuffer,
		kNotifyBufferSize, _NotifyCallback, this);
	if(result != B_OK) {
		TRACE_ALWAYS("Error of requesting notify interrupt:%#010x\n", result);
		return result;
	}
	
	// TODO enable "wakeup" at the device level or we'll never get anything !
	// TODO check if link was already up before enabling interrupts. If so, we
	// need to notify the network stack right now.

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

	if (deviceDescriptor->vendor_id != fVendorID
		&& deviceDescriptor->product_id != fProductID) {
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

	for(size_t ep = 0; ep < interface->endpoint_count; ep++) {
		usb_endpoint_descriptor *epd = interface->endpoint[ep].descr;
		
		// Is it an interrupt enpoint ?
		if((epd->attributes & USB_ENDPOINT_ATTR_MASK)
	  			== USB_ENDPOINT_ATTR_INTERRUPT) {
	    	notifyEndpoint = ep;
	    	continue;
	  	}
	  	
	  	// Is it a bulk one ?
	  	if((epd->attributes & USB_ENDPOINT_ATTR_MASK) != USB_ENDPOINT_ATTR_BULK) 		{
	    	TRACE_ALWAYS("Error: USB endpoint type %#04x is unknown.\n",
	    		epd->attributes);
	    	continue;
	  	}

		// If so, which direction ?
		if((epd->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
				== USB_ENDPOINT_ADDR_DIR_IN) {
	    	readEndpoint = ep;
			continue;
		}

		if((epd->endpoint_address & USB_ENDPOINT_ADDR_DIR_OUT)
				== USB_ENDPOINT_ADDR_DIR_OUT) {
	    	writeEndpoint = ep;
			continue;
		}
	}

	// Did we find all the needed endpoints ?
	if (notifyEndpoint == -1 || readEndpoint == -1 || writeEndpoint == -1) {
		TRACE_ALWAYS("Error: not all USB endpoints were found: "
			"notify:%d; read:%d; write:%d\n", notifyEndpoint, readEndpoint,
			writeEndpoint);
		return B_ERROR;
	}

	gUSBModule->set_configuration(fDevice, config);

	fNotifyEndpoint = interface->endpoint[notifyEndpoint].handle;
	fReadEndpoint   = interface->endpoint[readEndpoint  ].handle;
	fWriteEndpoint  = interface->endpoint[writeEndpoint ].handle;

	return B_OK;
}


status_t
DavicomDevice::ReadMACAddress(ether_address_t *address)
{
	status_t result = _ReadRegister(PAR, sizeof(ether_address),
		(uint8*)address);
	if(result != B_OK) {
		TRACE_ALWAYS("Error of reading MAC address:%#010x\n", result);
		return result;
	}

	return B_OK;
}


status_t
DavicomDevice::StopDevice()
{
	/*
	status_t result = WriteRXControlRegister(0);

	if(result != B_OK) {
		TRACE_ALWAYS("Error of writing %#04x RX Control:%#010x\n", 0, result);
	}

	TRACE_RET(result);
	return result;
	*/
	TRACE_ALWAYS("Stop device not implemented\n");
	return B_ERROR;
}


status_t
DavicomDevice::SetPromiscuousMode(bool on)
{

	/* load multicast filter and update promiscious mode bit */
	uint8_t rxmode;

	status_t result = _ReadRegister(RCR, 1, &rxmode);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading RX Control:%#010x\n", result);
		return result;
	}
	rxmode &= ~(RCR_ALL | RCR_PRMSC);

	if (on)
		rxmode |= RCR_ALL | RCR_PRMSC;
/*	else if (ifp->if_flags & IFF_ALLMULTI)
		rxmode |= RCR_ALL; */

	/* write new mode bits */
	result = _Write1Register(RCR, rxmode);
	if(result != B_OK) {
		TRACE_ALWAYS("Error writing %#04x to RX Control:%#010x\n", rxmode, result);
	}

	return result;
}


status_t
DavicomDevice::ModifyMulticastTable(bool add, uint8 address)
{
		//TODO: !!!
	TRACE_ALWAYS("Call for (%d, %#02x) is not implemented\n", add, address);
	return B_OK;
}


void
DavicomDevice::_ReadCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("ReadCB: %d bytes; status:%#010x\n", actualLength, status);
	DavicomDevice *device = (DavicomDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fNotifyReadSem, 1, B_DO_NOT_RESCHEDULE);
}


void
DavicomDevice::_WriteCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("WriteCB: %d bytes; status:%#010x\n", actualLength, status);
	DavicomDevice *device = (DavicomDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
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

	if (status != B_OK) {
		TRACE_ALWAYS("Device status error:%#010x\n", status);
		/*
		status_t result = gUSBModule->clear_feature(device->fNotifyEndpoint,
													USB_FEATURE_ENDPOINT_HALT);
		if(result != B_OK)
			TRACE_ALWAYS("Error during clearing of HALT state:%#010x.\n", result);
		*/
	}

	device->OnNotify(actualLength);

	// schedule next notification buffer
	gUSBModule->queue_interrupt(device->fNotifyEndpoint, device->fNotifyBuffer,
		kNotifyBufferSize, _NotifyCallback, device);
	atomic_add(&device->fInsideNotify, -1);
}


status_t
DavicomDevice::StartDevice()
{
	uint8 registerValue = 0;
	status_t result;
	
	/* disable loopback  */
	result = _ReadRegister(NCR, 1, &registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading NCR: %#010x.\n", result);
		return result;
	}
	if (registerValue & NCR_EXT_PHY)
		TRACE_ALWAYS("Device uses external PHY\n");
	registerValue &= ~NCR_LBK;
	result = _Write1Register(NCR, registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to NCR: %#010x.\n", registerValue, result);
		return result;
	}

	/* Initialize RX control register */
	result = _ReadRegister(RCR, 1, &registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading RCR: %#010x.\n", result);
		return result;
	}
	registerValue &= RCR_DIS_LONG & RCR_DIS_CRC & RCR_RXEN;
	result = _Write1Register(RCR, registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to RCR: %#010x.\n", registerValue, result);
		return result;
	}

	/* clear POWER_DOWN state of internal PHY */
	result = _ReadRegister(GPCR, 1, &registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading GPCR: %#010x.\n", result);
		return result;
	}
	registerValue |= GPCR_GEP_CNTL0;
	result = _Write1Register(GPCR, registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to GPCR: %#010x.\n", registerValue, result);
		return result;
	}

	result = _ReadRegister(GPR, 1, &registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading GPR: %#010x.\n", result);
		return result;
	}
	registerValue &= ~GPR_GEP_GEPIO0;
	result = _Write1Register(GPR, registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error writing %#02X to GPR: %#010x.\n", registerValue, result);
		return result;
	}

	return B_OK;
}


status_t
DavicomDevice::OnNotify(uint32 actualLength)
{
	if (actualLength != kNotifyBufferSize) {
		TRACE_ALWAYS("Data underrun error. %d of 8 bytes received\n",
			actualLength);
		return B_BAD_DATA;
	}

	// 3 = RX status
	// 4 = Receive overflow counter
	// 5 = Received packet counter
	// 6 = Transmit packet counter
	// 7 = GPR

	// 0 = Network status (NSR)
	bool linkIsUp = (fNotifyBuffer[0] & NSR_LINKST) != 0;
	fTXBufferFull = (fNotifyBuffer[0] & NSR_TXFULL) != 0;
	bool rxOverflow = (fNotifyBuffer[0] & NSR_RXOV) != 0;

	bool linkStateChange = (linkIsUp != fHasConnection);
	fHasConnection = linkIsUp;

	if (linkStateChange) {
		if (fHasConnection) {
			TRACE("Link is now up at %s Mb/s\n",
				(fNotifyBuffer[0] & NSR_SPEED) ? "10" : "100");
		} else
			TRACE("Link is now down.\n");
	}

	if (rxOverflow)
		TRACE("RX buffer overflow occured %d times\n", fNotifyBuffer[4]);

	// 1,2 = TX status
	if (fNotifyBuffer[1])
		TRACE("Error %x occured on transmitting packet 1\n", fNotifyBuffer[1]);
	if (fNotifyBuffer[2])
		TRACE("Error %x occured on transmitting packet 2\n", fNotifyBuffer[2]);

	if (linkStateChange && fLinkStateChangeSem >= B_OK)
		release_sem_etc(fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
	return B_OK;
}

status_t
DavicomDevice::GetLinkState(ether_link_state *linkState)
{
	uint8 registerValue = 0;
	status_t result = _ReadRegister(NSR, 1, &registerValue);
	if (result != B_OK) {
		TRACE_ALWAYS("Error reading NSR register! %x\n",result);
		return result;
	}

	if (registerValue & NSR_SPEED)
		linkState->speed = 10000000;
	else
		linkState->speed = 100000000;

	linkState->quality = 1000;

	linkState->media = IFM_ETHER | IFM_100_TX;
	if (fHasConnection) {
		linkState->media |= IFM_ACTIVE;
		result = _ReadRegister(NCR, 1, &registerValue);
		if (result != B_OK) {
			TRACE_ALWAYS("Error reading NCR register: %s\n",strerror(result));
			return result;
		}

		if (registerValue & NCR_FDX)
			linkState->media |= IFM_FULL_DUPLEX;
		else
			linkState->media |= IFM_HALF_DUPLEX;

		if (registerValue & NCR_LBK)
			linkState->media |= IFM_LOOP;
	}

	TRACE_FLOW("Medium state: %s, %lld MBit/s, %s duplex.\n",
						(linkState->media & IFM_ACTIVE) ? "active" : "inactive",
						linkState->speed,
						(linkState->media & IFM_FULL_DUPLEX) ? "full" : "half");
	return B_OK;
}

