/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008, 2011 S.Zharski <imker@gmx.li>
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


#include <ether_driver.h>
#include <util/Vector.h>

#include "Driver.h"
#include "MIIBus.h"


struct DeviceInfo {
	union Id {
		uint16	fIds[2];
		uint32	fKey;
	}			fId;

	enum Type {
		AX88172 = 0,
		AX88772 = 1,
		AX88178 = 2
	}			fType;

	const char* fName;

	inline uint16		VendorId()	{ return fId.fIds[0]; }
	inline uint16		ProductId()	{ return fId.fIds[1]; }
	inline uint32		Key()		{ return fId.fKey;    }
};


class ASIXDevice {
public:
							ASIXDevice(usb_device device, DeviceInfo& devInfo);
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
		// overrides
virtual	status_t			StartDevice() = 0;
virtual	status_t			StopDevice();
virtual status_t			OnNotify(uint32 actualLength) = 0;
virtual	status_t			GetLinkState(ether_link_state *state) = 0;
virtual	status_t			SetPromiscuousMode(bool bOn);
		uint32				EthernetCRC32(const uint8* buffer, size_t length);
virtual	status_t			ModifyMulticastTable(bool add,
								ether_address_t* group);
		status_t			ReadMACAddress(ether_address_t *address);
		status_t			ReadRXControlRegister(uint16 *rxcontrol);
		status_t			WriteRXControlRegister(uint16 rxcontrol);

		// device info
		usb_device			fDevice;
		DeviceInfo			fDeviceInfo;
		ether_address_t		fMACAddress;

		// state tracking
		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		bool				fHasConnection;
		bool				fNonBlocking;
		vint32				fInsideNotify;

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
		sem_id				fLinkStateChangeSem;

		// MII bus handler
		MIIBus				fMII;

		// connection data
		bool				fUseTRXHeader;
		uint8				fIPG[3];
		uint8				fReadNodeIDRequest;
		Vector<uint32>		fMulticastHashes;
};

#endif // _USB_ASIX_DEVICE_H_
