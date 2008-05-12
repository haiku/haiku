/*
	Driver for USB Ethernet Control Model devices
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_ECM_DEVICE_H_
#define _USB_ECM_DEVICE_H_

#include "Driver.h"

class ECMDevice {
public:
							ECMDevice(usb_device device);
							~ECMDevice();

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
								void *data, uint32 actualLength);
static	void				_WriteCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_NotifyCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);

		status_t			_SetupDevice();
		status_t			_ReadMACAddress(usb_device device, uint8 *buffer);

		// state tracking
		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		vint32				fInsideNotify;
		usb_device			fDevice;
		uint16				fVendorID;
		uint16				fProductID;

		// interface and device infos
		uint8				fControlInterfaceIndex;
		uint8				fDataInterfaceIndex;
		uint8				fMACAddressIndex;
		uint16				fMaxSegmentSize;

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

		uint8 *				fNotifyBuffer;
		uint32				fNotifyBufferLength;

		// connection data
		sem_id				fLinkStateChangeSem;
		uint8				fMACAddress[6];
		bool				fHasConnection;
		uint32				fDownstreamSpeed;
		uint32				fUpstreamSpeed;
};

#endif //_USB_ECM_DEVICE_H_
