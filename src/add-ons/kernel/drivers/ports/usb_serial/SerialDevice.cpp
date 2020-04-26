/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <new>

#include "SerialDevice.h"
#include "USB3.h"

#include "ACM.h"
#include "FTDI.h"
#include "KLSI.h"
#include "Option.h"
#include "Prolific.h"
#include "Silicon.h"

#include <sys/ioctl.h>


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
		fOutputBuffer(NULL),
		fOutputBufferSize(ROUNDUP(DEF_BUFFER_SIZE, 16)),
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
		fSystemTTYCookie(NULL),
		fDeviceTTYCookie(NULL),
		fInputThread(-1),
		fStopThreads(false)
{
	memset(&fTTYConfig, 0, sizeof(termios));
	fTTYConfig.c_cflag = B9600 | CS8 | CREAD;
}


SerialDevice::~SerialDevice()
{
	Removed();

	if (fDoneRead >= 0)
		delete_sem(fDoneRead);
	if (fDoneWrite >= 0)
		delete_sem(fDoneWrite);

	if (fBufferArea >= 0)
		delete_area(fBufferArea);
}


status_t
SerialDevice::Init()
{
	fDoneRead = create_sem(0, "usb_serial:done_read");
	if (fDoneRead < 0)
		return fDoneRead;

	fDoneWrite = create_sem(0, "usb_serial:done_write");
	if (fDoneWrite < 0)
		return fDoneWrite;

	size_t totalBuffers = fReadBufferSize + fOutputBufferSize + fWriteBufferSize
		+ fInterruptBufferSize;
	fBufferArea = create_area("usb_serial:buffers_area", (void **)&fReadBuffer,
		B_ANY_KERNEL_ADDRESS, ROUNDUP(totalBuffers, B_PAGE_SIZE), B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (fBufferArea < 0)
		return fBufferArea;

	fOutputBuffer = fReadBuffer + fReadBufferSize;
	fWriteBuffer = fOutputBuffer + fOutputBufferSize;
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


inline int32
baud_index_to_speed(int index)
{
	switch (index) {
		case B0: return 0;
		case B50: return 50;
		case B75: return 75;
		case B110: return 110;
		case B134: return 134;
		case B150: return 150;
		case B200: return 200;
		case B300: return 300;
		case B600: return 600;
		case B1200: return 1200;
		case B1800: return 1800;
		case B2400: return 2400;
		case B4800: return 4800;
		case B9600: return 9600;
		case B19200: return 19200;
		case B31250: return 31250;
		case B38400: return 38400;
		case B57600: return 57600;
		case B115200: return 115200;
		case B230400: return 230400;
	}

	TRACE_ALWAYS("invalid baud index %d\n", index);
	return -1;
}


void
SerialDevice::SetModes(struct termios *tios)
{
	TRACE_FUNCRES(trace_termios, tios);

	uint8 baud = tios->c_cflag & CBAUD;
	int32 speed = baud_index_to_speed(baud);
	if (speed < 0) {
		baud = CBAUD;
		speed = tios->c_ospeed;
	}

	// update our master config in full
	memcpy(&fTTYConfig, tios, sizeof(termios));
	fTTYConfig.c_cflag &= ~CBAUD;
	fTTYConfig.c_cflag |= baud;

	// only apply the relevant parts to the device side
	termios config;
	memset(&config, 0, sizeof(termios));
	config.c_cflag = tios->c_cflag;
	config.c_cflag &= ~CBAUD;
	config.c_cflag |= baud;

	// update the termios of the device side
	gTTYModule->tty_control(fDeviceTTYCookie, TCSETA, &config, sizeof(termios));

	SetHardwareFlowControl((tios->c_cflag & CRTSCTS) != 0);

	usb_cdc_line_coding lineCoding;
	lineCoding.speed = speed;
	lineCoding.stopbits = (tios->c_cflag & CSTOPB)
		? USB_CDC_LINE_CODING_2_STOPBITS : USB_CDC_LINE_CODING_1_STOPBIT;

	if (tios->c_cflag & PARENB) {
		lineCoding.parity = USB_CDC_LINE_CODING_EVEN_PARITY;
		if (tios->c_cflag & PARODD)
			lineCoding.parity = USB_CDC_LINE_CODING_ODD_PARITY;
	} else
		lineCoding.parity = USB_CDC_LINE_CODING_NO_PARITY;

	lineCoding.databits = (tios->c_cflag & CS8) ? 8 : 7;

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
	if (!fDeviceOpen)
		return false;

	if (tty != fMasterTTY)
		return false;

	switch (op) {
		case TTYENABLE:
		{
			bool enable = *(bool *)buffer;
			TRACE("TTYENABLE: %sable\n", enable ? "en" : "dis");

			gTTYModule->tty_hardware_signal(fSystemTTYCookie, TTYHWDCD, enable);
			gTTYModule->tty_hardware_signal(fSystemTTYCookie, TTYHWCTS, enable);

			fControlOut = enable ? USB_CDC_CONTROL_SIGNAL_STATE_DTR
				| USB_CDC_CONTROL_SIGNAL_STATE_RTS : 0;
			SetControlLineState(fControlOut);
			return true;
		}

		case TTYISTOP:
			fInputStopped = *(bool *)buffer;
			TRACE("TTYISTOP: %sstopped\n", fInputStopped ? "" : "not ");
			gTTYModule->tty_hardware_signal(fSystemTTYCookie, TTYHWCTS,
				!fInputStopped);
			return true;

		case TTYGETSIGNALS:
			TRACE("TTYGETSIGNALS\n");
			gTTYModule->tty_hardware_signal(fSystemTTYCookie, TTYHWDCD,
				(fControlOut & (USB_CDC_CONTROL_SIGNAL_STATE_DTR
					| USB_CDC_CONTROL_SIGNAL_STATE_RTS)) != 0);
			gTTYModule->tty_hardware_signal(fSystemTTYCookie, TTYHWCTS,
				!fInputStopped);
			gTTYModule->tty_hardware_signal(fSystemTTYCookie, TTYHWDSR, false);
			gTTYModule->tty_hardware_signal(fSystemTTYCookie, TTYHWRI, false);
			return true;

		case TTYSETMODES:
			TRACE("TTYSETMODES\n");
			SetModes((struct termios *)buffer);
			return true;

		case TTYSETDTR:
		case TTYSETRTS:
		{
			bool set = *(bool *)buffer;
			uint8 bit = op == TTYSETDTR ? USB_CDC_CONTROL_SIGNAL_STATE_DTR
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
		case TTYFLUSH:
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

	fSlaveTTY = gTTYModule->tty_create(usb_serial_service, false);
	if (fSlaveTTY == NULL) {
		TRACE_ALWAYS("open: failed to init slave tty\n");
		gTTYModule->tty_destroy(fMasterTTY);
		return B_NO_MEMORY;
	}

	fSystemTTYCookie = gTTYModule->tty_create_cookie(fMasterTTY, fSlaveTTY,
		O_RDWR);
	if (fSystemTTYCookie == NULL) {
		TRACE_ALWAYS("open: failed to init system tty cookie\n");
		gTTYModule->tty_destroy(fMasterTTY);
		gTTYModule->tty_destroy(fSlaveTTY);
		return B_NO_MEMORY;
	}

	fDeviceTTYCookie = gTTYModule->tty_create_cookie(fSlaveTTY, fMasterTTY,
		O_RDWR);
	if (fDeviceTTYCookie == NULL) {
		TRACE_ALWAYS("open: failed to init device tty cookie\n");
		gTTYModule->tty_destroy_cookie(fSystemTTYCookie);
		gTTYModule->tty_destroy(fMasterTTY);
		gTTYModule->tty_destroy(fSlaveTTY);
		return B_NO_MEMORY;
	}

	ResetDevice();

	fStopThreads = false;

	fInputThread = spawn_kernel_thread(_InputThread,
		"usb_serial input thread", B_NORMAL_PRIORITY, this);
	if (fInputThread < 0) {
		TRACE_ALWAYS("open: failed to spawn input thread\n");
		return fInputThread;
	}

	resume_thread(fInputThread);

	fControlOut = USB_CDC_CONTROL_SIGNAL_STATE_DTR
		| USB_CDC_CONTROL_SIGNAL_STATE_RTS;
	SetControlLineState(fControlOut);

	status_t status = gUSBModule->queue_interrupt(fControlPipe,
		fInterruptBuffer, fInterruptBufferSize, _InterruptCallbackFunction,
		this);
	if (status < B_OK)
		TRACE_ALWAYS("failed to queue initial interrupt\n");

	// set our config (will propagate to the slave config as well in SetModes()
	gTTYModule->tty_control(fSystemTTYCookie, TCSETA, &fTTYConfig,
		sizeof(termios));

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

	return gTTYModule->tty_read(fSystemTTYCookie, buffer, numBytes);
}


status_t
SerialDevice::Write(const char *buffer, size_t *numBytes)
{
	if (fDeviceRemoved) {
		*numBytes = 0;
		return B_DEV_NOT_READY;
	}

	size_t bytesLeft = *numBytes;
	*numBytes = 0;

	while (bytesLeft > 0) {
		size_t length = MIN(bytesLeft, 256);
			// TODO: This is an ugly hack; We use a small buffer size so that
			// we don't overrun the tty line buffer and cause it to block. While
			// that isn't a problem, we shouldn't just hardcode the value here.

		status_t result = gTTYModule->tty_write(fSystemTTYCookie, buffer,
			&length);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to write to tty: %s\n", strerror(result));
			return result;
		}

		buffer += length;
		*numBytes += length;
		bytesLeft -= length;

		while (true) {
			// Write to the device as long as there's anything in the tty buffer
			int readable = 0;
			gTTYModule->tty_control(fDeviceTTYCookie, FIONREAD, &readable,
				sizeof(readable));
			if (readable == 0)
				break;

			result = _WriteToDevice();
			if (result != B_OK) {
				TRACE_ALWAYS("failed to write to device: %s\n",
					strerror(result));
				return result;
			}
		}
	}

	if (*numBytes > 0)
		return B_OK;

	return B_ERROR;
}


status_t
SerialDevice::Control(uint32 op, void *arg, size_t length)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	return gTTYModule->tty_control(fSystemTTYCookie, op, arg, length);
}


status_t
SerialDevice::Select(uint8 event, uint32 ref, selectsync *sync)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	return gTTYModule->tty_select(fSystemTTYCookie, event, ref, sync);
}


status_t
SerialDevice::DeSelect(uint8 event, selectsync *sync)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	return gTTYModule->tty_deselect(fSystemTTYCookie, event, sync);
}


