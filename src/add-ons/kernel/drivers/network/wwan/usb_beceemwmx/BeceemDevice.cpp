/*
 *	Beceem WiMax USB Driver
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *
 *  Based on GPL code developed by: Beceem Communications Pvt. Ltd
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 *
 *	Partially using:
 *		USB Ethernet Control Model devices
 *			(c) 2008 by Michael Lotz, <mmlr@mlotz.ch>
 *		ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver
 *			(c) 2008 by S.Zharski, <imker@gmx.li>
 *
 *	This code is the entry point for the operating system to
 *	Beceem device communications.
 */


#include <sys/ioctl.h>
#include <unistd.h>

#include "util.h"

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <lock.h> // for mutex
#else
#include "BeOSCompatibility.h" // for pseudo mutex
#endif

#include "BeceemDevice.h"
#include "Driver.h"
#include "Settings.h"


mutex gUSBLock;


// auto-release helper class
class USBSmartLock {
public:
	USBSmartLock() { mutex_lock(&gUSBLock); }
	~USBSmartLock()	{ mutex_unlock(&gUSBLock); }
};


status_t
BeceemDevice::ReadRegister(uint32 reg, size_t size, uint32* buffer)
{
	USBSmartLock USBSubsystemLock; // released on exit

	if (size > 255) {
		TRACE_ALWAYS("Read too big! size = %d\n", size);
		return B_BAD_VALUE;
	}

	size_t actualLength = 0;
	int retries = 0;
	status_t result = B_ERROR;

	do {
		result = gUSBModule->send_request(fDevice,
			0xC0 | USB_REQTYPE_ENDPOINT_OUT, 0x02,
			(reg & 0xFFFF),
			((reg >> 16) & 0xFFFF),
			size, buffer,
			&actualLength);
		retries++ ;
		if (-ENODEV == result) {
			TRACE_ALWAYS("Error: Device was removed during USB read\n");
			break;
		}

	} while (result < 0 && retries < MAX_USB_IO_RETRIES);

	if (result < 0) {
		TRACE_ALWAYS("Error: USB read request failure."
			" Result: %d; Attempt: %d.\n", result, retries);
		return result;
	}


	if (size != actualLength) {
		TRACE_ALWAYS("Error: Size mismatch on USB read request."
			" Asked: %d; Got: %d; Attempt: %d.\n", size, actualLength, retries);
	}

	return result;
}


status_t
BeceemDevice::WriteRegister(uint32 reg, size_t size, uint32* buffer)
{
	USBSmartLock USBSubsystemLock; // released on exit

	if (size > 255) {
		TRACE_ALWAYS("Write too big! size = %d\n", size);
		return B_BAD_VALUE;
	}

	size_t actualLength = 0;
	int	 retries = 0;
	status_t result;

	do {
		result = gUSBModule->send_request(fDevice,
			0x40 | USB_REQTYPE_ENDPOINT_OUT, 0x01,
			(reg & 0xFFFF),
			((reg >> 16) & 0xFFFF),
			size, buffer,
			&actualLength);
		retries++ ;
		if (-ENODEV == result) {
			TRACE_ALWAYS("Error: Device was removed during USB write\n");
			break;
		}

	} while (result < 0 && retries < MAX_USB_IO_RETRIES);

	if (result < 0) {
		TRACE_ALWAYS("Error: USB write request failure."
			" Result: %d; Attempt: %d.\n",
			result, retries);

		return result;
	}

	if (size != actualLength) {
		TRACE_ALWAYS("Error: Size mismatch on USB write request."
			" Provided: %d; Took: %d; Attempt: %d.\n",
			size, actualLength, retries);
	}

	return result;
}


status_t
BeceemDevice::BizarroReadRegister(uint32 reg, size_t size,
	uint32* buffer)
{
	// NET_TO_HOST long

	// Read then flip
	status_t status = ReadRegister(reg, size, buffer);

	convertEndian(false, size, buffer);

	return status;
}


status_t
BeceemDevice::BizarroWriteRegister(uint32 reg, size_t size,
	uint32* buffer)
{
	// HOST_TO_NET long

	volatile uint32 reload = *buffer;

	convertEndian(true, size, buffer);

	status_t status = WriteRegister(reg, size, buffer);
		// Flip then write

	// as we modified the input data, and other things
	// outside this function may need it, restore the original val.
	*buffer = reload;

	return status;
}


