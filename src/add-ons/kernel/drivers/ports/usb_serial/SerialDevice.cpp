/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#include <new>

#include "SerialDevice.h"
#include "USB3.h"

#include "ACM.h"
#include "FTDI.h"
#include "KLSI.h"
#include "Prolific.h"

SerialDevice::SerialDevice(usb_device device, uint16 vendorID,
	uint16 productID, const char *description)
	:	fDevice(device),
		fVendorID(vendorID),
		fProductID(productID),
		fDescription(description),
		fDeviceOpen(false),
		fDeviceRemoved(false),
		fControlPipe(0),
		fReadPipe(0),
		fWritePipe(0),
		fBufferArea(-1),
		fReadBuffer(NULL),
		fReadBufferSize(ROUNDUP(DEF_BUFFER_SIZE, 16)),
		fWriteBuffer(NULL),
		fWriteBufferSize(ROUNDUP(DEF_BUFFER_SIZE, 16)),
		fInterruptBuffer(NULL),
		fInterruptBufferSize(16),
		fDoneRead(-1),
		fDoneWrite(-1),
		fControlOut(0),
		fInputStopped(false),
		fMasterTTY(NULL),
		fSlaveTTY(NULL),
		fTTYCookie(NULL),
		fDeviceThread(-1),
		fStopDeviceThread(false)
{
	mutex_init(&fReadLock, "usb_serial read lock");
	mutex_init(&fWriteLock, "usb_serial write lock");
}


SerialDevice::~SerialDevice()
{
	Removed();

	if (fDoneRead >= B_OK)
		delete_sem(fDoneRead);
	if (fDoneWrite >= B_OK)
		delete_sem(fDoneWrite);

	if (fBufferArea >= B_OK)
		delete_area(fBufferArea);

	mutex_destroy(&fReadLock);
	mutex_destroy(&fWriteLock);
}


status_t
SerialDevice::Init()
{
	fDoneRead = create_sem(0, "usb_serial:done_read");
	fDoneWrite = create_sem(0, "usb_serial:done_write");
	mutex_init(&fReadLock, "usb_serial:read_lock");
	mutex_init(&fWriteLock, "usb_serial:write_lock");

	size_t totalBuffers = fReadBufferSize + fWriteBufferSize
		+ fInterruptBufferSize;
	fBufferArea = create_area("usb_serial:buffers_area", (void **)&fReadBuffer,
		B_ANY_KERNEL_ADDRESS, ROUNDUP(totalBuffers, B_PAGE_SIZE), B_CONTIGUOUS,
		B_READ_AREA | B_WRITE_AREA);

	fWriteBuffer = fReadBuffer + fReadBufferSize;
	fInterruptBuffer = fWriteBuffer + fWriteBufferSize;
	return B_OK;
}


void
SerialDevice::SetControlPipe(usb_pipe handle)
{
	fControlPipe = handle;
}


void
SerialDevice::SetReadPipe(usb_pipe handle)
{
	fReadPipe = handle;
}


void
SerialDevice::SetWritePipe(usb_pipe handle)
{
	fWritePipe = handle;
}


