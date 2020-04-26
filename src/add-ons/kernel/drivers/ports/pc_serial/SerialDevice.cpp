/*
 * Copyright 2009-2010, François Revol, <revol@free.fr>.
 * Sponsored by TuneTracker Systems.
 * Based on the Haiku usb_serial driver which is:
 *
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#include "SerialDevice.h"
#include "UART.h"

#include <sys/ioctl.h>

SerialDevice::SerialDevice(const struct serial_support_descriptor *device,
	uint32 ioBase, uint32 irq, const SerialDevice *master)
	:	/*fSupportDescriptor(device->descriptor),
		fDevice(device),
		fDescription(device->descriptor->name),*/
		fSupportDescriptor(device),
		fDevice(NULL),
		fDescription(device->name),
		//
		fDeviceOpen(false),
		fDeviceRemoved(false),
		fBus(device->bus),
		fIOBase(ioBase),
		fIRQ(irq),
		fMaster(master),
		fCachedIER(0x0),
		fCachedIIR(0x1),
		fPendingDPC(0),
		fReadBufferAvail(0),
		fReadBufferIn(0),
		fReadBufferOut(0),
		fReadBufferSem(-1),
		fWriteBufferAvail(0),
		fWriteBufferIn(0),
		fWriteBufferOut(0),
		fWriteBufferSem(-1),
		fDoneRead(-1),
		fDoneWrite(-1),
		fControlOut(0),
		fInputStopped(false),
		fMasterTTY(NULL),
		fSlaveTTY(NULL),
		fSystemTTYCookie(NULL),
		fDeviceTTYCookie(NULL),
		fDeviceThread(-1),
		fStopDeviceThread(false)
{
	memset(&fTTYConfig, 0, sizeof(termios));
	fTTYConfig.c_cflag = B9600 | CS8 | CREAD;
	memset(fReadBuffer, 'z', DEF_BUFFER_SIZE);
	memset(fWriteBuffer, 'z', DEF_BUFFER_SIZE);
}


SerialDevice::~SerialDevice()
{
	Removed();

	if (fDoneWrite >= B_OK)
		delete_sem(fDoneWrite);
	if (fReadBufferSem >= B_OK)
		delete_sem(fReadBufferSem);
	if (fWriteBufferSem >= B_OK)
		delete_sem(fWriteBufferSem);
}


bool
SerialDevice::Probe()
{
	uint8 msr;
	msr = ReadReg8(MSR);
	// just in case read twice to make sure the "delta" bits are 0
	msr = ReadReg8(MSR);
	// this should be enough to probe for the device for now
	// we might want to check the scratch reg, and try identifying
	// the model as in:
	// http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming#Software_Identification_of_the_UART
	return (msr != 0xff);
}


status_t
SerialDevice::Init()
{
	fDoneWrite = create_sem(0, "pc_serial:done_write");
	fReadBufferSem = create_sem(0, "pc_serial:done_read");
	fWriteBufferSem = create_sem(0, "pc_serial:done_write");

	// disable IRQ
	fCachedIER = 0;
	WriteReg8(IER, fCachedIER);

	// disable DLAB
	WriteReg8(LCR, 0);

	return B_OK;
}


