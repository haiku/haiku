/*
 * Copyright 2008-2011, Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_HID_DEVICE_H
#define USB_HID_DEVICE_H


#include "HIDParser.h"

#include <USB3.h>


class ProtocolHandler;


class HIDDevice {
public:
								HIDDevice(usb_device device,
									const usb_configuration_info *config,
									size_t interfaceIndex);
								~HIDDevice();

			void				SetParentCookie(int32 cookie);
			int32				ParentCookie() const { return fParentCookie; }

			status_t			InitCheck() const { return fStatus; }

			bool				IsOpen() const { return fOpenCount > 0; }
			status_t			Open(ProtocolHandler *handler, uint32 flags);
			status_t			Close(ProtocolHandler *handler);
			int32				OpenCount() const { return fOpenCount; }

			void				Removed();
			bool				IsRemoved() const { return fRemoved; }

			status_t			MaybeScheduleTransfer();

			status_t			SendReport(HIDReport *report);

			HIDParser &			Parser() { return fParser; }
			ProtocolHandler *	ProtocolHandlerAt(uint32 index) const;

			// only to be used for the kernel debugger information
			usb_pipe			InterruptPipe() const { return fInterruptPipe; }

private:
	static	void				_TransferCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);

private:
			status_t			fStatus;
			usb_device			fDevice;
			usb_pipe			fInterruptPipe;
			size_t				fInterfaceIndex;

			int32				fTransferScheduled;
			size_t				fTransferBufferSize;
			uint8 *				fTransferBuffer;

			int32				fParentCookie;
			int32				fOpenCount;
			bool				fRemoved;

			HIDParser			fParser;

			uint32				fProtocolHandlerCount;
			ProtocolHandler *	fProtocolHandlerList;
};


#endif // USB_HID_DEVICE_H