void
SerialDevice::SetModes(struct termios *tios)
{
	uint16 newControl = fControlOut;
	TRACE_FUNCRES(trace_termios, tios);

	static uint32 baudRates[] = {
		0x00000000, //B0
		0x00000032, //B50
		0x0000004B, //B75
		0x0000006E, //B110
		0x00000086, //B134
		0x00000096, //B150
		0x000000C8, //B200
		0x0000012C, //B300
		0x00000258, //B600
		0x000004B0, //B1200
		0x00000708, //B1800
		0x00000960, //B2400
		0x000012C0, //B4800
		0x00002580, //B9600
		0x00004B00, //B19200
		0x00009600, //B38400
		0x0000E100, //B57600
		0x0001C200, //B115200
		0x00038400, //B230400
		0x00070800, //460800
		0x000E1000, //921600
	};

	uint32 baudCount = sizeof(baudRates) / sizeof(baudRates[0]);
	uint32 baudIndex = tios->c_cflag & CBAUD;
	if (baudIndex == 0)
		baudIndex = tios->c_ispeed;
	if (baudIndex == 0)
		baudIndex = tios->c_ospeed;
	if (baudIndex > baudCount)
		baudIndex = baudCount - 1;

	usb_cdc_line_coding lineCoding;
	lineCoding.speed = baudRates[baudIndex];
	lineCoding.stopbits = (tios->c_cflag & CSTOPB)
		? USB_CDC_LINE_CODING_2_STOPBITS : USB_CDC_LINE_CODING_1_STOPBIT;

	if (tios->c_cflag & PARENB) {
		lineCoding.parity = USB_CDC_LINE_CODING_EVEN_PARITY;
		if (tios->c_cflag & PARODD)
			lineCoding.parity = USB_CDC_LINE_CODING_ODD_PARITY;
	} else
		lineCoding.parity = USB_CDC_LINE_CODING_NO_PARITY;

	lineCoding.databits = (tios->c_cflag & CS8) ? 8 : 7;

	if (fControlOut != newControl) {
		fControlOut = newControl;
		TRACE("newctrl send to modem: 0x%08x\n", newControl);
		SetControlLineState(newControl);
	}

	if (memcmp(&lineCoding, &fLineCoding, sizeof(usb_cdc_line_coding)) != 0) {
		fLineCoding.speed = lineCoding.speed;
		fLineCoding.stopbits = lineCoding.stopbits;
		fLineCoding.databits = lineCoding.databits;
		fLineCoding.parity = lineCoding.parity;
		TRACE("send to modem: speed %d sb: 0x%08x db: 0x%08x parity: 0x%08x\n",
			fLineCoding.speed, fLineCoding.stopbits, fLineCoding.databits,
			fLineCoding.parity);
		SetLineCoding(&fLineCoding);
	}
}


bool
SerialDevice::Service(struct tty *tty, uint32 op, void *buffer, size_t length)
{
	if (tty != fMasterTTY)
		return false;

	switch (op) {
		case TTYENABLE:
		{
			bool enable = *(bool *)buffer;
			TRACE("TTYENABLE: %sable\n", enable ? "en" : "dis");

			gTTYModule->tty_hardware_signal(fTTYCookie, TTYHWDCD, enable);
			gTTYModule->tty_hardware_signal(fTTYCookie, TTYHWCTS, enable);

			fControlOut = enable ? USB_CDC_CONTROL_SIGNAL_STATE_DTR
				| USB_CDC_CONTROL_SIGNAL_STATE_RTS : 0;
			SetControlLineState(fControlOut);
			return true;
		}

		case TTYISTOP:
			fInputStopped = *(bool *)buffer;
			TRACE("TTYISTOP: %sstopped\n", fInputStopped ? "" : "not ");
			gTTYModule->tty_hardware_signal(fTTYCookie, TTYHWCTS,
				!fInputStopped);
			return true;

		case TTYGETSIGNALS:
			TRACE("TTYGETSIGNALS\n");
			gTTYModule->tty_hardware_signal(fTTYCookie, TTYHWDCD,
				(fControlOut & (USB_CDC_CONTROL_SIGNAL_STATE_DTR
					| USB_CDC_CONTROL_SIGNAL_STATE_RTS)) != 0);
			gTTYModule->tty_hardware_signal(fTTYCookie, TTYHWCTS,
				!fInputStopped);
			gTTYModule->tty_hardware_signal(fTTYCookie, TTYHWDSR, false);
			gTTYModule->tty_hardware_signal(fTTYCookie, TTYHWRI, false);
			return true;

		case TTYSETMODES:
			TRACE("TTYSETMODES\n");
			SetModes((struct termios *)buffer);
			return true;

		case TTYSETDTR:
		case TTYSETRTS:
		{
			bool set = *(bool *)buffer;
			uint8 bit = TTYSETDTR ? USB_CDC_CONTROL_SIGNAL_STATE_DTR
				: USB_CDC_CONTROL_SIGNAL_STATE_RTS;
			if (set)
				fControlOut |= bit;
			else
				fControlOut &= ~bit;

			SetControlLineState(fControlOut);
			return true;
		}

		case TTYOSTART:
		case TTYOSYNC:
		case TTYSETBREAK:
			TRACE("TTY other\n");
			return true;
	}

	return false;
}