void
SerialDevice::SetModes(struct termios *tios)
{
	//TRACE_FUNCRES(trace_termios, tios);
	spin(10000);
	uint32 baudIndex = tios->c_cflag & CBAUD;
	if (baudIndex > BLAST)
		baudIndex = BLAST;

	// update our master config in full
	memcpy(&fTTYConfig, tios, sizeof(termios));
	fTTYConfig.c_cflag &= ~CBAUD;
	fTTYConfig.c_cflag |= baudIndex;

	// only apply the relevant parts to the device side
	termios config;
	memset(&config, 0, sizeof(termios));
	config.c_cflag = tios->c_cflag;
	config.c_cflag &= ~CBAUD;
	config.c_cflag |= baudIndex;

	// update the termios of the device side
	gTTYModule->tty_control(fDeviceTTYCookie, TCSETA, &config, sizeof(termios));

	uint8 lcr = 0;
	uint16 divisor = SupportDescriptor()->bauds[baudIndex];

	switch (tios->c_cflag & CSIZE) {
#if	CS5 != CS7
	// in case someday...
	case CS5:
		lcr |= LCR_5BIT;
		break;
	case CS6:
		lcr |= LCR_6BIT;
		break;
#endif
	case CS7:
		lcr |= LCR_7BIT;
		break;
	case CS8:
	default:
		lcr |= LCR_8BIT;
		break;
	}

	if (tios->c_cflag & CSTOPB)
		lcr |= LCR_2STOP;
	if (tios->c_cflag & PARENB)
		lcr |= LCR_P_EN;
	if ((tios->c_cflag & PARODD) == 0)
		lcr |= LCR_P_EVEN;

	if (baudIndex == B0) {
		// disable
		MaskReg8(MCR, MCR_DTR);
	} else {
		// set FCR now, 
		// 16650 and later chips have another reg at 2 when DLAB=1
		uint8 fcr = FCR_ENABLE | FCR_RX_RST | FCR_TX_RST | FCR_F_8 | FCR_F64EN;
		// enable fifo
		//fcr = 0;
		WriteReg8(FCR, fcr);

		// unmask the divisor latch regs
		WriteReg8(LCR, lcr | LCR_DLAB);
		// set divisor
		WriteReg8(DLLB, divisor & 0x00ff);
		WriteReg8(DLHB, divisor >> 8);
		//WriteReg8(2,0);

	}
	// set control lines, and disable divisor latch reg
	WriteReg8(LCR, lcr);


#if 0
	uint16 newControl = fControlOut;

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

	usb_serial_line_coding lineCoding;
	lineCoding.speed = baudRates[baudIndex];
	lineCoding.stopbits = (tios->c_cflag & CSTOPB) ? LC_STOP_BIT_2 : LC_STOP_BIT_1;

	if (tios->c_cflag & PARENB) {
		lineCoding.parity = LC_PARITY_EVEN;
		if (tios->c_cflag & PARODD)
			lineCoding.parity = LC_PARITY_ODD;
	} else
		lineCoding.parity = LC_PARITY_NONE;

	lineCoding.databits = (tios->c_cflag & CS8) ? 8 : 7;

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
#endif
}


