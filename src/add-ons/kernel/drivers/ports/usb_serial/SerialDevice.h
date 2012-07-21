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
#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_


#include "Driver.h"


struct usb_serial_device {
	uint32      vendorID;
	uint32      productID;
	const char* deviceName;
};


class SerialDevice {
public:
								SerialDevice(usb_device device,
									uint16 vendorID, uint16 productID,
									const char *description);
virtual							~SerialDevice();

static	SerialDevice *			MakeDevice(usb_device device, uint16 vendorID,
									uint16 productID);

		status_t				Init();

		usb_device				Device() { return fDevice; };
		uint16					ProductID() { return fProductID; };
		uint16					VendorID() { return fVendorID; };
		const char *			Description() { return fDescription; };

		void					SetControlPipe(usb_pipe handle);
		usb_pipe				ControlPipe() { return fControlPipe; };

		void					SetReadPipe(usb_pipe handle);
		usb_pipe				ReadPipe() { return fReadPipe; };

		void					SetWritePipe(usb_pipe handle);
		usb_pipe				WritePipe() { return fWritePipe; }

		char *					ReadBuffer() { return fReadBuffer; };
		size_t					ReadBufferSize() { return fReadBufferSize; };

		char *					WriteBuffer() { return fWriteBuffer; };
		size_t					WriteBufferSize() { return fWriteBufferSize; };

		void					SetModes(struct termios *tios);
		bool					Service(struct tty *tty, uint32 op,
									void *buffer, size_t length);

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
virtual	status_t				AddDevice(const usb_configuration_info *config);

virtual	status_t				ResetDevice();

virtual	status_t				SetLineCoding(usb_cdc_line_coding *coding);
virtual	status_t				SetControlLineState(uint16 state);

virtual	void					OnRead(char **buffer, size_t *numBytes);
virtual	void					OnWrite(const char *buffer, size_t *numBytes,
									size_t *packetBytes);
virtual	void					OnClose();

protected:
		void					SetReadBufferSize(size_t size) { fReadBufferSize = size; };
		void					SetWriteBufferSize(size_t size) { fWriteBufferSize = size; };
		void					SetInterruptBufferSize(size_t size) { fInterruptBufferSize = size; };

private:
static	int32					_InputThread(void *data);
		status_t				_WriteToDevice();

static	void					_ReadCallbackFunction(void *cookie,
									int32 status, void *data,
									uint32 actualLength);
static	void					_WriteCallbackFunction(void *cookie,
									int32 status, void *data,
									uint32 actualLength);
static	void					_InterruptCallbackFunction(void *cookie,
									int32 status, void *data,
									uint32 actualLength);

		usb_device				fDevice;		// USB device handle
		uint16					fVendorID;
		uint16					fProductID;
		const char *			fDescription;	// informational description
		bool					fDeviceOpen;
		bool					fDeviceRemoved;

		/* communication pipes */
		usb_pipe				fControlPipe;
		usb_pipe				fReadPipe;
		usb_pipe				fWritePipe;

		/* line coding */
		usb_cdc_line_coding		fLineCoding;

		/* data buffers */
		area_id					fBufferArea;
		char *					fReadBuffer;
		size_t					fReadBufferSize;
		char *					fOutputBuffer;
		size_t					fOutputBufferSize;
		char *					fWriteBuffer;
		size_t					fWriteBufferSize;
		char *					fInterruptBuffer;
		size_t					fInterruptBufferSize;

		/* variables used in callback functionality */
		size_t					fActualLengthRead;
		status_t				fStatusRead;
		size_t					fActualLengthWrite;
		status_t				fStatusWrite;
		size_t					fActualLengthInterrupt;
		status_t				fStatusInterrupt;

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

		/* input thread management */
		thread_id				fInputThread;
		bool					fStopThreads;
};

#endif // _USB_DEVICE_H_
