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
#ifdef __HAIKU__
		fMasterTTY(NULL),
		fSlaveTTY(NULL),
		fTTYCookie(NULL),
#endif /* __HAIKU__ */
		fDeviceThread(-1),
		fStopDeviceThread(false)
{
#ifdef __BEOS__
	memset(&fTTYFile, 0, sizeof(ttyfile));
	memset(&fTTY, 0, sizeof(tty));
	memset(&fRover, 0, sizeof(ddrover));
#endif /* __BEOS__ */
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

	size_t totalBuffers = fReadBufferSize + fWriteBufferSize + fInterruptBufferSize;
	fBufferArea = create_area("usb_serial:buffers_area", (void **)&fReadBuffer,
		B_ANY_KERNEL_ADDRESS, ROUNDUP(totalBuffers, B_PAGE_SIZE), B_CONTIGUOUS,
		B_READ_AREA | B_WRITE_AREA);

	fWriteBuffer = fReadBuffer + fReadBufferSize;
	fInterruptBuffer = fWriteBuffer + fWriteBufferSize;

	// disable DLAB
	WriteReg8(LCR, 0);

	return B_OK;
}


#ifdef __BEOS__
void
SerialDevice::SetModes()
{
	struct termios tios;
	memcpy(&tios, &fTTY.t, sizeof(struct termios));
	SetModes(&tios);
}
#endif /* __BEOS__ */


void
SerialDevice::SetModes(struct termios *tios)
{
	//TRACE_FUNCRES(trace_termios, tios);
	spin(10000);
	uint32 baudIndex = tios->c_cflag & CBAUD;
	if (baudIndex > BLAST)
		baudIndex = BLAST;

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
		uint8 fcr = FCR_ENABLE | FCR_RX_RST | FCR_TX_RST | FCR_F_8;
		// enable fifo
		//fcr = 0;
		WriteReg8(FCR, fcr);

		// unmask the divisor latch regs
		WriteReg8(LCR, LCR_DLAB);
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


#ifdef __HAIKU__


bool
SerialDevice::Service(struct tty *tty, uint32 op, void *buffer, size_t length)
{
	uint8 msr;
	status_t err;

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
				WriteReg8(IER, 0);
				WriteReg8(MCR, 0);
			}

			msr = ReadReg8(MSR);

			SignalControlLineState(TTYHWDCD, msr & MSR_DCD);
			SignalControlLineState(TTYHWCTS, msr & MSR_CTS);

			if (enable) {
				// 
				WriteReg8(MCR, MCR_DTR | MCR_RTS | MCR_IRQ_EN /*| MCR_LOOP*//*XXXXXXX*/);
				// enable irqs
				WriteReg8(IER, IER_RLS | IER_MS | IER_RDA);
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
			uint8 bit = TTYSETDTR ? MCR_DTR : MCR_RTS;
			if (set)
				OrReg8(MCR, bit);
			else
				MaskReg8(MCR, bit);

			return true;
		}

		case TTYOSTART:
			TRACE("TTYOSTART\n");
			// enable irqs
			WriteReg8(IER, IER_RLS | IER_MS | IER_RDA | IER_THRE);
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
		default:
			return false;
	}

	return false;
}


#else /* __HAIKU__ */


bool
SerialDevice::Service(struct tty *ptty, struct ddrover *ddr, uint flags)
{
	uint8 msr;
	status_t err;

	if (&fTTY != ptty)
		return false;

	TRACE("%s(,,0x%08lx)\n", __FUNCTION__, flags);

	switch (flags) {
		case TTYENABLE:
			TRACE("TTYENABLE\n");

			SetModes();
			err = install_io_interrupt_handler(IRQ(), pc_serial_interrupt, this, 0);
			TRACE("installing irq handler for %d: %s\n", IRQ(), strerror(err));
			msr = ReadReg8(MSR);
			gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDCD, msr & MSR_DCD);
			gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, msr & MSR_CTS);
			// 
			WriteReg8(MCR, MCR_DTR | MCR_RTS | MCR_IRQ_EN /*| MCR_LOOP*//*XXXXXXX*/);
			// enable irqs
			WriteReg8(IER, IER_RLS | IER_MS | IER_RDA);
			//WriteReg8(IER, IER_RDA);
			break;

		case TTYDISABLE:
			TRACE("TTYDISABLE\n");
			// remove the handler
			remove_io_interrupt_handler(IRQ(), pc_serial_interrupt, this);
			// disable IRQ
			WriteReg8(IER, 0);
			WriteReg8(MCR, 0);
			msr = ReadReg8(MSR);
			gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDCD, msr & MSR_DCD);
			break;

		case TTYISTOP:
			TRACE("TTYISTOP\n");
			MaskReg8(MCR, MCR_RTS);
			//fInputStopped = true;
			//gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, false);
			break;

		case TTYIRESUME:
			TRACE("TTYIRESUME\n");
			OrReg8(MCR, MCR_RTS);
			//gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, true);
			//fInputStopped = false;
			break;

		case TTYGETSIGNALS:
			TRACE("TTYGETSIGNALS\n");
			msr = ReadReg8(MSR);
			gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDCD, msr & MSR_DCD);
			gTTYModule->ttyhwsignal(ptty, ddr, TTYHWCTS, msr & MSR_CTS);
			gTTYModule->ttyhwsignal(ptty, ddr, TTYHWDSR, msr & MSR_DSR);
			gTTYModule->ttyhwsignal(ptty, ddr, TTYHWRI, msr & MSR_RI);
			break;

		case TTYSETMODES:
			TRACE("TTYSETMODES\n");
			SetModes();
