/*
 *	Davicom DM9601 USB 1.1 Ethernet Driver.
 *	Copyright (c) 2009 Adrien Destugues <pulkomandy@gmail.com>
 *	Copyright (c) 2008, 2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the 
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_Davicom_DEVICE_H_
#define _USB_Davicom_DEVICE_H_


#include <net/if_media.h>

#include "Driver.h"


class DavicomDevice {
public:
							DavicomDevice(usb_device device, const char *description);
		virtual				~DavicomDevice();

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

		status_t			_ReadRegister(uint8 reg, size_t size, uint8* buffer);
		status_t			_WriteRegister(uint8 reg, size_t size, uint8* buffer);
		status_t			_Write1Register(uint8 reg, uint8 buffer);
		status_t			_ReadMII(uint8 reg, uint16* data);
		status_t			_WriteMII(uint8 reg, uint16 data);
		status_t			_InitMII();
		status_t			_EnableInterrupts(bool enable);

static const int			kFrameSize = 1518;
static const size_t			kRXHeaderSize = 3;
static const size_t			kTXHeaderSize = 2;

protected:
		/* overrides */
virtual	status_t			StartDevice() ;
virtual	status_t			StopDevice();
virtual status_t			OnNotify(uint32 actualLength) ;
virtual	status_t			GetLinkState(ether_link_state *state) ;
virtual	status_t			SetPromiscuousMode(bool bOn);
virtual	status_t			ModifyMulticastTable(bool add, uint8 address);
		status_t			ReadMACAddress(ether_address_t *address);
		
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

		// pipes for notifications and data io
		usb_pipe			fNotifyEndpoint;
		usb_pipe			fReadEndpoint;
		usb_pipe			fWriteEndpoint;
		uint16				fMaxTXPacketSize;

		// data stores for async usb transfers
		uint32				fActualLengthRead;
		uint32				fActualLengthWrite;
		int32				fStatusRead;
		int32				fStatusWrite;
		sem_id				fNotifyReadSem;
		sem_id				fNotifyWriteSem;

		uint8 *				fNotifyBuffer;
static const size_t			kNotifyBufferSize = 8;

		// connection data
		sem_id				fLinkStateChangeSem;
		ether_address_t		fMACAddress;
		bool				fHasConnection;
		bool				fTXBufferFull;
};

#endif //_USB_Davicom_DEVICE_H_
