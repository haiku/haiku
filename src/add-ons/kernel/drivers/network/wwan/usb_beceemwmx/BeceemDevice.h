/*
 *	Beceem WiMax USB Driver
 *	Copyright 2010-2011 Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 *
 *	Partially using:
 *		USB Ethernet Control Model devices
 *			(c) 2008 by Michael Lotz, <mmlr@mlotz.ch>
 *		ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver
 *			(c) 2008 by S.Zharski, <imker@gmx.li>
 */
#ifndef _USB_BECEEM_DEVICE_H_
#define _USB_BECEEM_DEVICE_H_

#include <net/if_media.h>
#include <sys/ioctl.h>

#include "BeceemNVM.h"
#include "BeceemDDR.h"
#include "BeceemCPU.h"
#include "BeceemLED.h"

#include "Driver.h"
#include "DeviceStruct.h"

#define MAX_USB_IO_RETRIES	1
#define MAX_USB_TRANSFER	255


class BeceemDevice
	:
	public BeceemNVM,
	public BeceemDDR,
	public BeceemCPU,
	public BeceemLED
{

public:
								BeceemDevice(usb_device device,
									const char *description);
virtual							~BeceemDevice();

			status_t			InitCheck() { return fStatus; };

			status_t			Open(uint32 flags);
			bool				IsOpen() { return fOpen; };

			status_t			Close();
			status_t			Free();

			status_t			Read(uint8 *buffer, size_t *numBytes);
			status_t			Write(const uint8 *buffer, size_t *numBytes);
			status_t			Control(uint32 op, void *buffer, size_t length);
			status_t			LoadConfig();
			void				DumpConfig();
			status_t			PushConfig(uint32 loc);
			status_t			PushFirmware(uint32 loc);

			void				Removed();

			status_t			CompareAndReattach(usb_device device);
virtual		status_t			SetupDevice(bool deviceReplugged);

			status_t			ReadRegister(uint32 reg,
									size_t size, uint32* buffer);
			status_t			WriteRegister(uint32 reg,
									size_t size, uint32* buffer);
			status_t			BizarroReadRegister(uint32 reg,
									size_t size, uint32* buffer);
			status_t			BizarroWriteRegister(uint32 reg,
									size_t size, uint32* buffer);

private:
static		void				_ReadCallback(void *cookie, int32 status,
									void *data, uint32 actualLength);
static		void				_WriteCallback(void *cookie, int32 status,
									void *data, uint32 actualLength);
static		void				_NotifyCallback(void *cookie, int32 status,
									void *data, uint32 actualLength);

			status_t			_SetupEndpoints();

			status_t			IdentifyChipset();

static		const int			kFrameSize = 1518;
static		const uint8			kRXHeaderSize = 3;
static		const uint8			kTXHeaderSize = 2;

protected:
virtual		status_t			StartDevice() ;
virtual		status_t			StopDevice();
virtual		status_t			OnNotify(uint32 actualLength) ;
virtual		status_t			GetLinkState(ether_link_state *state) ;
virtual		status_t			SetPromiscuousMode(bool bOn);
virtual		status_t			ModifyMulticastTable(bool add, uint8 address);


			// state tracking
			status_t			fStatus;
			bool				fOpen;
			vint32				fInsideNotify;
			usb_device			fDevice;
			uint16				fVendorID;
			uint16				fProductID;
const		char *				fDescription;
			bool				fNonBlocking;

struct		WIMAX_DEVICE		*pwmxdevice;
			// Our driver device struct

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
			ether_address_t		fMACAddress;
			bool				fHasConnection;
			bool				fTXBufferFull;
};

#endif /*_USB_BECEEM_DEVICE_H_*/

