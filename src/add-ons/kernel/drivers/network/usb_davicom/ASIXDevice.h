/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *	
 *	Heavily based on code of the 
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */

#ifndef _USB_ASIX_DEVICE_H_
#define _USB_ASIX_DEVICE_H_

#include <net/if_media.h>

#include "Driver.h"
#include "MIIBus.h"

class ASIXDevice {
public:
							ASIXDevice(usb_device device, const char *description);
		virtual				~ASIXDevice();

		status_t			InitCheck() { return fStatus; };

		status_t			Open(uint32 flags);
		bool				IsOpen() { return fOpen; };

		status_t			Close();
		status_t			Free();

		status_t			Read(uint8 *buffer, size_t *numBytes);
		status_t			Write(const uint8 *buffer, size_t *numBytes);
		status_t			Control(uint32 op, void *buffer, size_t length);

		void				Removed();
		bool				IsRemoved() { return fRemoved; };

		status_t			CompareAndReattach(usb_device device);
virtual	status_t			SetupDevice(bool deviceReplugged);
		
private:
static	void				_ReadCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_WriteCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_NotifyCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);

		status_t			_SetupEndpoints();

protected:
		/* overrides */
virtual	status_t			StartDevice() = 0;
virtual	status_t			StopDevice();
virtual status_t			OnNotify(uint32 actualLength) = 0;
virtual	status_t			GetLinkState(ether_link_state *state) = 0;		
virtual	status_t			SetPromiscuousMode(bool bOn);
virtual	status_t			ModifyMulticastTable(bool add, uint8 address);
		status_t			ReadMACAddress(ether_address_t *address);
		status_t			ReadRXControlRegister(uint16 *rxcontrol);
		status_t			WriteRXControlRegister(uint16 rxcontrol);
		
		// state tracking
		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		vint32				fInsideNotify;
		usb_device			fDevice;
		uint16				fVendorID;
		uint16				fProductID;
const	char *				fDescription;
		bool				fNonBlocking;

		// interface and device infos
		uint16				fFrameSize;

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

		// MII bus handler
		MIIBus				fMII;

		// connection data
		sem_id				fLinkStateChangeSem;
		ether_address_t		fMACAddress;
		bool				fHasConnection;
		bool				fUseTRXHeader;
		uint8				fIPG[3];
		uint8				fReadNodeIDRequest;
		uint8				fReadRXControlRequest;
		uint8				fWriteRXControlRequest;
		uint16				fPromiscuousBits;
};

#endif //_USB_ASIX_DEVICE_H_