//WriteReg8(IER, IER_RLS | IER_MS | IER_RDA);
			break;

		case TTYOSTART:
			TRACE("TTYOSTART\n");
			// enable irqs
			WriteReg8(IER, IER_RLS | IER_MS | IER_RDA | IER_THRE);
			break;
		case TTYOSYNC:
			TRACE("TTYOSYNC\n");
			return (ReadReg8(LSR) & (LSR_THRE | LSR_TSRE)) == (LSR_THRE | LSR_TSRE);
			break;
		case TTYSETBREAK:
			TRACE("TTYSETBREAK\n");
			OrReg8(LCR, LCR_BREAK);
			break;
		case TTYCLRBREAK:
			TRACE("TTYCLRBREAK\n");
			MaskReg8(LCR, LCR_BREAK);
			break;
		case TTYSETDTR:
			TRACE("TTYSETDTR\n");
			OrReg8(MCR, MCR_DTR);
			break;
		case TTYCLRDTR:
			TRACE("TTYCLRDTR\n");
			MaskReg8(MCR, MCR_DTR);
			break;
		default:
			return false;
	}

	return false;
}
#endif /* __HAIKU__ */


int32
SerialDevice::InterruptHandler()
{
	int32 ret = B_UNHANDLED_INTERRUPT;
#ifdef __HAIKU__
	//XXX: what should we do here ? (certainly not use a mutex !)
#else /* __HAIKU__ */
	gTTYModule->ddrstart(&fRover);
	gTTYModule->ttyilock(&fTTY, &fRover, true);
#endif /* __HAIKU__ */

	uint8 iir, lsr, msr;

	while (((iir = ReadReg8(IIR)) & IIR_PENDING) == 0) { // 0 means yes
		int fifoavail = 1;
		
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
			for (int i = 0; i < fifoavail; i++) {
				int chr;
#ifdef __HAIKU__
				chr = 'H';//XXX: what should we do here ? (certainly not call tty_read() !)
#else /* __HAIKU__ */
				chr = gTTYModule->ttyout(&fTTY, &fRover);
#endif /* __HAIKU__ */
				if (chr < 0) {
					//WriteReg8(THB, (uint8)chr);
					break;
				}
				WriteReg8(THB, (uint8)chr);
			}
			break;
		case IIR_TO:
		case IIR_TO | IIR_RDA:
			// timeout: FALLTHROUGH
		case IIR_RDA:
			TRACE(("IIR_TO/RDA\n"));
			// while data is ready... get it
			while (ReadReg8(LSR) & LSR_DR)
#ifdef __HAIKU__
				ReadReg8(RBR);//XXX: what should we do here ? (certainly not call tty_write() !)
#else /* __HAIKU__ */
				gTTYModule->ttyin(&fTTY, &fRover, ReadReg8(RBR));
#endif /* __HAIKU__ */
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
	}


#ifdef __HAIKU__
	//XXX: what should we do here ? (certainly not use a mutex !)
#else /* __HAIKU__ */
	gTTYModule->ttyilock(&fTTY, &fRover, false);
	gTTYModule->ddrdone(&fRover);
#endif /* __HAIKU__ */
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

#ifdef __HAIKU__
	fMasterTTY = gTTYModule->tty_create(pc_serial_service, true);
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

#else /* __HAIKU__ */

	gTTYModule->ttyinit(&fTTY, false);
	fTTYFile.tty = &fTTY;
	fTTYFile.flags = flags;


	ResetDevice();

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	gTTYModule->ddacquire(ddr, &gSerialDomain);
	status = gTTYModule->ttyopen(&fTTYFile, ddr, pc_serial_service);
	gTTYModule->ddrdone(ddr);

#endif /* __HAIKU__ */

	if (status < B_OK) {
		TRACE_ALWAYS("open: failed to open tty\n");
		return status;
	}

#if 0
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

#ifdef __HAIKU__

	status = mutex_lock(&fReadLock);
	if (status != B_OK) {
		TRACE_ALWAYS("read: failed to get read lock\n");
		*numBytes = 0;
		return status;
	}

	status = gTTYModule->tty_read(fTTYCookie, buffer, numBytes);

	mutex_unlock(&fReadLock);

#else /* __HAIKU__ */

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr) {
		*numBytes = 0;
		return B_NO_MEMORY;
	}

	status = gTTYModule->ttyread(&fTTYFile, ddr, buffer, numBytes);
	gTTYModule->ddrdone(ddr);