BeceemDevice::BeceemDevice(usb_device device, const char *description)
	:
	fStatus(B_ERROR),
	fOpen(false),
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
	fNotifyBufferLength(0),
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

	mutex_init(&gUSBLock, DRIVER_NAME"_usbsubsys");

	// TODO : Investigate possible Notify Buffer sizes
	fNotifyBufferLength = 8;
	fNotifyBuffer = (uint8*)malloc(fNotifyBufferLength);
	if (fNotifyBuffer == NULL) {
		TRACE_ALWAYS("Error allocating notify buffer\n");
		return;
	}

	if ((pwmxdevice = (WIMAX_DEVICE*)malloc(sizeof(WIMAX_DEVICE))) == NULL)
		TRACE_ALWAYS("Error allocating Wimax device configuration buffer\n");

	if ((pwmxdevice->nvmFlashCSInfo
			= (PFLASH_CS_INFO)malloc(sizeof(FLASH_CS_INFO))) == NULL)
		TRACE_ALWAYS("Error allocating Flash CS info structure.\n");

	// set initial states
	pwmxdevice->LEDThreadID = 0;
	pwmxdevice->driverDDRinit = false;
	pwmxdevice->driverHalt = false;
	pwmxdevice->driverFwPushed = false;

	if (_SetupEndpoints() != B_OK) {
		return;
	}

	fStatus = B_OK;
}


BeceemDevice::~BeceemDevice()
{
	pwmxdevice->driverHalt = true;

	snooze(500);

	// Terminate LED thread cleanly
	if (pwmxdevice->LEDThreadID > 0) {
		LEDThreadTerminate();
		// Turn lights out
		LightsOut();
	}

	gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
	gUSBModule->cancel_queued_transfers(fReadEndpoint);
	gUSBModule->cancel_queued_transfers(fWriteEndpoint);

	if (fNotifyReadSem >= B_OK)
		delete_sem(fNotifyReadSem);
	if (fNotifyWriteSem >= B_OK)
		delete_sem(fNotifyWriteSem);

	free(fNotifyBuffer);
		// Free notification buffer
	free(pwmxdevice->nvmFlashCSInfo);
		// Free flash configuration structure
	free(pwmxdevice);
		// Free malloc of wimax device struct

	mutex_destroy(&gUSBLock);

	TRACE("Debug: BeceemDevice deconstruction complete\n");
}


