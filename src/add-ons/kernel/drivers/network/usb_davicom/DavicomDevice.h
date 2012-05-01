/*
 *	Davicom DM9601 USB 1.1 Ethernet Driver.
 *	Copyright (c) 2008, 2011 Siarzhuk Zharski <imker@gmx.li>
 *	Copyright (c) 2009 Adrien Destugues <pulkomandy@gmail.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 */
#ifndef _USB_DAVICOM_DEVICE_H_
#define _USB_DAVICOM_DEVICE_H_


#include <ether_driver.h>
#include <util/Vector.h>

#include "Driver.h"


struct DM9601NotifyData {
	// Network Status Register
	uint RXRDY	:1;
	uint RXOV	:1;
	uint TX1END	:1;
	uint TX2END	:1;
	uint TXFULL	:1;
	uint WAKEST	:1;
	uint LINKST	:1;
	uint SPEED	:1;

	struct {
		uint		:2;
		uint EC		:1;
		uint COL	:1;
		uint LC		:1;
		uint NC		:1;
		uint LCR	:1;
		uint		:1;
	} __attribute__((__packed__)) TSR1, TSR2;

	// RX Status Register
	uint FOE	:1;
	uint CE		:1;
	uint AE		:1;
	uint PLE	:1;
	uint RWTO	:1;
	uint LCS	:1;
	uint MF		:1;
	uint RT		:1;

	// RX Overflows Count
	uint RXFU	:1;
	uint ROC	:7;

	uint RXC	:8;
	uint TXC	:8;
	uint GPR	:8;

	DM9601NotifyData() { memset(this, 0, sizeof(DM9601NotifyData)); }
} __attribute__((__packed__));


struct DeviceInfo {
	uint16	fIds[2];

	const char* fName;
	inline uint16		VendorId()	{ return fIds[0]; }
	inline uint16		ProductId()	{ return fIds[1]; }
	inline uint32		Key()		{ return fIds[0] << 16 | fIds[1]; }
};

class DavicomDevice {

		struct _Statistics {
			// NSR
			int txFull;
			int rxOverflow;
			int rxOvCount;
			// RSR
			int	runtFrames;
			int lateRXCollisions;
			int rwTOs;
			int physLayerErros;
			int alignmentErros;
			int crcErrors;
			int overErrors;
			// TSR 1/2
			int lateTXCollisions;
			int lostOfCarrier;
			int noCarrier;
			int txCollisions;
			int excCollisions;

			int notifyCount;
			int readCount;
			int writeCount;
			_Statistics() { memset(this, 0, sizeof(_Statistics)); }
		};

public:
							DavicomDevice(usb_device device, DeviceInfo& Info);
							~DavicomDevice();

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
		status_t			SetupDevice(bool deviceReplugged);

private:
static	void				_ReadCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_WriteCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_NotifyCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);

		status_t			_SetupEndpoints();

		status_t			_StartDevice();
		status_t			_StopDevice();
		status_t			_OnNotify(uint32 actualLength);
		status_t			_GetLinkState(ether_link_state *state);
		status_t			_SetPromiscuousMode(bool bOn);
		uint32				_EthernetCRC32(const uint8* buffer, size_t length);
		status_t			_ModifyMulticastTable(bool join,
								ether_address_t *group);
		status_t			_ReadMACAddress(ether_address_t *address);

		status_t			_ReadRegister(uint8 reg, size_t size, uint8* buffer);
		status_t			_WriteRegister(uint8 reg, size_t size, uint8* buffer);
		status_t			_Write1Register(uint8 reg, uint8 buffer);
		status_t			_ReadMII(uint8 reg, uint16* data);
		status_t			_WriteMII(uint8 reg, uint16 data);
		status_t			_InitMII();
		status_t			_EnableInterrupts(bool enable);


		// device info
		usb_device			fDevice;
		DeviceInfo			fDeviceInfo;
		ether_address_t		fMACAddress;

		// state tracking
		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		bool				fHasConnection;
		bool				fTXBufferFull;
		bool				fNonBlocking;
		vint32				fInsideNotify;

		// pipes for notifications, data io and tx packet size
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
		sem_id				fLinkStateChangeSem;

		DM9601NotifyData*	fNotifyData;
		_Statistics			fStats;
		Vector<uint32>		fMulticastHashes;
};

#endif // _USB_DAVICOM_DEVICE_H_