status_t
SerialDevice::Open(uint32 flags)
{
	if (fDeviceOpen)
		return B_BUSY;

	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	fMasterTTY = gTTYModule->tty_create(usb_serial_service, true);
	if (fMasterTTY == NULL) {
		TRACE_ALWAYS("open: failed to init master tty\n");
		return B_NO_MEMORY;
	}

	fSlaveTTY = gTTYModule->tty_create(NULL, false);
	if (fSlaveTTY == NULL) {
		TRACE_ALWAYS("open: failed to init slave tty\n");
		gTTYModule->tty_destroy(fMasterTTY);
		return B_NO_MEMORY;
	}

	fTTYCookie = gTTYModule->tty_create_cookie(fMasterTTY, fSlaveTTY, flags);
	if (fTTYCookie == NULL) {
		TRACE_ALWAYS("open: failed to init tty cookie\n");
		gTTYModule->tty_destroy(fMasterTTY);
		gTTYModule->tty_destroy(fSlaveTTY);
		return B_NO_MEMORY;
	}

	ResetDevice();

	fDeviceThread = spawn_kernel_thread(DeviceThread,
		"usb_serial device thread", B_NORMAL_PRIORITY, this);

	if (fDeviceThread < B_OK) {
		TRACE_ALWAYS("open: failed to spawn kernel thread\n");
		return fDeviceThread;
	}

	resume_thread(fDeviceThread);

	fControlOut = USB_CDC_CONTROL_SIGNAL_STATE_DTR
		| USB_CDC_CONTROL_SIGNAL_STATE_RTS;
	SetControlLineState(fControlOut);

	status_t status = gUSBModule->queue_interrupt(fControlPipe,
		fInterruptBuffer, fInterruptBufferSize, InterruptCallbackFunction,
		this);
	if (status < B_OK)
		TRACE_ALWAYS("failed to queue initial interrupt\n");

	fDeviceOpen = true;
	return B_OK;
}


status_t
SerialDevice::Read(char *buffer, size_t *numBytes)
{
	if (fDeviceRemoved) {
		*numBytes = 0;
		return B_DEV_NOT_READY;
	}

	status_t status = mutex_lock(&fReadLock);
	if (status != B_OK) {
		TRACE_ALWAYS("read: failed to get read lock\n");
		*numBytes = 0;
		return status;
	}

	status = gTTYModule->tty_read(fTTYCookie, buffer, numBytes);

	mutex_unlock(&fReadLock);
	return status;
}


status_t
SerialDevice::Write(const char *buffer, size_t *numBytes)
{
	size_t bytesLeft = *numBytes;
	*numBytes = 0;

	status_t status = mutex_lock(&fWriteLock);
	if (status != B_OK) {
		TRACE_ALWAYS("write: failed to get write lock\n");
		return status;
	}

	if (fDeviceRemoved) {
		mutex_unlock(&fWriteLock);
		return B_DEV_NOT_READY;
	}

	while (bytesLeft > 0) {
		size_t length = MIN(bytesLeft, fWriteBufferSize);
		size_t packetLength = length;
		OnWrite(buffer, &length, &packetLength);

		status = gUSBModule->queue_bulk(fWritePipe, fWriteBuffer,
			packetLength, WriteCallbackFunction, this);
		if (status < B_OK) {
			TRACE_ALWAYS("write: queueing failed with status 0x%08x\n", status);
			break;
		}

		status = acquire_sem_etc(fDoneWrite, 1, B_CAN_INTERRUPT, 0);
		if (status < B_OK) {
			TRACE_ALWAYS("write: failed to get write done sem 0x%08x\n",
				status);
			break;
		}

		if (fStatusWrite != B_OK) {
			TRACE("write: device status error 0x%08x\n", fStatusWrite);
			status = gUSBModule->clear_feature(fWritePipe,
				USB_FEATURE_ENDPOINT_HALT);
			if (status < B_OK) {
				TRACE_ALWAYS("write: failed to clear device halt\n");
				status = B_ERROR;
				break;
			}
			continue;
		}

		buffer += length;
		*numBytes += length;
		bytesLeft -= length;
	}

	mutex_unlock(&fWriteLock);
	return status;
}


