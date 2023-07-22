/*
	Driver for USB RNDIS network devices
	Copyright (C) 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_RNDIS_DEVICE_H_
#define _USB_RNDIS_DEVICE_H_

#include "Driver.h"

class RNDISDevice {
public:
							RNDISDevice(usb_device device);
							~RNDISDevice();

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

		status_t			CompareAndReattach(usb_device device);

private:
static	void				_ReadCallback(void *cookie, int32 status,
								void *data, size_t actualLength);
static	void				_WriteCallback(void *cookie, int32 status,
								void *data, size_t actualLength);
static	void				_NotifyCallback(void *cookie, int32 status,
								void *data, size_t actualLength);

		status_t			_SendCommand(const void*, size_t);
		status_t			_ReadResponse(void*, size_t);

		status_t			_RNDISInitialize();
		status_t			_GetOID(uint32 oid, void* buffer, size_t length);

		status_t			_SetupDevice();
		status_t			_ReadMACAddress(usb_device device, uint8 *buffer);
		status_t			_ReadMaxSegmentSize(usb_device device);
		status_t			_ReadMediaState(usb_device device);
		status_t			_ReadLinkSpeed(usb_device device);
		status_t			_EnableBroadcast(usb_device device);

		// state tracking
		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		int32				fInsideNotify;
		usb_device			fDevice;
		uint16				fVendorID;
		uint16				fProductID;

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
		sem_id				fNotifyControlSem;

		uint8				fNotifyBuffer[8];
		uint8				fReadBuffer[0x4000];
		uint32*				fReadHeader;

		// connection data
		sem_id				fLinkStateChangeSem;
		uint8				fMACAddress[6];
		uint32				fMediaConnectState;
		uint32				fDownstreamSpeed;
};

#endif //_USB_RNDIS_DEVICE_H_