status_t
SerialDevice::Close()
{
	OnClose();

	fStopThreads = true;
	fInputStopped = false;
	fDeviceOpen = false;

	if (!fDeviceRemoved) {
		gUSBModule->cancel_queued_transfers(fReadPipe);
		gUSBModule->cancel_queued_transfers(fWritePipe);
		gUSBModule->cancel_queued_transfers(fControlPipe);
	}

	gTTYModule->tty_close_cookie(fSystemTTYCookie);
	gTTYModule->tty_close_cookie(fDeviceTTYCookie);

	int32 result = B_OK;
	wait_for_thread(fInputThread, &result);
	fInputThread = -1;

	gTTYModule->tty_destroy_cookie(fSystemTTYCookie);
	gTTYModule->tty_destroy_cookie(fDeviceTTYCookie);

	gTTYModule->tty_destroy(fMasterTTY);
	gTTYModule->tty_destroy(fSlaveTTY);

	fMasterTTY = NULL;
	fSlaveTTY = NULL;
	fSystemTTYCookie = NULL;
	fDeviceTTYCookie = NULL;
	return B_OK;
}


status_t
SerialDevice::Free()
{
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
	fStopThreads = true;
	fInputStopped = false;
	gUSBModule->cancel_queued_transfers(fReadPipe);
	gUSBModule->cancel_queued_transfers(fWritePipe);
	gUSBModule->cancel_queued_transfers(fControlPipe);
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
	return B_NOT_SUPPORTED;
}


