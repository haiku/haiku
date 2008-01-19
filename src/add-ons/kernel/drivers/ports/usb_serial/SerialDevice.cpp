/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
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
		fControlPipe(0),
		fReadPipe(0),
		fWritePipe(0),
		fBufferArea(-1),
		fReadBuffer(NULL),
		fReadBufferSize(0),
		fWriteBuffer(NULL),
		fWriteBufferSize(0),
		fInterruptBuffer(NULL),
		fInterruptBufferSize(0),
		fDoneRead(-1),
		fDoneWrite(-1),
		fControlOut(0),
		fInputStopped(false),
		fDeviceThread(-1),
		fStopDeviceThread(false)
{
	memset(&fTTYFile, 0, sizeof(ttyfile));
	memset(&fTTY, 0, sizeof(tty));
}


SerialDevice::~SerialDevice()
{
	fStopDeviceThread = true;
	gUSBModule->cancel_queued_transfers(fReadPipe);
	gUSBModule->cancel_queued_transfers(fWritePipe);
	gUSBModule->cancel_queued_transfers(fControlPipe);

	if (fDoneRead >= B_OK)
		delete_sem(fDoneRead);
	if (fDoneWrite >= B_OK)
		delete_sem(fDoneWrite);

	int32 result = B_OK;
	wait_for_thread(fDeviceThread, &result);

	if (fBufferArea >= B_OK)
		delete_area(fBufferArea);

	benaphore_destroy(&fReadLock);
	benaphore_destroy(&fWriteLock);
}


status_t
SerialDevice::Init()
{
	fDoneRead = create_sem(0, "usb_serial:done_read");
	fDoneWrite = create_sem(0, "usb_serial:done_write");
	benaphore_init(&fReadLock, "usb_serial:read_lock");
	benaphore_init(&fWriteLock, "usb_serial:write_lock");

	fReadBufferSize = fWriteBufferSize = ROUNDUP(DEF_BUFFER_SIZE, 16);
	fInterruptBufferSize = 16;

	size_t totalBuffers = fReadBufferSize + fWriteBufferSize + fInterruptBufferSize;
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
SerialDevice::SetModes()
{
	struct termios tios;
	memcpy(&tios, &fTTY.t, sizeof(struct termios));
	uint16 newControl = fControlOut;
	TRACE_FUNCRES(trace_termios, &tios);

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
	uint32 baudIndex = tios.c_cflag & CBAUD;
	if (baudIndex > baudCount)
		baudIndex = baudCount - 1;

	usb_serial_line_coding lineCoding;
	lineCoding.speed = baudRates[baudIndex];
	lineCoding.stopbits = (tios.c_cflag & CSTOPB) ? LC_STOP_BIT_2 : LC_STOP_BIT_1;

	if (tios.c_cflag & PARENB) {
		lineCoding.parity = LC_PARITY_EVEN;
		if (tios.c_cflag & PARODD)
			lineCoding.parity = LC_PARITY_ODD;
	} else
		lineCoding.parity = LC_PARITY_NONE;

	lineCoding.databits = (tios.c_cflag & CS8) ? 8 : 7;

	if (lineCoding.speed == 0) {
		newControl &= 0xfffffffe;
		lineCoding.speed = fLineCoding.speed;
	} else
		newControl = CLS_LINE_DTR;

	if (fControlOut != newControl) {
		fControlOut = newControl;
		TRACE("newctrl send to modem: 0x%08x\n", newControl);
		SetControlLineState(newControl);
	}

	if (memcmp(&lineCoding, &fLineCoding, sizeof(usb_serial_line_coding)) != 0) {
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
SerialDevice::Service(struct tty *ptty, struct ddrover *ddr, uint flags)
{
	if (&fTTY != ptty)
		return false;

	if (flags <= TTYGETSIGNALS) {
		switch (flags) {
			case TTYENABLE:
				TRACE("TTYENABLE\n");
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDCD, false);
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, true);
				fControlOut = CLS_LINE_DTR | CLS_LINE_RTS;
				SetControlLineState(fControlOut);
				break;

			case TTYDISABLE:
				TRACE("TTYDISABLE\n");
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDCD, false);
				fControlOut = 0x0;
				SetControlLineState(fControlOut);
				break;

			case TTYISTOP:
				TRACE("TTYISTOP\n");
				fInputStopped = true;
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, false);
				break;

			case TTYIRESUME:
				TRACE("TTYIRESUME\n");
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, true);
				fInputStopped = false;
				break;

			case TTYGETSIGNALS:
				TRACE("TTYGETSIGNALS\n");
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDCD, true);
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, true);
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDSR, false);
				gTTYModule->ttyhwsignal(ptty, ddr, TTYHWRI, false);
				break;

			case TTYSETMODES:
				TRACE("TTYSETMODES\n");
				SetModes();
				break;

			case TTYOSTART:
			case TTYOSYNC:
			case TTYSETBREAK:
			case TTYCLRBREAK:
			case TTYSETDTR:
			case TTYCLRDTR:
				TRACE("TTY other\n");
				break;
		}

		return true;
	}

	return false;
}


