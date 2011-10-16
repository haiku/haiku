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
		B_READ_AREA | B_WRITE_AREA);
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
		baud = B19200;
		speed = 19200;
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

	fDeviceOpen = false;
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
SerialDevice::_WriteCallbackFunction(void *cookie, status_t status, void *data,
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
SerialDevice::_InterruptCallbackFunction(void *cookie, status_t status,
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
			device->_InterruptCallbackFunction, device);
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

		case VENDOR_RENESAS:
		{
			switch (productID) {
				case 0x0053:
					description = "Renesas RX610 RX-Stick";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_AKATOM:
		{
			switch (productID) {
				case 0x066A:
					description = "AKTAKOM ACE-1001";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_PIRELLI:
		{
			switch (productID) {
				case 0xE000:
				case 0xE003:
					description = "Pirelli DP-L10 GSM Mobile";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_CYPHERLAB:
		{
			switch (productID) {
				case 0x1000:
					description = "Cipherlab CCD Barcode Scanner";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_GEMALTO:
		{
			switch (productID) {
				case 0x5501:
					description = "Gemalto contactless smartcard reader";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_DIGIANSWER:
		{
			switch (productID) {
				case 0x000A:
					description = "Digianswer ZigBee MAC device";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_MEI:
		{
			switch (productID) {
				case 0x1100:
				case 0x1101:
					description = "MEI Acceptor";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_DYNASTREAM:
		{
			switch (productID) {
				case 0x1003:
				case 0x1004:
				case 0x1006:
					description = "Dynastream ANT development board";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_KNOCKOFF:
		{
			switch (productID) {
				case 0xAA26:
					description = "Knock-off DCU-11";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_SIEMENS:
		{
			switch (productID) {
				case 0x10C5:
					description = "Siemens MC60";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_NOKIA:
		{
			switch (productID) {
				case 0xAC70:
					description = "Nokia CA-42";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_SILICON:
		{
			switch (productID) {
				case 0x0F91:
				case 0x1101:
				case 0x1601:
				case 0x800A:
				case 0x803B:
				case 0x8044:
				case 0x804E:
				case 0x8053:
				case 0x8054:
				case 0x8066:
				case 0x806F:
				case 0x807A:
				case 0x80CA:
				case 0x80DD:
				case 0x80F6:
				case 0x8115:
				case 0x813D:
				case 0x813F:
				case 0x814A:
				case 0x814B:
				case 0x8156:
				case 0x815E:
				case 0x818B:
				case 0x819F:
				case 0x81A6:
				case 0x81AC:
				case 0x81AD:
				case 0x81C8:
				case 0x81E2:
				case 0x81E7:
				case 0x81E8:
				case 0x81F2:
				case 0x8218:
				case 0x822B:
				case 0x826B:
				case 0x8293:
				case 0x82F9:
				case 0x8341:
				case 0x8382:
				case 0x83A8:
				case 0x83D8:
				case 0x8411:
				case 0x8418:
				case 0x846E:
				case 0x8477:
				case 0x85EA:
				case 0x85EB:
				case 0x8664:
				case 0x8665:
				case 0xEA60:
				case 0xEA61:
				case 0xEA71:
				case 0xF001:
				case 0xF002:
				case 0xF003:
				case 0xF004:
					description = "Silicon Labs CP210x USB UART converter";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_SILICON2:
		{
			switch (productID) {
				case 0xEA61:
					description = "Silicon Labs GPRS USB Modem";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_SILICON3:
		{
			switch (productID) {
				case 0xEA6A:
					description = "Silicon Labs GPRS USB Modem 100EU";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_BALTECH:
		{
			switch (productID) {
				case 0x9999:
					description = "Balteck card reader";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_OWEN:
		{
			switch (productID) {
				case 0x0004:
					description = "Owen AC4 USB-RS485 Converter";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_CLIPSAL:
		{
			switch (productID) {
				case 0x0303:
					description = "Clipsal 5500PCU C-Bus USB interface";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_JABLOTRON:
		{
			switch (productID) {
				case 0x0001:
					description = "Jablotron serial interface";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_WIENER:
		{
			switch (productID) {
				case 0x0010:
				case 0x0011:
				case 0x0012:
				case 0x0015:
					description = "W-IE-NE-R Plein & Baus GmbH device";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_WAVESENSE:
		{
			switch (productID) {
				case 0xAAAA:
					description = "Wavesense Jazz blood glucose meter";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_VAISALA:
		{
			switch (productID) {
				case 0x0200:
					description = "Vaisala USB instrument";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_ELV:
		{
			switch (productID) {
				case 0xE00F:
					description = "ELV USB I²C interface";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_WAGO:
		{
			switch (productID) {
				case 0x07A6:
					description = "WAGO 750-923 USB Service";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}
		case VENDOR_DW700:
		{
			switch (productID) {
				case 0x9500:
					description = "DW700 GPS USB interface";
					break;
			}

			if (description != NULL)
				goto SILICON;
			break;
		}

SILICON:
		return new(std::nothrow) SiliconDevice(device, vendorID, productID,
			description);
	}

	return new(std::nothrow) ACMDevice(device, vendorID, productID,
		"CDC ACM compatible device");
}