status_t
SerialDevice::Control(uint32 op, void *arg, size_t length)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	return gTTYModule->tty_control(fTTYCookie, op, arg, length);
}


status_t
SerialDevice::Select(uint8 event, uint32 ref, selectsync *sync)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	return gTTYModule->tty_select(fTTYCookie, event, ref, sync);
}


status_t
SerialDevice::DeSelect(uint8 event, selectsync *sync)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	return gTTYModule->tty_deselect(fTTYCookie, event, sync);
}


status_t
SerialDevice::Close()
{
	OnClose();

	if (!fDeviceRemoved) {
		gUSBModule->cancel_queued_transfers(fReadPipe);
		gUSBModule->cancel_queued_transfers(fWritePipe);
		gUSBModule->cancel_queued_transfers(fControlPipe);
	}

	gTTYModule->tty_destroy_cookie(fTTYCookie);

	fDeviceOpen = false;
	return B_OK;
}


status_t
SerialDevice::Free()
{
	gTTYModule->tty_destroy(fMasterTTY);
	gTTYModule->tty_destroy(fSlaveTTY);
	return B_OK;
}


void
SerialDevice::Removed()
{
	if (fDeviceRemoved)
		return;

	// notifies us that the device was removed
	fDeviceRemoved = true;

	// we need to ensure that we do not use the device anymore
	fStopDeviceThread = true;
	fInputStopped = false;
	gUSBModule->cancel_queued_transfers(fReadPipe);
	gUSBModule->cancel_queued_transfers(fWritePipe);
	gUSBModule->cancel_queued_transfers(fControlPipe);

	int32 result = B_OK;
	wait_for_thread(fDeviceThread, &result);
	fDeviceThread = -1;

	mutex_lock(&fWriteLock);
	mutex_unlock(&fWriteLock);
}


status_t
SerialDevice::AddDevice(const usb_configuration_info *config)
{
	// default implementation - does nothing
	return B_ERROR;
}


status_t
SerialDevice::ResetDevice()
{
	// default implementation - does nothing
	return B_OK;
}


status_t
SerialDevice::SetLineCoding(usb_cdc_line_coding *coding)
{
	// default implementation - does nothing
	return B_OK;
}


status_t
SerialDevice::SetControlLineState(uint16 state)
{
	// default implementation - does nothing
	return B_OK;
}


void
SerialDevice::OnRead(char **buffer, size_t *numBytes)
{
	// default implementation - does nothing
}


void
SerialDevice::OnWrite(const char *buffer, size_t *numBytes, size_t *packetBytes)
{
	// default implementation - does nothing
}


void
SerialDevice::OnClose()
{
	// default implementation - does nothing
}


int32
SerialDevice::DeviceThread(void *data)
{
	SerialDevice *device = (SerialDevice *)data;

	while (!device->fStopDeviceThread) {
		status_t status = gUSBModule->queue_bulk(device->fReadPipe,
			device->fReadBuffer, device->fReadBufferSize,
			device->ReadCallbackFunction, data);
		if (status < B_OK) {
			TRACE_ALWAYS("device thread: queueing failed with error: 0x%08x\n",
				status);
			break;
		}

		status = acquire_sem_etc(device->fDoneRead, 1, B_CAN_INTERRUPT, 0);
		if (status < B_OK) {
			TRACE_ALWAYS("device thread: failed to get read done sem 0x%08x\n",
				status);
			break;
		}

		if (device->fStatusRead != B_OK) {
			TRACE("device thread: device status error 0x%08x\n",
				device->fStatusRead);
			if (gUSBModule->clear_feature(device->fReadPipe,
				USB_FEATURE_ENDPOINT_HALT) != B_OK) {
				TRACE_ALWAYS("device thread: failed to clear halt feature\n");
				break;
			}
		}

		char *buffer = device->fReadBuffer;
		size_t readLength = device->fActualLengthRead;
		device->OnRead(&buffer, &readLength);
		if (readLength == 0)
			continue;

		while (device->fInputStopped)
			snooze(100);

		gTTYModule->tty_write(device->fTTYCookie, buffer, &readLength);
	}

	return B_OK;
}