bool
SerialDevice::Service(struct tty *tty, uint32 op, void *buffer, size_t length)
{
	uint8 msr;
	status_t err;

	TRACE("%s(,0x%08lx,,%d)\n", __FUNCTION__, op, length);
	if (tty != fMasterTTY)
		return false;

	TRACE("%s(,0x%08lx,,%d)\n", __FUNCTION__, op, length);

	switch (op) {
		case TTYENABLE:
		{
			bool enable = *(bool *)buffer;
			TRACE("TTYENABLE: %sable\n", enable ? "en" : "dis");

			if (enable) {
				//XXX:must call SetModes();
				err = install_io_interrupt_handler(IRQ(), pc_serial_interrupt, this, 0);
				TRACE("installing irq handler for %d: %s\n", IRQ(), strerror(err));
			} else {
				// remove the handler
				remove_io_interrupt_handler(IRQ(), pc_serial_interrupt, this);
				// disable IRQ
				fCachedIER = 0;
				WriteReg8(IER, fCachedIER);
				WriteReg8(MCR, 0);
			}

			msr = ReadReg8(MSR);

			SignalControlLineState(TTYHWDCD, msr & MSR_DCD);
			SignalControlLineState(TTYHWCTS, msr & MSR_CTS);

			if (enable) {
				// 
				WriteReg8(MCR, MCR_DTR | MCR_RTS | MCR_IRQ_EN /*| MCR_LOOP*//*XXXXXXX*/);
				// enable irqs
				fCachedIER = IER_RLS | IER_MS | IER_RDA;
				WriteReg8(IER, fCachedIER);
				//WriteReg8(IER, IER_RDA);
			}

			return true;
		}

		case TTYISTOP:
			fInputStopped = *(bool *)buffer;
			TRACE("TTYISTOP: %sstopped\n", fInputStopped ? "" : "not ");

			if (fInputStopped)
				MaskReg8(MCR, MCR_RTS);
			else
				OrReg8(MCR, MCR_RTS);

			//gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, false);
			//SignalControlLineState(TTYHWCTS, !fInputStopped);
			//msr = ReadReg8(MSR);
			//SignalControlLineState(TTYHWCTS, msr & MSR_CTS);
			return true;

		case TTYGETSIGNALS:
			TRACE("TTYGETSIGNALS\n");
			msr = ReadReg8(MSR);
			SignalControlLineState(TTYHWDCD, msr & MSR_DCD);
			SignalControlLineState(TTYHWCTS, msr & MSR_CTS);
			SignalControlLineState(TTYHWDSR, msr & MSR_DSR);
			SignalControlLineState(TTYHWRI, msr & MSR_RI);
			return true;

		case TTYSETMODES:
			TRACE("TTYSETMODES\n");
			SetModes((struct termios *)buffer);
//WriteReg8(IER, IER_RLS | IER_MS | IER_RDA);
			return true;

		case TTYSETDTR:
		case TTYSETRTS:
		{
			bool set = *(bool *)buffer;
			uint8 bit = op == TTYSETDTR ? MCR_DTR : MCR_RTS;
			if (set)
				OrReg8(MCR, bit);
			else
				MaskReg8(MCR, bit);

			return true;
		}

		case TTYOSTART:
			TRACE("TTYOSTART\n");
			// enable irqs
			fCachedIER |= IER_THRE;
			// XXX: toggle the bit to make VirtualBox happy !?
			WriteReg8(IER, fCachedIER & ~IER_THRE);
			WriteReg8(IER, fCachedIER);
			return true;
		case TTYOSYNC:
			TRACE("TTYOSYNC\n");
			return (ReadReg8(LSR) & (LSR_THRE | LSR_TSRE)) == (LSR_THRE | LSR_TSRE);
			return true;
		case TTYSETBREAK:
		{
			bool set = *(bool *)buffer;
			if (set)
				OrReg8(MCR, LCR_BREAK);
			else
				MaskReg8(MCR, LCR_BREAK);

			return true;
		}
		case TTYFLUSH:
		{
			int directions = *(int*)buffer;
			uint8 mask = 0;
			if (directions & TCIFLUSH)
				mask |= FCR_RX_RST;
			if (directions & TCOFLUSH)
				mask |= FCR_TX_RST;
			// These bits clear themselves when flushing is complete
			OrReg8(FCR, mask);

			return true;
		}
		default:
			return false;
	}

	return false;
}


bool
SerialDevice::IsInterruptPending()
{
	TRACE(("IsInterruptPending()\n"));

	// because reading the IIR acknowledges some IRQ conditions,
	// the next time we'll read we'll miss the IRQ condition
	// so we just cache the value for the real handler
	fCachedIIR = ReadReg8(IIR);

	bool pending = (fCachedIIR & IIR_PENDING) == 0;

	if (pending) {
		// temporarily mask the IRQ
		// else VirtualBox triggers one per every written byte it seems
		// not sure it's required on real hardware
		WriteReg8(IER, fCachedIER & ~(IER_RLS | IER_MS | IER_RDA | IER_THRE));

		atomic_add(&fPendingDPC, 1);
	}

	return pending; // 0 means yes
}