status_t
SerialDevice::SetControlLineState(uint16 state)
{
	// default implementation - does nothing
	return B_NOT_SUPPORTED;
}


status_t
SerialDevice::SetHardwareFlowControl(bool enable)
{
	// default implementation - does nothing
	return B_NOT_SUPPORTED;
}


void
SerialDevice::OnRead(char **buffer, size_t *numBytes)
{
	// default implementation - does nothing
}


void
SerialDevice::OnWrite(const char *buffer, size_t *numBytes, size_t *packetBytes)
{
	memcpy(fWriteBuffer, buffer, *numBytes);
}


void
SerialDevice::OnClose()
{
	// default implementation - does nothing
}


int32
SerialDevice::_InputThread(void *data)
{
	SerialDevice *device = (SerialDevice *)data;

	while (!device->fStopThreads) {
		status_t status = gUSBModule->queue_bulk(device->fReadPipe,
			device->fReadBuffer, device->fReadBufferSize,
			device->_ReadCallbackFunction, data);
		if (status < B_OK) {
			TRACE_ALWAYS("input thread: queueing failed with error: 0x%08x\n",
				status);
			return status;
		}

		status = acquire_sem_etc(device->fDoneRead, 1, B_CAN_INTERRUPT, 0);
		if (status < B_OK) {
			TRACE_ALWAYS("input thread: failed to get read done sem 0x%08x\n",
				status);
			return status;
		}

		if (device->fStatusRead != B_OK) {
			TRACE("input thread: device status error 0x%08x\n",
				device->fStatusRead);
			if (device->fStatusRead == B_DEV_STALLED
				&& gUSBModule->clear_feature(device->fReadPipe,
					USB_FEATURE_ENDPOINT_HALT) != B_OK) {
				TRACE_ALWAYS("input thread: failed to clear halt feature\n");
				return B_ERROR;
			}

			continue;
		}

		char *buffer = device->fReadBuffer;
		size_t readLength = device->fActualLengthRead;
		device->OnRead(&buffer, &readLength);
		if (readLength == 0)
			continue;

		while (device->fInputStopped)
			snooze(100);

		status = gTTYModule->tty_write(device->fDeviceTTYCookie, buffer,
			&readLength);
		if (status != B_OK) {
			TRACE_ALWAYS("input thread: failed to write into TTY\n");
			return status;
		}
	}

	return B_OK;
}