void
SerialDevice::ReadCallbackFunction(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FUNCALLS("read callback: cookie: 0x%08x status: 0x%08x data: 0x%08x "
		"length: %lu\n", cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fDoneRead, 1, B_DO_NOT_RESCHEDULE);
}


void
SerialDevice::WriteCallbackFunction(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FUNCALLS("write callback: cookie: 0x%08x status: 0x%08x data: 0x%08x "
		"length: %lu\n", cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fDoneWrite, 1, B_DO_NOT_RESCHEDULE);
}


void
SerialDevice::InterruptCallbackFunction(void *cookie, int32 status,
	void *data, uint32 actualLength)
{
	TRACE_FUNCALLS("interrupt callback: cookie: 0x%08x status: 0x%08x data: "
		"0x%08x len: %lu\n", cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthInterrupt = actualLength;
	device->fStatusInterrupt = status;

	// ToDo: maybe handle those somehow?

	if (status == B_OK && !device->fDeviceRemoved) {
		status = gUSBModule->queue_interrupt(device->fControlPipe,
			device->fInterruptBuffer, device->fInterruptBufferSize,
			device->InterruptCallbackFunction, device);
	}
}


SerialDevice *
SerialDevice::MakeDevice(usb_device device, uint16 vendorID,
	uint16 productID)
{
	const char *description = NULL;

	switch (vendorID) {
		case VENDOR_IODATA:
		case VENDOR_ATEN:
		case VENDOR_TDK:
		case VENDOR_RATOC:
		case VENDOR_PROLIFIC:
		case VENDOR_ELECOM:
		case VENDOR_SOURCENEXT:
		case VENDOR_HAL:
		{
			switch (productID) {
				case PRODUCT_PROLIFIC_RSAQ2:
					description = "PL2303 Serial adapter (IODATA USB-RSAQ2)";
					break;
				case PRODUCT_IODATA_USBRSAQ:
					description = "I/O Data USB serial adapter USB-RSAQ1";
					break;
				case PRODUCT_ATEN_UC232A:
					description = "Aten Serial adapter";
					break;
				case PRODUCT_TDK_UHA6400:
					description = "TDK USB-PHS Adapter UHA6400";
					break;
				case PRODUCT_RATOC_REXUSB60:
					description = "Ratoc USB serial adapter REX-USB60";
					break;
				case PRODUCT_PROLIFIC_PL2303:
					description = "PL2303 Serial adapter (ATEN/IOGEAR UC232A)";
					break;
				case PRODUCT_ELECOM_UCSGT:
					description = "Elecom UC-SGT";
					break;
				case PRODUCT_SOURCENEXT_KEIKAI8:
					description = "SOURCENEXT KeikaiDenwa 8";
					break;
				case PRODUCT_SOURCENEXT_KEIKAI8_CHG:
					description = "SOURCENEXT KeikaiDenwa 8 with charger";
					break;
				case PRODUCT_HAL_IMR001:
					description = "HAL Corporation Crossam2+USB";
					break;
			}

			if (description == NULL)
				break;

			return new(std::nothrow) ProlificDevice(device, vendorID, productID,
				description);
		}

		case VENDOR_FTDI:
		{
			switch (productID) {
				case PRODUCT_FTDI_8U100AX:
					description = "FTDI 8U100AX serial converter";
					break;
				case PRODUCT_FTDI_8U232AM:
					description = "FTDI 8U232AM serial converter";
					break;
			}

			if (description == NULL)
				break;

			return new(std::nothrow) FTDIDevice(device, vendorID, productID,
				description);
		}

		case VENDOR_PALM:
		case VENDOR_KLSI:
		{
			switch (productID) {
				case PRODUCT_PALM_CONNECT:
					description = "PalmConnect RS232";
					break;
				case PRODUCT_KLSI_KL5KUSB105D:
					description = "KLSI KL5KUSB105D";
					break;
			}

			if (description == NULL)
				break;

			return new(std::nothrow) KLSIDevice(device, vendorID, productID,
				description);
		}
	}

	return new(std::nothrow) ACMDevice(device, vendorID, productID,
		"CDC ACM compatible device");
}