#endif /* __HAIKU__ */

	return status;
}


status_t
SerialDevice::Write(const char *buffer, size_t *numBytes)
{
	//size_t bytesLeft = *numBytes;
	//*numBytes = 0;

	status_t status = EINVAL;

#ifdef __HAIKU__

	status = mutex_lock(&fWriteLock);
	if (status != B_OK) {
		TRACE_ALWAYS("write: failed to get write lock\n");
		return status;
	}

	//XXX: WTF tty_write() is not for write() hook ?
	//status = gTTYModule->tty_write(fTTYCookie, buffer, numBytes);
	mutex_unlock(&fWriteLock);

#else /* __HAIKU__ */

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr) {
		*numBytes = 0;
		return B_ERROR;
	}

	status = gTTYModule->ttywrite(&fTTYFile, ddr, buffer, numBytes);
	gTTYModule->ddrdone(ddr);

#endif /* __HAIKU__ */

#if 0
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

		buffer += length;
		*numBytes += length;
		bytesLeft -= length;
	}

	mutex_unlock(&fWriteLock);
#endif
	return status;
}


status_t
SerialDevice::Control(uint32 op, void *arg, size_t length)
{
	status_t status = B_OK;

	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

#ifdef __HAIKU__

	status = gTTYModule->tty_control(fTTYCookie, op, arg, length);

#else /* __HAIKU__ */

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status = gTTYModule->ttycontrol(&fTTYFile, ddr, op, arg, length);
	gTTYModule->ddrdone(ddr);

#endif /* __HAIKU__ */

	return status;
}


status_t
SerialDevice::Select(uint8 event, uint32 ref, selectsync *sync)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

#ifdef __HAIKU__

	return gTTYModule->tty_select(fTTYCookie, event, ref, sync);

#else /* __HAIKU__ */

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status_t status = gTTYModule->ttyselect(&fTTYFile, ddr, event, ref, sync);
	gTTYModule->ddrdone(ddr);

	return status;

#endif /* __HAIKU__ */
}


status_t
SerialDevice::DeSelect(uint8 event, selectsync *sync)
{
	if (fDeviceRemoved)
		return B_DEV_NOT_READY;

#ifdef __HAIKU__

	return gTTYModule->tty_deselect(fTTYCookie, event, sync);

#else /* __HAIKU__ */

	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status_t status = gTTYModule->ttydeselect(&fTTYFile, ddr, event, sync);
	gTTYModule->ddrdone(ddr);

	return status;

#endif /* __HAIKU__ */
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

#ifdef __HAIKU__
	gTTYModule->tty_destroy_cookie(fTTYCookie);
#else /* __HAIKU__ */
	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status = gTTYModule->ttyclose(&fTTYFile, ddr);
	gTTYModule->ddrdone(ddr);

#endif /* __HAIKU__ */

	fDeviceOpen = false;
	return status;
}


status_t
SerialDevice::Free()
{
	status_t status = B_OK;
#ifdef __HAIKU__
	gTTYModule->tty_destroy(fMasterTTY);
	gTTYModule->tty_destroy(fSlaveTTY);
#else /* __HAIKU__ */
	struct ddrover *ddr = gTTYModule->ddrstart(NULL);
	if (!ddr)
		return B_NO_MEMORY;

	status = gTTYModule->ttyfree(&fTTYFile, ddr);
	gTTYModule->ddrdone(ddr);
#endif /* __HAIKU__ */
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

	//int32 result = B_OK;
	//wait_for_thread(fDeviceThread, &result);
	fDeviceThread = -1;

	mutex_lock(&fWriteLock);
	mutex_unlock(&fWriteLock);
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
#ifdef __HAIKU__
	gTTYModule->tty_hardware_signal(fTTYCookie, line, enable);
#else
	// XXX: only works for interrupt handler, Service func must pass the ddrover
	gTTYModule->ttyhwsignal(&fTTY, &fRover, line, enable);
#endif
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