int32
SerialDevice::InterruptHandler()
{
	int32 ret = B_UNHANDLED_INTERRUPT;
	//XXX: what should we do here ? (certainly not use a mutex !)

	uint8 iir, lsr, msr;
	uint8 buffer[64];
	int tries = 8; // avoid busy looping
	TRACE(("InterruptHandler()\n"));

	// start with the first (cached) irq condition
	iir = fCachedIIR;
	while ((iir & IIR_PENDING) == 0) { // 0 means yes
		status_t status;
		size_t bytesLeft;
		size_t readable = 0;
		size_t fifoavail = 1;
		size_t i;
		
		//DEBUG
//		for (int count = 0; ReadReg8(LSR) & LSR_DR; count++)
//			gTTYModule->ttyin(&fTTY, &fRover, ReadReg8(RBR));

		switch (iir & (IIR_IMASK | IIR_TO)) {
		case IIR_THRE:
			TRACE(("IIR_THRE\n"));
			// check how much fifo we can use
			//XXX: move to Init() ?
			if ((iir & IIR_FMASK) == IIR_FMASK)
				fifoavail = 16;
			if (iir & IIR_F64EN)
				fifoavail = 64;
			// we're not open... just discard the data
			if (!IsOpen())
				break;
			gTTYModule->tty_control(fDeviceTTYCookie, FIONREAD, &readable,
				sizeof(readable));
			TRACE("%s: FIONREAD: %d\n", __FUNCTION__, readable);

			if (readable == 0) {
				release_sem_etc(fDoneWrite, 1, B_DO_NOT_RESCHEDULE);
				// mask it until there's data again
				fCachedIER &= ~IER_THRE;
				break;
			}

			bytesLeft = MIN(fifoavail, sizeof(buffer));
			bytesLeft = MIN(bytesLeft, readable);
			TRACE("%s: left %d\n", __FUNCTION__, bytesLeft);
			status = gTTYModule->tty_read(fDeviceTTYCookie, buffer, &bytesLeft);
			TRACE("%s: tty_read: %d\n", __FUNCTION__, bytesLeft);
			if (status != B_OK) {
				dprintf(DRIVER_NAME ": irq: tty_read: %s\n", strerror(status));
				break;
			}

			for (i = 0; i < bytesLeft; i++) {
				WriteReg8(THB, buffer[i]);
			}

			break;
		case IIR_TO:
		case IIR_TO | IIR_RDA:
			// timeout: FALLTHROUGH
		case IIR_RDA:
			TRACE(("IIR_TO/RDA\n"));
			// while data is ready... and we have room for it, get it
			bytesLeft = sizeof(buffer);
			for (i = 0; i < bytesLeft && (ReadReg8(LSR) & LSR_DR); i++) {
				buffer[i] = ReadReg8(RBR);
			}
			// we're not open... just discard the data
			if (!IsOpen())
				break;
			// we shouldn't block here but it's < 256 bytes anyway
			status = gTTYModule->tty_write(fDeviceTTYCookie, buffer, &i);
			if (status != B_OK) {
				dprintf(DRIVER_NAME ": irq: tty_write: %s\n", strerror(status));
				break;
			}
			break;
		case IIR_RLS:
			TRACE(("IIR_RLS\n"));
			// ack
			lsr = ReadReg8(LSR);
			//XXX: handle this somehow
			break;
		case IIR_MS:
			TRACE(("IIR_MS\n"));
			// modem signals changed
			msr = ReadReg8(MSR);
			if (!IsOpen())
				break;
			if (msr & MSR_DDCD)
				SignalControlLineState(TTYHWDCD, msr & MSR_DCD);
			if (msr & MSR_DCTS)
				SignalControlLineState(TTYHWCTS, msr & MSR_CTS);
			if (msr & MSR_DDSR)
				SignalControlLineState(TTYHWDSR, msr & MSR_DSR);
			if (msr & MSR_TERI)
				SignalControlLineState(TTYHWRI, msr & MSR_RI);
			break;
		default:
			TRACE(("IIR_?\n"));
			// something happened
			break;
		}
		ret = B_HANDLED_INTERRUPT;
		TRACE(("IRQ:h\n"));

		// enough for now
		if (tries-- == 0)
			break;

		// check the next IRQ condition
		iir = ReadReg8(IIR);
	}

	atomic_add(&fPendingDPC, -1);

	// unmask IRQ
	WriteReg8(IER, fCachedIER);

	TRACE_FUNCRET("< IRQ:%d\n", ret);
	return ret;
}