status_t
BeceemDevice::Open(uint32 flags)
{
	if (fOpen)
		return B_BUSY;
	if (pwmxdevice->driverHalt)
		return B_ERROR;

	status_t result = StartDevice();
	if (result != B_OK)
		return result;

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
BeceemDevice::Close()
{
	if (pwmxdevice->driverHalt) {
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
BeceemDevice::Free()
{
	return B_OK;
}


// Network device Rx
status_t
BeceemDevice::Read(uint8 *buffer, size_t *numBytes)
{
	size_t numBytesToRead = *numBytes;
	*numBytes = 0;

	TRACE_FLOW("Request %d bytes.\n", numBytesToRead);

	if (pwmxdevice->driverHalt) {
		TRACE_ALWAYS("Error receiving %d bytes from removed device.\n",
			numBytesToRead);
		return B_DEVICE_NOT_FOUND;
	}

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

	// Safely tidy up after a major device hiccup
	if (fStatusRead != B_OK && fStatusRead != B_CANCELED
		&& !pwmxdevice->driverHalt) {
		TRACE_ALWAYS("Error: Device status error:%#010x\n", fStatusRead);
		result = gUSBModule->clear_feature(fReadEndpoint,
			USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK) {
			TRACE_ALWAYS("Error: Problem during clearing of HALT state: "
				"%#010x.\n", result);
			return result;
		}
	}

	if (fActualLengthRead < kRXHeaderSize) {
		TRACE_ALWAYS("Error: no place for TRXHeader:only %d of %d bytes.\n",
			fActualLengthRead, kRXHeaderSize);
		return B_ERROR; // TODO: ???
	}

	/*
	 * TODO :see what the first byte holds ?
	if (!header.IsValid()) {
		TRACE_ALWAYS("Error:TRX Header is invalid: len:%#04x; ilen:%#04x\n",
			header.fLength, header.fInvertedLength);
		return B_ERROR; // TODO: ???
	}
	*/

	*numBytes = header[1] | (header[2] << 8);

	if ((header[0] & 0xBF) != 0) {
		TRACE_ALWAYS("RX error %d occured !\n", header[0]);
	}

	if (fActualLengthRead - kRXHeaderSize > *numBytes) {
		TRACE_ALWAYS("MISMATCH of the frame length: hdr %d; received:%d\n",
			*numBytes, fActualLengthRead - kRXHeaderSize);
	}

	TRACE_FLOW("Read %d bytes.\n", *numBytes);
	return B_OK;
}


// Network device Tx
status_t
BeceemDevice::Write(const uint8 *buffer, size_t *numBytes)
{
	size_t numBytesToWrite = *numBytes;
	*numBytes = 0;

	if (pwmxdevice->driverHalt) {
		TRACE_ALWAYS("Error writing %d bytes to removed device.\n",
			numBytesToWrite);
		return B_DEVICE_NOT_FOUND;
	}

	if (!fHasConnection) {
		TRACE_ALWAYS("Error writing %d bytes to device"
			" while WiMAX connection down.\n",
			numBytesToWrite);
		return B_ERROR;
	}

	if (fTXBufferFull) {
		TRACE_ALWAYS("Error writing %d bytes to device"
			" while TX buffer full.\n",
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

	// Safely tidy up after a major device hiccup
	if (fStatusWrite != B_OK && fStatusWrite != B_CANCELED
			&& !pwmxdevice->driverHalt) {

		TRACE_ALWAYS("Device status error:%#010x\n", fStatusWrite);
		result = gUSBModule->clear_feature(fWriteEndpoint,
			USB_FEATURE_ENDPOINT_HALT);

		if (result != B_OK) {
			TRACE_ALWAYS("Error during clearing of HALT state:"
				" %#010x\n", result);
			return result;
		}
	}

	*numBytes = fActualLengthWrite - kTXHeaderSize;;

	TRACE_FLOW("Written %d bytes.\n", *numBytes);
	return B_OK;
}


status_t
BeceemDevice::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case ETHER_INIT:
			return B_OK;

		case ETHER_GETADDR:
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
			return GetLinkState((ether_link_state *)buffer);
#endif

		default:
			TRACE_ALWAYS("Error: Unhandled IOCTL catched: %#010x\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


void
BeceemDevice::Removed()
{
	fHasConnection = false;
	pwmxdevice->driverHalt = true;

	TRACE("Debug: Pre InsideNotify\n");

	// the notify hook is different from the read and write hooks as it does
	// itself schedule traffic (while the other hooks only release a semaphore
	// to notify another thread which in turn safly checks for the removed
	// case) - so we must ensure that we are not inside the notify hook anymore
	// before returning, as we would otherwise violate the promise not to use
	// any of the pipes after returning from the removed hook
	while (atomic_add(&fInsideNotify, 0) != 0)
		snooze(100);

	TRACE("Debug: Post InsideNotify\n");

	TRACE("Debug: Canceling queued USB transfers... [NotifyEndpoint]\n");
	gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
	TRACE("Debug: Canceling queued USB transfers... [ReadEndpoint]\n");
	gUSBModule->cancel_queued_transfers(fReadEndpoint);
	TRACE("Debug: Canceling queued USB transfers... [WriteEndpoint]\n");
	gUSBModule->cancel_queued_transfers(fWriteEndpoint);

	if (fLinkStateChangeSem >= B_OK)
		release_sem_etc(fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
}


/*! Identify Beceem Baseband chip and populate deviceChipID with it
 */
status_t
BeceemDevice::IdentifyChipset()
{

	if (BizarroReadRegister(CHIP_ID_REG, sizeof(uint32),
		&pwmxdevice->deviceChipID) != B_OK)
	{
		TRACE_ALWAYS("Error: Beceem device identification read failure\n");
		return B_ERROR;
	}

	switch(pwmxdevice->deviceChipID) {
		case T3:
			TRACE_ALWAYS(
				"Info: Identified Beceem T3 baseband chipset (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
		case T3B:
			TRACE_ALWAYS(
				"Info: Identified Beceem T3B baseband chipset (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
		case T3LPB:
			TRACE_ALWAYS(
				"Info: Identified Beceem T3LPB baseband chipset (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
		case BCS250_BC:
			TRACE_ALWAYS(
				"Info: Identified Beceem BC250_BC baseband chipset (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
		case BCS220_2:
			TRACE_ALWAYS(
				"Info: Identified Beceem BCS220_2 baseband chipset (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
		case BCS220_2BC:
			TRACE_ALWAYS(
				"Info: Identified Beceem BCS220_2BC baseband chipset (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
		case BCS220_3:
			TRACE_ALWAYS(
				"Info: Identified Beceem BCS220_3 baseband chipset (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
		default:
			TRACE_ALWAYS(
				"Warning: Unknown Beceem chipset detected (0x%x)\n",
				pwmxdevice->deviceChipID);
			break;
	}

	return B_OK;
}


status_t
BeceemDevice::SetupDevice(bool deviceReplugged)
{
	pwmxdevice->driverState = STATE_INIT;

	// ID the Beceem chipset
	IdentifyChipset();

	// init the CPU subsystem
	CPUInit(pwmxdevice);

	// Parse vendor config and put into struct
	if (LoadConfig() != B_OK)
		return B_ERROR;

	if (pwmxdevice->deviceChipID >= T3LPB) {
		uint32 value = 0;
		BizarroReadRegister(SYS_CFG, sizeof(value), &value);
		pwmxdevice->syscfgBefFw = value;
		if ((value & 0x60) == 0) {
			TRACE("Debug: CPU is FlashBoot\n");
			pwmxdevice->CPUFlashBoot = true;
		}
	}

	// take a quick break to let things settle
	snooze(100);

	CPUReset();

	// Initilize non-volatile memory
	if (NVMInit(pwmxdevice) != B_OK) {
		TRACE_ALWAYS("Error: Non-volatile memory initialization failure.\n");
		return B_ERROR;
	}

	// Initilize device DDR memory
	if (DDRInit(pwmxdevice) != B_OK) {
		TRACE_ALWAYS("Error: DDR Initialization failed\n");
		return B_ERROR;
	} else {
		TRACE("Debug: DDR Initialization successful.\n");
		pwmxdevice->driverDDRinit = true;
	}

	// Push vendor configuration to device
	// Each bcm chip has a custom binary config
	// telling the device about itself (how it was designed)
	if (PushConfig(CONF_BEGIN_ADDR) != B_OK) {
		TRACE_ALWAYS("Vendor configuration push failed."
			" Aborting device setup.\n");
		return B_ERROR;
	}

	// Set up GPIO (nvmVer 5+ is double sized)
	if (pwmxdevice->nvmVerMajor < 5) {
		TRACE("Debug: VerMajor < 5 PARAM pointer\n");
		NVMRead(GPIO_PARAM_POINTER, 2,
			(uint32*)&pwmxdevice->hwParamPtr);
		pwmxdevice->hwParamPtr = ntohs(pwmxdevice->hwParamPtr);
	} else {
		TRACE("Debug: VerMajor 5+ PARAM pointer\n");
		NVMRead(GPIO_PARAM_POINTER_MAP5, 4,
			(uint32*)&pwmxdevice->hwParamPtr);
		// TODO : NVM : validate v5+ nvm params a-la ValidateDSDParamsChecksum
		pwmxdevice->hwParamPtr = ntohl(pwmxdevice->hwParamPtr);
	}

	TRACE("Debug: Raw PARAM pointer: 0x%x\n", pwmxdevice->hwParamPtr);

	if (pwmxdevice->hwParamPtr < DSD_START_OFFSET
		|| pwmxdevice->hwParamPtr > pwmxdevice->nvmDSDSize - DSD_START_OFFSET) {
		TRACE_ALWAYS("Error: DSD Status checksum mismatch\n");
		return B_ERROR;
	}

	unsigned long dwReadValue = pwmxdevice->hwParamPtr;
		// hw paramater pointer
	dwReadValue = dwReadValue + DSD_START_OFFSET;
		// add DSD start offset
	dwReadValue = dwReadValue + GPIO_START_OFFSET;
		// add GPIO start offset

	NVMRead(dwReadValue, 32, (uint32*)&pwmxdevice->gpioInfo);
		// populate for LED information

	ValidateDSD(pwmxdevice->hwParamPtr);
		// validate the hardware DSD0 we calculated

	LEDInit(pwmxdevice);
		// Initilize LED on GPIO subsystem and spawn thread

	pwmxdevice->driverState = STATE_FWPUSH;

	status_t firm_push_result = PushFirmware(FIRM_BEGIN_ADDR);
		// Push firmware to device

	if (firm_push_result != B_OK) {
		TRACE_ALWAYS("Firmware push failed, aborting device setup.\n");
		pwmxdevice->driverFwPushed = false;
		return B_ERROR;
	} else {
		pwmxdevice->driverFwPushed = true;
		pwmxdevice->driverState = STATE_FWPUSH_OK;
		CPURun();
	}

	ether_address address;

	if (ReadMACFromNVM(&address) != B_OK)
		return B_ERROR;

	TRACE("MAC address is: %02x:%02x:%02x:%02x:%02x:%02x\n",
		address.ebyte[0], address.ebyte[1], address.ebyte[2],
		address.ebyte[3], address.ebyte[4], address.ebyte[5]);

	if (deviceReplugged) {
		// this might be the same device that was replugged - read the
		// mac address (which should be at the same index) to make sure
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

	pwmxdevice->driverState = STATE_NONET;

	// TODO : init Urb a-la StartInterruptUrb
	// TODO : wait for StartInterruptUrb
	// TODO : register_control_device_interface


	// TODO : Preserve stability
	// Things will break past this point until the ethernet interface is more
	// complete. Prevent completion of device initilization to preseve system
	// and network stack stability.

	#if 1
	TRACE_ALWAYS("A Beceem WiMax device was attached, "
		"however the Beceem driver is incomplete.\n");
	return B_ERROR;
	#endif

	return B_OK;
}


status_t
BeceemDevice::CompareAndReattach(usb_device device)
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
	// resetup the endpoints and transfers and open the device if it was
	// previously opened
	fDevice = device;
	pwmxdevice->driverHalt = false;

	status_t result = _SetupEndpoints();
	if (result != B_OK) {
		pwmxdevice->driverHalt = true;
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
BeceemDevice::_SetupEndpoints()
{
	const usb_configuration_info *config
		= gUSBModule->get_nth_configuration(fDevice, 0);

	if (config == NULL) {
		TRACE_ALWAYS("Error: Failed to get USB"
			" device configuration.\n");
		return B_ERROR;
	}

	if (config->interface_count <= 0) {
		TRACE_ALWAYS("Error: No interfaces found"
			" in USB device configuration\n");
		return B_ERROR;
	}

	usb_interface_info *interface = config->interface[0].active;
	if (interface == 0) {
		TRACE_ALWAYS("Error:invalid active interface in "
						"USB device configuration\n");
		return B_ERROR;
	}

	int notifyEndpoint = -1;
	int readEndpoint = -1;
	int writeEndpoint = -1;

	for (size_t ep = 0; ep < interface->endpoint_count; ep++) {
		usb_endpoint_descriptor *epd = interface->endpoint[ep].descr;

		if ((epd->attributes & USB_ENDPOINT_ATTR_MASK)
			== USB_ENDPOINT_ATTR_INTERRUPT) {
			// Interrupt endpoint
			notifyEndpoint = ep;
		} else if ((epd->attributes & USB_ENDPOINT_ATTR_MASK)
			== USB_ENDPOINT_ATTR_BULK) {

			// Bulk data endpoint
			if ((epd->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
				== USB_ENDPOINT_ADDR_DIR_IN)
				readEndpoint = ep;
			else if ((epd->endpoint_address & USB_ENDPOINT_ADDR_DIR_OUT)
				== USB_ENDPOINT_ADDR_DIR_OUT)
				writeEndpoint = ep;
			else
				TRACE_ALWAYS("Warning: BULK USB Endpoint"
					" %d (%#04x) is unknown.\n", ep, epd->attributes);
		} else if ((epd->attributes & USB_ENDPOINT_ATTR_MASK)
			== USB_ENDPOINT_ATTR_ISOCHRONOUS) {
			// Isochronous endpoint
			// TODO : do we need the Isochronous USB endpoints?
			// http://www.beyondlogic.org/usbnutshell/usb4.shtml#Isochronous
		} else {
			// Strange...
			TRACE_ALWAYS("Warning: USB Endpoint %d (%#04x) doesn't match known "
				"attribute mask.\n", ep, epd->attributes);
		}

	}

	if (notifyEndpoint == -1 || readEndpoint == -1 || writeEndpoint == -1) {
		TRACE_ALWAYS("Error: not all USB endpoints were found: "
			"notify:%d; read:%d; write:%d\n",
			notifyEndpoint, readEndpoint, writeEndpoint);
		return B_ERROR;
	}

	TRACE("Debug: USB Endpoints configured: notify %d; read %d; write %d\n",
		notifyEndpoint, readEndpoint, writeEndpoint);

	gUSBModule->set_configuration(fDevice, config);

	fNotifyEndpoint = interface->endpoint[notifyEndpoint].handle;
	fReadEndpoint = interface->endpoint[readEndpoint].handle;
	fWriteEndpoint = interface->endpoint[writeEndpoint].handle;

	return B_OK;
}


status_t
BeceemDevice::StopDevice()
{
	TRACE_ALWAYS("Stop device not implemented\n");
	return B_ERROR;
}


status_t
BeceemDevice::SetPromiscuousMode(bool on)
{
	// TODO : SetPromiscuousMode

	return B_OK;
}


status_t
BeceemDevice::ModifyMulticastTable(bool add, uint8 address)
{
	// TODO : ModifyMulticastTable
	TRACE_ALWAYS("Call for (%d, %#02x) is not implemented\n", add, address);
	return B_OK;
}


void
BeceemDevice::_ReadCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("ReadCB: %d bytes; status:%#010x\n", actualLength, status);
	BeceemDevice *device = (BeceemDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fNotifyReadSem, 1, B_DO_NOT_RESCHEDULE);
}


void
BeceemDevice::_WriteCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("WriteCB: %d bytes; status:%#010x\n", actualLength, status);
	BeceemDevice *device = (BeceemDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fNotifyWriteSem, 1, B_DO_NOT_RESCHEDULE);
}


void
BeceemDevice::_NotifyCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	BeceemDevice *device = (BeceemDevice *)cookie;
	atomic_add(&device->fInsideNotify, 1);
	if (status == B_CANCELED || device->pwmxdevice->driverHalt) {
		atomic_add(&device->fInsideNotify, -1);
		return;
	}

	if (status != B_OK) {
		TRACE_ALWAYS("Device status error :%#010x\n", status);
		status_t result = gUSBModule->clear_feature(device->fNotifyEndpoint,
			USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK)
			TRACE_ALWAYS("Error during clearing of HALT state:"
				" %#010x.\n", result);
	}

	// parse data in overriden class
	device->OnNotify(actualLength);

	// schedule next notification buffer
	gUSBModule->queue_interrupt(device->fNotifyEndpoint, device->fNotifyBuffer,
		device->fNotifyBufferLength, _NotifyCallback, device);
	atomic_add(&device->fInsideNotify, -1);
}


status_t
BeceemDevice::StartDevice()
{
	// TODO : StartDevice on open (might not be needed)
	return B_OK;
}


status_t
BeceemDevice::LoadConfig()
{
	int fh = open(FIRM_CFG, O_RDONLY);

	struct stat cfgStat;
	if (fh == B_ERROR || fstat(fh, &cfgStat) < 0) {
		TRACE_ALWAYS("Error: Unable to open the configuration at %s\n", FIRM_CFG);
		return fh;
	}

	size_t file_size = cfgStat.st_size;

	uint32* buffer = (uint32*)malloc(MAX_USB_TRANSFER);

	if (buffer == NULL) {
		TRACE_ALWAYS("Error: Memory allocation error.\n");
		return B_ERROR;
	}

	if (read(fh, buffer, MAX_USB_TRANSFER) < 0) {
		TRACE_ALWAYS("Error: Error reading from vendor configuration.\n");
		close(fh);
		free(buffer);
		return B_ERROR;
	}

	if (file_size != sizeof(VENDORCFG)) {
		TRACE_ALWAYS("Error: Size mismatch in vendor configuration struct!\n");
		close(fh);
		free(buffer);
		return B_ERROR;
	} else {
		TRACE_ALWAYS("Info: Found valid vendor configuration"
			" %ld bytes long.\n", file_size);
	}

	// we copy the configuration into our struct to know what the device knows
	memcpy(&pwmxdevice->vendorcfg, buffer, sizeof(VENDORCFG));

	DumpConfig();

	close(fh);
	free(buffer);

	return B_OK;
}


void
BeceemDevice::DumpConfig()
{
	TRACE("Debug: Vendor Config: Config File Version is 0x%x\n",
		pwmxdevice->vendorcfg.m_u32CfgVersion);
	TRACE("Debug: Vendor Config: Center Frequency is 0x%x\n",
		pwmxdevice->vendorcfg.m_u32CenterFrequency);
	TRACE("Debug: Vendor Config: Band A Scan = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32BandAScan);
	TRACE("Debug: Vendor Config: Band B Scan = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32BandBScan);
	TRACE("Debug: Vendor Config: Band C Scan = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32BandCScan);
	TRACE("Debug: Vendor Config: PHS Enable = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32PHSEnable);
	TRACE("Debug: Vendor Config: Handoff Enable = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32HoEnable);
	TRACE("Debug: Vendor Config: HO Reserved1 = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32HoReserved1);
	TRACE("Debug: Vendor Config: HO Reserved2 = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32HoReserved2);
	TRACE("Debug: Vendor Config: MIMO Enable = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32MimoEnable);
	TRACE("Debug: Vendor Config: PKMv2 Enable = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32SecurityEnable);
	TRACE("Debug: Vendor Config: Power Saving Modes Enable = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32PowerSavingModesEnable);
	TRACE("Debug: Vendor Config: Power Saving Mode Options = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32PowerSavingModeOptions);
	TRACE("Debug: Vendor Config: ARQ Enable = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32ArqEnable);
	TRACE("Debug: Vendor Config: Harq Enable = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32HarqEnable);
	TRACE("Debug: Vendor Config: EEPROM Flag = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32EEPROMFlag);
	TRACE("Debug: Vendor Config: Customize = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32Customize);
	TRACE("Debug: Vendor Config: Bandwidth = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32ConfigBW);
	TRACE("Debug: Vendor Config: RadioParameter = 0x%x\n",
		pwmxdevice->vendorcfg.m_u32RadioParameter);
	TRACE("Debug: Vendor Config: HostDrvrConfig1 is 0x%x\n",
		pwmxdevice->vendorcfg.HostDrvrConfig1);
	TRACE("Debug: Vendor Config: HostDrvrConfig2 is 0x%x\n",
		pwmxdevice->vendorcfg.HostDrvrConfig2);
	TRACE("Debug: Vendor Config: HostDrvrConfig3 is 0x%x\n",
		pwmxdevice->vendorcfg.HostDrvrConfig3);
	TRACE("Debug: Vendor Config: HostDrvrConfig4 is 0x%x\n",
		pwmxdevice->vendorcfg.HostDrvrConfig4);
	TRACE("Debug: Vendor Config: HostDrvrConfig5 is 0x%x\n",
		pwmxdevice->vendorcfg.HostDrvrConfig5);
	TRACE("Debug: Vendor Config: HostDrvrConfig6 is 0x%x\n",
		pwmxdevice->vendorcfg.HostDrvrConfig6);
}


status_t
BeceemDevice::PushConfig(uint32 loc)
{
	int fh = open(FIRM_CFG, O_RDONLY);

	struct stat cfgStat;
	if (fh == B_ERROR || fstat(fh, &cfgStat) < 0) {
		TRACE_ALWAYS("Error: Unable to open the configuration at %s\n",
			FIRM_CFG);
		return fh;
	}

	TRACE_ALWAYS("Info: Vendor configuration to be pushed to 0x%x on device.\n",
		loc);

	uint32* buffer = (uint32*)malloc(MAX_USB_TRANSFER);

	if (!buffer) {
		TRACE_ALWAYS("Error: Memory allocation error.\n");
		return B_ERROR;
	}

	int chipwriteloc = 0;
	int readposition = 0;
	status_t result;

	// We have to spoon feed the data to the usb device as it is probbably too
	// much to be written in one go.
	while (1) {
		readposition = read(fh, buffer, MAX_USB_TRANSFER);

		if (readposition <= 0) {
			if (readposition < 0) {
				TRACE_ALWAYS("Error: Error reading firmware.\n");
				result = B_ERROR;
			} else {
				TRACE_ALWAYS("Info: Got end of file!\n");
				result = B_OK;
			}
			break;
		}

		// Good data... but consumes too much I/O for larger file writes
		// TRACE("Debug: Read: %d, To Write %d - %d\n", readposition,
		//	chipwriteloc, chipwriteloc+readposition);

		// As readposition should always be less then MAX_USB_TRANSFER
		// and we have checked the validity of read's output above.
		if (WriteRegister(loc + chipwriteloc, readposition, buffer) != B_OK) {
			TRACE_ALWAYS("Write failure\n");
			result = B_ERROR;
			break;
		}

		// +1 as we don't want to write the same sector twice
		chipwriteloc += MAX_USB_TRANSFER + 1;
	}

	close(fh);
	free(buffer);

	if (result < 0) {
		TRACE_ALWAYS("Error: Push of vendor configuration failed: %d\n",
			result);
		return B_ERROR;
	} else {
		TRACE_ALWAYS("Info: Push of vendor configuration was successful.\n");
		return B_OK;
	}
}


status_t
BeceemDevice::PushFirmware(uint32 loc)
{
	int fh = open(FIRM_BIN, O_RDONLY);

	struct stat firmStat;
	if (fh == B_ERROR || fstat(fh, &firmStat) < 0) {
		TRACE_ALWAYS("Error: Unable to open the firmware at %s\n", FIRM_BIN);
		return fh;
	} else
		TRACE_ALWAYS("Info: Found firmware at %s.\n", FIRM_BIN);

	// The size of the firmware can very slightly
	//   CLEAR 1900 Beceem firmware is 2018596
	//   Sprint 1901 Beceem firmware is 2028080
	size_t file_size = firmStat.st_size;

	TRACE_ALWAYS("Info: %ld byte firmware to be pushed to 0x%x on device.\n",
		file_size, loc);

	// For the push we load the file into the buffer
	uint32* buffer = (uint32*)malloc(MAX_USB_TRANSFER);

	if (!buffer) {
		TRACE_ALWAYS("Error: Memory allocation error.\n");
		return B_ERROR;
	}

	// TODO : Firmware Download : investigate this, SHADOW clearing causes a KDL
	#if 0
	// Clear the NVM SHADOW signature always before fw download.
	WriteRegister(EEPROM_CAL_DATA_INTERNAL_LOC-4, 1, 0);
	WriteRegister(EEPROM_CAL_DATA_INTERNAL_LOC-8, 1, 0);
	#endif

	int chipwriteloc = 0;
	int readposition = 0;
	status_t result;

	// We have to spoon feed the data to the usb device as it is probbably too
	// much to be written in one go.
	while (1) {
		readposition = read(fh, buffer, MAX_USB_TRANSFER);

		if (readposition <= 0) {
			if (readposition < 0) {
				TRACE_ALWAYS("Error: Error reading firmware.\n");
				result = B_ERROR;
			} else {
				TRACE_ALWAYS("Info: Got end of file!\n");
				result = B_OK;
			}
			break;
		}

		// Good data... but consumes too much I/O for larger file writes
		// TRACE("Debug: Read: %d, To Write %d - %d\n", readposition,
		//	chipwriteloc, chipwriteloc + readposition);

		// As readposition should always be less then MAX_USB_TRANSFER
		// and we have checked the validity of read's output above.
		if (WriteRegister(loc + chipwriteloc, readposition, buffer) != B_OK) {
			TRACE_ALWAYS("Write failure\n");
			result = B_ERROR;
			break;
		}

		// +1 as we don't want to write the same sector twice
		chipwriteloc += MAX_USB_TRANSFER + 1;
	}

	free(buffer);
	close(fh);

	if (result != B_OK) {
		TRACE_ALWAYS("Error: Push of firmware to device failed :%d\n",
			result);
		return B_ERROR;
	} else {
		TRACE_ALWAYS("Info: Push of firmware to device was successful.\n");
		return B_OK;
	}
}


status_t
BeceemDevice::OnNotify(uint32 actualLength)
{
	// TODO : OnNotify  --  Device state change notification
	if (actualLength != fNotifyBufferLength) {
		TRACE_ALWAYS("Data underrun error. %d of 8 bytes received\n",
			actualLength);
		return B_BAD_DATA;
	}

	return B_OK;
}


status_t
BeceemDevice::GetLinkState(ether_link_state *linkState)
{
	// TODO : Report link state of WiMAX connection
	// media	as specified in net/if_media.h
	// quality	one tenth of a percent (0.1 == 10%, 1.0 == 100%)
	// speed	in bits / s

	// Our starting state
	linkState->media = IFM_ETHER | IFM_100_TX;

	if (fHasConnection == false) {
		linkState->quality = 0;
		linkState->speed = 100000000;
	} else {
		linkState->quality = 1.0;
		linkState->media |= IFM_ACTIVE;
		// 40Mbits is the maximum theoretical speed for a 802.16e connection.
		linkState->speed = 100000000;
	}

	return B_OK;
}