status_t
SerialDevice::_WriteToDevice()
{
	char *buffer = fOutputBuffer;
	size_t bytesLeft = fOutputBufferSize;
	status_t status = gTTYModule->tty_read(fDeviceTTYCookie, buffer,
		&bytesLeft);
	if (status != B_OK) {
		TRACE_ALWAYS("write to device: failed to read from TTY: %s\n",
			strerror(status));
		return status;
	}

	while (!fDeviceRemoved && bytesLeft > 0) {
		size_t length = MIN(bytesLeft, fWriteBufferSize);
		size_t packetLength = length;
		OnWrite(buffer, &length, &packetLength);

		status = gUSBModule->queue_bulk(fWritePipe, fWriteBuffer, packetLength,
			_WriteCallbackFunction, this);
		if (status != B_OK) {
			TRACE_ALWAYS("write to device: queueing failed with status "
				"0x%08x\n", status);
			return status;
		}

		status = acquire_sem_etc(fDoneWrite, 1, B_CAN_INTERRUPT, 0);
		if (status != B_OK) {
			TRACE_ALWAYS("write to device: failed to get write done sem "
				"0x%08x\n", status);
			return status;
		}

		if (fStatusWrite != B_OK) {
			TRACE("write to device: device status error 0x%08x\n",
				fStatusWrite);
			if (fStatusWrite == B_DEV_STALLED) {
				status = gUSBModule->clear_feature(fWritePipe,
					USB_FEATURE_ENDPOINT_HALT);
				if (status != B_OK) {
					TRACE_ALWAYS("write to device: failed to clear device "
						"halt\n");
					return B_ERROR;
				}
			}

			continue;
		}

		buffer += length;
		bytesLeft -= length;
	}

	return B_OK;
}