status_t
SerialDevice::Open(uint32 flags)
{
	status_t status = B_OK;

	if (fDeviceOpen)
		return B_BUSY;

	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	fMasterTTY = gTTYModule->tty_create(pc_serial_service, true);
	if (fMasterTTY == NULL) {
		TRACE_ALWAYS("open: failed to init master tty\n");
		return B_NO_MEMORY;
	}

	fSlaveTTY = gTTYModule->tty_create(pc_serial_service, false);
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

	//XXX: we shouldn't have to do this!
	bool en = true;
	Service(fMasterTTY, TTYENABLE, &en, sizeof(en));

	if (status < B_OK) {
		TRACE_ALWAYS("open: failed to open tty\n");
		return status;
	}

	// set our config (will propagate to the slave config as well in SetModes()
	gTTYModule->tty_control(fSystemTTYCookie, TCSETA, &fTTYConfig,
		sizeof(termios));

#if 0
	fDeviceThread = spawn_kernel_thread(_DeviceThread, "usb_serial device thread",
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

#endif
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

	status_t status;

	status = gTTYModule->tty_read(fSystemTTYCookie, buffer, numBytes);

	return status;
}


status_t
SerialDevice::Write(const char *buffer, size_t *numBytes)
{
	TRACE("%s(,&%d)\n", __FUNCTION__, *numBytes);
	if (fDeviceRemoved) {
		*numBytes = 0;
		return B_DEV_NOT_READY;
	}

	status_t status;
	size_t bytesLeft = *numBytes;
	*numBytes = 0;

	while (bytesLeft > 0) {
		size_t length = MIN(bytesLeft, 256);
			// TODO: This is an ugly hack; We use a small buffer size so that
			// we don't overrun the tty line buffer and cause it to block. While
			// that isn't a problem, we shouldn't just hardcode the value here.

		TRACE("%s: tty_write(,&%d)\n", __FUNCTION__, length);
		status = gTTYModule->tty_write(fSystemTTYCookie, buffer,
			&length);
		if (status != B_OK) {
			TRACE_ALWAYS("failed to write to tty: %s\n", strerror(status));
			return status;
		}

		buffer += length;
		*numBytes += length;
		bytesLeft -= length;

		// XXX: WTF: this ought to be done by the tty module calling service_func!
		// enable irqs
		Service(fMasterTTY, TTYOSTART, NULL, 0);
	}

	status = acquire_sem_etc(fDoneWrite, 1, B_CAN_INTERRUPT, 0);
	if (status != B_OK) {
		TRACE_ALWAYS("write: failed to get write done sem "
				"0x%08x\n", status);
		return status;
	}


	if (*numBytes > 0)
		return B_OK;

	return B_ERROR;
}


status_t
SerialDevice::Control(uint32 op, void *arg, size_t length)
{
	status_t status = B_OK;

	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

	status = gTTYModule->tty_control(fSystemTTYCookie, op, arg, length);

	return status;
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
	status_t status = B_OK;

	OnClose();

	if (!fDeviceRemoved) {
#if 0
		gUSBModule->cancel_queued_transfers(fReadPipe);
		gUSBModule->cancel_queued_transfers(fWritePipe);
		gUSBModule->cancel_queued_transfers(fControlPipe);
#endif
	}

	fDeviceOpen = false;

	gTTYModule->tty_close_cookie(fSystemTTYCookie);
	gTTYModule->tty_close_cookie(fDeviceTTYCookie);

	//XXX: we shouldn't have to do this!
	bool en = false;
	Service(fMasterTTY, TTYENABLE, &en, sizeof(en));

	return status;
}


status_t
SerialDevice::Free()
{
	status_t status = B_OK;

	// wait until currently executing DPC is done. In case another one
	// is run beyond this point it will just bail out on !IsOpen().
	//while (atomic_get(&fPendingDPC))
	//	snooze(1000);

	gTTYModule->tty_destroy_cookie(fSystemTTYCookie);
	gTTYModule->tty_destroy_cookie(fDeviceTTYCookie);
	fSystemTTYCookie = fDeviceTTYCookie = NULL;

	gTTYModule->tty_destroy(fMasterTTY);
	gTTYModule->tty_destroy(fSlaveTTY);
	fMasterTTY = fSlaveTTY = NULL;

	return status;
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
#if 0
	gUSBModule->cancel_queued_transfers(fReadPipe);
	gUSBModule->cancel_queued_transfers(fWritePipe);
	gUSBModule->cancel_queued_transfers(fControlPipe);
#endif

	int32 result = B_OK;
	wait_for_thread(fDeviceThread, &result);
	fDeviceThread = -1;
}


status_t
SerialDevice::AddDevice(const serial_config_descriptor *config)
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


#if 0
status_t
SerialDevice::SetLineCoding(usb_serial_line_coding *coding)
{
	// default implementation - does nothing
	return B_OK;
}
#endif

status_t
SerialDevice::SignalControlLineState(int line, bool enable)
{
	gTTYModule->tty_hardware_signal(fSystemTTYCookie, line, enable);

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
SerialDevice::_DeviceThread(void *data)
{
#if 0
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

#endif
	return B_OK;
}


status_t
SerialDevice::_WriteToDevice()
{
	char *buffer = &fWriteBuffer[fWriteBufferIn];
	size_t bytesLeft = DEF_BUFFER_SIZE - atomic_get(&fWriteBufferAvail);
	bytesLeft = MIN(bytesLeft, DEF_BUFFER_SIZE - fWriteBufferIn);
	TRACE("%s: in %d left %d\n", __FUNCTION__, fWriteBufferIn, bytesLeft);
	status_t status = gTTYModule->tty_read(fDeviceTTYCookie, buffer,
		&bytesLeft);
	TRACE("%s: tty_read: %d\n", __FUNCTION__, bytesLeft);
	if (status != B_OK) {
		TRACE_ALWAYS("write to device: failed to read from TTY: %s\n",
			strerror(status));
		return status;
	}
	fWriteBufferIn += bytesLeft;
	fWriteBufferIn %= DEF_BUFFER_SIZE;
	atomic_add(&fWriteBufferAvail, bytesLeft);

	// XXX: WTF: this ought to be done by the tty module calling service_func!
	// enable irqs
	Service(fMasterTTY, TTYOSTART, NULL, 0);

	status = acquire_sem_etc(fWriteBufferSem, 1, B_CAN_INTERRUPT, 0);
	if (status != B_OK) {
		TRACE_ALWAYS("write to device: failed to acquire sem: %s\n",
			strerror(status));
		return status;
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

	if (status == B_OK && !device->fDeviceRemoved) {
#if 0
		status = gUSBModule->queue_interrupt(device->fControlPipe,
			device->fInterruptBuffer, device->fInterruptBufferSize,
			device->InterruptCallbackFunction, device);
#endif
	}
}



#if 0
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

			return new(std::nothrow) ProlificDevice(device, vendorID, productID, description);
		}

		case VENDOR_FTDI:
		{
			switch (productID) {
				case PRODUCT_FTDI_8U100AX: description = "FTDI 8U100AX serial converter"; break;
				case PRODUCT_FTDI_8U232AM: description = "FTDI 8U232AM serial converter"; break;
			}

			if (!description)
				break;

			return new(std::nothrow) FTDIDevice(device, vendorID, productID, description);
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

			return new(std::nothrow) KLSIDevice(device, vendorID, productID, description);
		}
	}

	return new(std::nothrow) ACMDevice(device, vendorID, productID, "CDC ACM compatible device");
}
#endif