status_t
SerialDevice::Open(uint32 flags)
{
	gTTYModule->ttyinit(&fTTY, true);
	fTTYFile.tty = &fTTY;
	fTTYFile.flags = flags;
	ResetDevice();

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	gTTYModule->ddacquire(ddr, &gSerialDomain);
	status_t status = gTTYModule->ttyopen(&fTTYFile, ddr, usb_serial_service);
	gTTYModule->ddrdone(ddr);

	if (status < B_OK) {
		TRACE_ALWAYS("open: failed to open tty\n");
		return status;
	}

	fDeviceThread = spawn_kernel_thread(DeviceThread, "usb_serial device thread",
		B_NORMAL_PRIORITY, this);

	if (fDeviceThread < B_OK) {
		TRACE_ALWAYS("open: failed to spawn kernel thread\n");
		return fDeviceThread;
	}

	resume_thread(fDeviceThread);

	fControlOut = CLS_LINE_DTR | CLS_LINE_RTS;
	SetControlLineState(fControlOut);

	status = gUSBModule->queue_interrupt(fControlPipe, fInterruptBuffer,
		fInterruptBufferSize, InterruptCallbackFunction, this);
	if (status < B_OK)
		TRACE_ALWAYS("failed to queue initial interrupt\n");
	return status;
}


status_t
SerialDevice::Read(char *buffer, size_t *numBytes)
{
	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr) {
		*numBytes = 0;
		return B_NO_MEMORY;
	}

	status_t status = benaphore_lock(&fReadLock);
	if (status != B_OK) {
		TRACE_ALWAYS("read: failed to get read lock\n");
		*numBytes = 0;
		return status;
	}

	status = gTTYModule->ttyread(&fTTYFile, ddr, buffer, numBytes);
	gTTYModule->ddrdone(ddr);

	benaphore_unlock(&fReadLock);
	return status;
}


status_t
SerialDevice::Write(const char *buffer, size_t *numBytes)
{
	size_t bytesLeft = *numBytes;
	*numBytes = 0;

	status_t status = benaphore_lock(&fWriteLock);
	if (status != B_OK) {
		TRACE_ALWAYS("write: failed to get write lock\n");
		return status;
	}

	while (bytesLeft > 0) {
		size_t length = MIN(bytesLeft, fWriteBufferSize);
		OnWrite(buffer, &length);

		status = gUSBModule->queue_bulk(fWritePipe, fWriteBuffer,
			length, WriteCallbackFunction, this);
		if (status < B_OK) {
			TRACE_ALWAYS("write: queueing failed with status 0x%08x\n", status);
			break;
		}

		status = acquire_sem_etc(fDoneWrite, 1, B_CAN_INTERRUPT, 0);
		if (status < B_OK) {
			TRACE_ALWAYS("write: failed to get write done sem 0x%08x\n", status);
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

		buffer += fActualLengthWrite;
		*numBytes += fActualLengthWrite;
		bytesLeft -= fActualLengthWrite;
	}

	benaphore_unlock(&fWriteLock);
	return status;
}


status_t
SerialDevice::Control(uint32 op, void *arg, size_t length)
{
	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status_t status = gTTYModule->ttycontrol(&fTTYFile, ddr, op, arg, length);
	gTTYModule->ddrdone(ddr);
	return status;
}


status_t
SerialDevice::Select(uint8 event, uint32 ref, selectsync *sync)
{
	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status_t status = gTTYModule->ttyselect(&fTTYFile, ddr, event, ref, sync);
	gTTYModule->ddrdone(ddr);
	return status;
}


status_t
SerialDevice::DeSelect(uint8 event, selectsync *sync)
{
	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status_t status = gTTYModule->ttydeselect(&fTTYFile, ddr, event, sync);
	gTTYModule->ddrdone(ddr);
	return status;
}


status_t
SerialDevice::Close()
{
	OnClose();

	gUSBModule->cancel_queued_transfers(fReadPipe);
	gUSBModule->cancel_queued_transfers(fWritePipe);
	gUSBModule->cancel_queued_transfers(fControlPipe);

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status_t status = gTTYModule->ttyclose(&fTTYFile, ddr);
	gTTYModule->ddrdone(ddr);
	return status;
}


status_t
SerialDevice::Free()
{
	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status_t status = gTTYModule->ttyfree(&fTTYFile, ddr);
	gTTYModule->ddrdone(ddr);
	return status;
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
SerialDevice::SetLineCoding(usb_serial_line_coding *coding)
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
SerialDevice::OnWrite(const char *buffer, size_t *numBytes)
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
			TRACE_ALWAYS("device thread: queueing failed with error: 0x%08x\n", status);
			break;
		}

		status = acquire_sem_etc(device->fDoneRead, 1, B_CAN_INTERRUPT, 0);
		if (status < B_OK) {
			TRACE_ALWAYS("device thread: failed to get read done sem 0x%08x\n", status);
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

		ddrover *ddr = gTTYModule->ddrstart(NULL);
		if (!ddr) {
			TRACE_ALWAYS("device thread: ddrstart problem\n");
			return B_NO_MEMORY;
		}

		while (device->fInputStopped)
			snooze(100);

		gTTYModule->ttyilock(&device->fTTY, ddr, true);
		for (size_t i = 0; i < readLength; i++)
			gTTYModule->ttyin(&device->fTTY, ddr, buffer[i]);

		gTTYModule->ttyilock(&device->fTTY, ddr, false);
		gTTYModule->ddrdone(ddr);
	}

	return B_OK;
}