void
SerialDevice::_ReadCallbackFunction(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	TRACE_FUNCALLS("read callback: cookie: 0x%08x status: 0x%08x data: 0x%08x "
		"length: %lu\n", cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fDoneRead, 1, B_DO_NOT_RESCHEDULE);
}


void
SerialDevice::_WriteCallbackFunction(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	TRACE_FUNCALLS("write callback: cookie: 0x%08x status: 0x%08x data: 0x%08x "
		"length: %lu\n", cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fDoneWrite, 1, B_DO_NOT_RESCHEDULE);
}


void
SerialDevice::_InterruptCallbackFunction(void *cookie, status_t status,
	void *data, size_t actualLength)
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
			device->_InterruptCallbackFunction, device);
	}
}


SerialDevice *
SerialDevice::MakeDevice(usb_device device, uint16 vendorID,
	uint16 productID)
{
	// FTDI Serial Device
	for (uint32 i = 0; i < sizeof(kFTDIDevices)
		/ sizeof(kFTDIDevices[0]); i++) {
		if (vendorID == kFTDIDevices[i].vendorID
			&& productID == kFTDIDevices[i].productID) {
			return new(std::nothrow) FTDIDevice(device, vendorID, productID,
				kFTDIDevices[i].deviceName);
		}
	}

	// KLSI Serial Device
	for (uint32 i = 0; i < sizeof(kKLSIDevices)
		/ sizeof(kKLSIDevices[0]); i++) {
		if (vendorID == kKLSIDevices[i].vendorID
			&& productID == kKLSIDevices[i].productID) {
			return new(std::nothrow) KLSIDevice(device, vendorID, productID,
				kKLSIDevices[i].deviceName);
		}
	}

	// Prolific Serial Device
	for (uint32 i = 0; i < sizeof(kProlificDevices)
		/ sizeof(kProlificDevices[0]); i++) {
		if (vendorID == kProlificDevices[i].vendorID
			&& productID == kProlificDevices[i].productID) {
			return new(std::nothrow) ProlificDevice(device, vendorID, productID,
				kProlificDevices[i].deviceName);
		}
	}

	// Silicon Serial Device
	for (uint32 i = 0; i < sizeof(kSiliconDevices)
		/ sizeof(kSiliconDevices[0]); i++) {
		if (vendorID == kSiliconDevices[i].vendorID
			&& productID == kSiliconDevices[i].productID) {
			return new(std::nothrow) SiliconDevice(device, vendorID, productID,
				kSiliconDevices[i].deviceName);
		}
	}

	// Option Serial Device
	for (uint32 i = 0; i < sizeof(kOptionDevices)
		/ sizeof(kOptionDevices[0]); i++) {
		if (vendorID == kOptionDevices[i].vendorID
			&& productID == kOptionDevices[i].productID) {
			return new(std::nothrow) OptionDevice(device, vendorID, productID,
				kOptionDevices[i].deviceName);
		}
	}

	// Otherwise, return standard ACM device
	return new(std::nothrow) ACMDevice(device, vendorID, productID,
		"CDC ACM compatible device");
}