uint8
SerialDevice::ReadReg8(int reg)
{
	uint8 ret;
	switch (fBus) {
	case B_ISA_BUS:
		ret = gISAModule->read_io_8(IOBase() + reg);
		break;
	case B_PCI_BUS:
		ret = gPCIModule->read_io_8(IOBase() + reg);
		break;
	default:
		TRACE_ALWAYS("%s: unknown bus!\n", __FUNCTION__);
		ret = 0;
	//XXX:pcmcia ?
	}
	TRACE/*_ALWAYS*/("RR8(%d) = %d [%02x]\n", reg, ret, ret);
	//spin(1000);
	return ret;
}

void
SerialDevice::WriteReg8(int reg, uint8 value)
{
//	TRACE_ALWAYS("WR8(0x%04x+%d, %d [0x%x])\n", IOBase(), reg, value, value);
	TRACE/*_ALWAYS*/("WR8(%d, %d [0x%x])\n", reg, value, value);
	switch (fBus) {
	case B_ISA_BUS:
		gISAModule->write_io_8(IOBase() + reg, value);
		break;
	case B_PCI_BUS:
		gPCIModule->write_io_8(IOBase() + reg, value);
		break;
	default:
		TRACE_ALWAYS("%s: unknown bus!\n", __FUNCTION__);
	//XXX:pcmcia ?
	}
	//spin(10000);
}


void
SerialDevice::OrReg8(int reg, uint8 value)
{
	WriteReg8(reg, ReadReg8(reg) | value);
}


void
SerialDevice::AndReg8(int reg, uint8 value)
{
	WriteReg8(reg, ReadReg8(reg) & value);
}


void
SerialDevice::MaskReg8(int reg, uint8 value)
{
	WriteReg8(reg, ReadReg8(reg) & ~value);
}
