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

		status_t			Open(uint32 flags);
		bool				IsOpen() { return fOpen; };

		status_t			Close();
		status_t			Free();

		status_t			Read(uint8 *buffer, size_t *numBytes);
		status_t			Write(const uint8 *buffer, size_t *numBytes);
		status_t			Control(uint32 op, void *buffer, size_t length);

		void				Removed() { fRemoved = true; };
		bool				IsRemoved() { return fRemoved; };

private:
static	void				_ReadCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_WriteCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);

		status_t			_ReadMACAddress();

		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		usb_device			fDevice;

		uint8				fMACAddress[6];

		uint8				fControlInterfaceIndex;
		uint8				fDataInterfaceIndex;
		uint8				fMACAddressIndex;
		uint16				fMaxSegmentSize;

		usb_pipe			fControlEndpoint;
		usb_pipe			fReadEndpoint;
		usb_pipe			fWriteEndpoint;

		uint32				fActualLengthRead;
		uint32				fActualLengthWrite;
		int32				fStatusRead;
		int32				fStatusWrite;
		sem_id				fNotifyRead;
		sem_id				fNotifyWrite;
};

#endif //_USB_ECM_DEVICE_H_
