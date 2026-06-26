/*
	Driver for USB NCM network devices
	Copyright (C) 2022-2026 Adrien Destugues <pulkomandy@pulkomandy.tk>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_NCM_DEVICE_H_
#define _USB_NCM_DEVICE_H_

#include "Driver.h"

class NCMDevice {
public:
							NCMDevice(usb_device device);
							~NCMDevice();

		status_t			InitCheck() { return fStatus; };

		status_t			Open();
		bool				IsOpen() { return fOpen; };

		status_t			Close();
		status_t			Free();

		status_t			Read(uint8 *buffer, size_t *numBytes);
		status_t			Write(const uint8 *buffer, size_t *numBytes);
		status_t			Control(uint32 op, void *buffer, size_t length);

		void				Removed();
		bool				IsRemoved() { return fRemoved; };

private:
static	void				_ReadCallback(void *cookie, int32 status,
								void *data, size_t actualLength);
static	void				_WriteCallback(void *cookie, int32 status,
								void *data, size_t actualLength);
static	void				_NotifyCallback(void *cookie, int32 status,
								void *data, size_t actualLength);

		status_t			_OutputRequest(uint16 request, void*, size_t);
		status_t			_InputRequest(uint16 request, void*, size_t);

		status_t			_SetupDevice();

		// state tracking
		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		int32				fInsideNotify;
		usb_device			fDevice;

		// interface and device infos
		uint8				fDataInterfaceIndex;
		uint32				fMaxSegmentSize;

		// pipes for notifications and data io
		usb_pipe			fNotifyEndpoint;
		usb_pipe			fReadEndpoint;
		usb_pipe			fWriteEndpoint;

		// data stores for async usb transfers
		uint32				fActualLengthRead;
		uint32				fActualLengthWrite;
		int32				fStatusRead;
		int32				fStatusWrite;
		sem_id				fNotifyReadSem;
		sem_id				fNotifyWriteSem;
		sem_id				fLockWriteSem;

		uint8				fNotifyBuffer[16];
		uint8				fReadBuffer[0x4000];
		uint8*				fReadHeader;
		uint8*				fReadHeader2;

		// NCM sequence number, to increment with each packet sent to the device
		uint16				fSequence;

		// connection data
		sem_id				fLinkStateChangeSem;
		uint8				fMACAddress[6];
		uint32				fMediaConnectState;
		uint32				fDownstreamSpeed;
		uint32				fInputMaxSize;
};

#endif //_USB_NCM_DEVICE_H_
