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
#ifndef _SERIAL_DEVICE_H_
#define _SERIAL_DEVICE_H_

#include "Driver.h"

class SerialDevice {
public:
/*								SerialDevice(struct serial_config_descriptor 
									*device, uint32 ioBase, uint32 irq, SerialDevice *master=NULL);*/
								SerialDevice(const struct serial_support_descriptor 
									*device, uint32 ioBase, uint32 irq, const SerialDevice *master=NULL);
virtual							~SerialDevice();

		bool					Probe();

static	SerialDevice *			MakeDevice(struct serial_config_descriptor 
									*device);

		status_t				Init();

		const struct serial_support_descriptor	*SupportDescriptor() const 
									{ return fSupportDescriptor; };
		struct serial_config_descriptor	*ConfigDescriptor() const 
									{ return fDevice; };
		//uint16					ProductID() const { return fProductID; };
		//uint16					VendorID() const { return fVendorID; };
		const char *			Description() const { return fDescription; };

		const SerialDevice *	Master() const { return fMaster ? fMaster : this; };

		char *					ReadBuffer() { return fReadBuffer; };

		char *					WriteBuffer() { return fWriteBuffer; };

		void					SetModes(struct termios *tios);

		bool					Service(struct tty *tty, uint32 op,
									void *buffer, size_t length);

		bool					IsInterruptPending();
		int32					InterruptHandler();

		status_t				Open(uint32 flags);
		status_t				Read(char *buffer, size_t *numBytes);
		status_t				Write(const char *buffer, size_t *numBytes);
		status_t				Control(uint32 op, void *arg, size_t length);
		status_t				Select(uint8 event, uint32 ref, selectsync *sync);
		status_t				DeSelect(uint8 event, selectsync *sync);
		status_t				Close();
		status_t				Free();

		bool					IsOpen() { return fDeviceOpen; };
		void					Removed();
		bool					IsRemoved() { return fDeviceRemoved; };

		/* virtual interface to be overriden as necessary */
virtual	status_t				AddDevice(const struct serial_config_descriptor *device);

virtual	status_t				ResetDevice();

//virtual	status_t				SetLineCoding(usb_serial_line_coding *coding);
//virtual	status_t				SetControlLineState(uint16 state);
virtual	status_t				SignalControlLineState(int line, bool enable);

virtual	void					OnRead(char **buffer, size_t *numBytes);
virtual	void					OnWrite(const char *buffer, size_t *numBytes,
									size_t *packetBytes);
virtual	void					OnClose();

		uint32					IOBase() const { return fIOBase; };
		uint32					IRQ() const { return fIRQ; };

private:
static	int32					_DeviceThread(void *data);
		status_t				_WriteToDevice();

static	void					ReadCallbackFunction(void *cookie,
									int32 status, void *data,
									uint32 actualLength);
static	void					WriteCallbackFunction(void *cookie,
									int32 status, void *data,
									uint32 actualLength);
static	void					InterruptCallbackFunction(void *cookie,
									int32 status, void *data,
									uint32 actualLength);

		uint8					ReadReg8(int reg);
		void					WriteReg8(int reg, uint8 value);
		void					OrReg8(int reg, uint8 value);
		void					AndReg8(int reg, uint8 value);
		void					MaskReg8(int reg, uint8 value);

		const struct serial_support_descriptor	*fSupportDescriptor;
		struct serial_config_descriptor		*fDevice;		// USB device handle
		const char *			fDescription;	// informational description
		bool					fDeviceOpen;
		bool					fDeviceRemoved;

		bus_type				fBus;
		uint32					fIOBase;
		uint32					fIRQ;
		const SerialDevice *	fMaster;

		/* line coding */
		//usb_serial_line_coding	fLineCoding;

		/* deferred interrupt */
		uint8					fCachedIER;	// last value written to IER
		uint8					fCachedIIR;	// cached IRQ condition
		int32					fPendingDPC; // some IRQ still

		/* data buffers */
		char					fReadBuffer[DEF_BUFFER_SIZE];
		int32					fReadBufferAvail;
		uint32					fReadBufferIn;
		uint32					fReadBufferOut;
		sem_id					fReadBufferSem;
		char					fWriteBuffer[DEF_BUFFER_SIZE];
		int32					fWriteBufferAvail;
		uint32					fWriteBufferIn;
		uint32					fWriteBufferOut;
		sem_id					fWriteBufferSem;

		/* variables used in callback functionality */
		size_t					fActualLengthRead;
		uint32					fStatusRead;
		size_t					fActualLengthWrite;
		uint32					fStatusWrite;
		size_t					fActualLengthInterrupt;
		uint32					fStatusInterrupt;

		/* semaphores used in callbacks */
		sem_id					fDoneRead;
		sem_id					fDoneWrite;

		uint16					fControlOut;
		bool					fInputStopped;

		struct tty *			fMasterTTY;
		struct tty *			fSlaveTTY;
		struct tty_cookie *		fSystemTTYCookie;
		struct tty_cookie *		fDeviceTTYCookie;
		struct termios			fTTYConfig;

		/* device thread management */
		thread_id				fDeviceThread;
		bool					fStopDeviceThread;
};

#endif // _SERIAL_DEVICE_H_
