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
		size_t					ReadBufferSize() { return fReadBufferSize; };

		char *					WriteBuffer() { return fWriteBuffer; };
		size_t					WriteBufferSize() { return fWriteBufferSize; };

#ifdef __BEOS__
		void					SetModes();
#endif
		void					SetModes(struct termios *tios);
#ifdef __HAIKU__
		bool					Service(struct tty *tty, uint32 op,
									void *buffer, size_t length);
#else
		bool					Service(struct tty *ptty, struct ddrover *ddr,
									uint flags);
#endif

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

protected:
		void					SetReadBufferSize(size_t size) { fReadBufferSize = size; };
		void					SetWriteBufferSize(size_t size) { fWriteBufferSize = size; };
		void					SetInterruptBufferSize(size_t size) { fInterruptBufferSize = size; };
private:
static	int32					DeviceThread(void *data);

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

		/* data buffers */
		area_id					fBufferArea;
		char *					fReadBuffer;
		size_t					fReadBufferSize;
		char *					fWriteBuffer;
		size_t					fWriteBufferSize;
		char *					fInterruptBuffer;
		size_t					fInterruptBufferSize;

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
#ifdef __HAIKU__
		struct tty *			fMasterTTY;
		struct tty *			fSlaveTTY;
		struct tty_cookie *		fTTYCookie;
#else
		struct ttyfile			fTTYFile;
		struct tty				fTTY;
		struct ddrover			fRover;
#endif

		/* device thread management */
		thread_id				fDeviceThread;
		bool					fStopDeviceThread;

		/* device locks to ensure no concurent reads/writes */
		mutex					fReadLock;
		mutex					fWriteLock;
};

#endif // _SERIAL_DEVICE_H_