void
SerialDevice::ReadCallbackFunction(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FUNCALLS("read callback: cookie: 0x%08x status: 0x%08x data: 0x%08x len: %lu\n",
		cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fDoneRead, 1, B_DO_NOT_RESCHEDULE);
}


void
SerialDevice::WriteCallbackFunction(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FUNCALLS("write callback: cookie: 0x%08x status: 0x%08x data: 0x%08x len: %lu\n",
		cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fDoneWrite, 1, B_DO_NOT_RESCHEDULE);
}


void
SerialDevice::InterruptCallbackFunction(void *cookie, int32 status,
	void *data, uint32 actualLength)
{
	TRACE_FUNCALLS("interrupt callback: cookie: 0x%08x status: 0x%08x data: 0x%08x len: %lu\n",
		cookie, status, data, actualLength);

	SerialDevice *device = (SerialDevice *)cookie;
	device->fActualLengthInterrupt = actualLength;
	device->fStatusInterrupt = status;

	// ToDo: maybe handle those somehow?

	if (status == B_OK) {
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
				case PRODUCT_PROLIFIC_RSAQ2: description = "PL2303 Serial adapter (IODATA USB-RSAQ2)"; break;
				case PRODUCT_IODATA_USBRSAQ: description = "I/O Data USB serial adapter USB-RSAQ1"; break;
				case PRODUCT_ATEN_UC232A: description = "Aten Serial adapter"; break;
				case PRODUCT_TDK_UHA6400: description = "TDK USB-PHS Adapter UHA6400"; break;
				case PRODUCT_RATOC_REXUSB60: description = "Ratoc USB serial adapter REX-USB60"; break;
				case PRODUCT_PROLIFIC_PL2303: description = "PL2303 Serial adapter (ATEN/IOGEAR UC232A)"; break;
				case PRODUCT_ELECOM_UCSGT: description = "Elecom UC-SGT"; break;
				case PRODUCT_SOURCENEXT_KEIKAI8: description = "SOURCENEXT KeikaiDenwa 8"; break;
				case PRODUCT_SOURCENEXT_KEIKAI8_CHG: description = "SOURCENEXT KeikaiDenwa 8 with charger"; break;
				case PRODUCT_HAL_IMR001: description = "HAL Corporation Crossam2+USB"; break;
			}

			if (!description)
				break;

			return new ProlificDevice(device, vendorID, productID, description);
		}

		case VENDOR_FTDI:
		{
			switch (productID) {
				case PRODUCT_FTDI_8U100AX: description = "FTDI 8U100AX serial converter"; break;
				case PRODUCT_FTDI_8U232AM: description = "FTDI 8U232AM serial converter"; break;
			}

			if (!description)
				break;

			return new FTDIDevice(device, vendorID, productID, description);
		}

		case VENDOR_PALM:
		case VENDOR_KLSI:
		{
			switch (productID) {
				case PRODUCT_PALM_CONNECT: description = "PalmConnect RS232"; break;
				case PRODUCT_KLSI_KL5KUSB105D: description = "KLSI KL5KUSB105D"; break;
			}

			if (!description)
				break;

			return new KLSIDevice(device, vendorID, productID, description);
		}
	}

	return new ACMDevice(device, vendorID, productID, "CDC ACM compatible device");
}
