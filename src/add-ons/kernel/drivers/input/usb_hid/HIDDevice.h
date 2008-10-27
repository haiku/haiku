/*
	Driver for USB Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_HID_DEVICE_H_
#define _USB_HID_DEVICE_H_

#include "hidparse.h"
#include "ring_buffer.h"
#include <USB3.h>

class HIDDevice {
public:
								HIDDevice(usb_device device,
									usb_pipe interruptPipe,
									size_t interfaceIndex,
									report_insn *instructions,
									size_t instructionCount,
									size_t totalReportSize,
									size_t ringBufferSize);
virtual							~HIDDevice();

static	HIDDevice *				MakeHIDDevice(usb_device device,
									const usb_configuration_info *config,
									size_t interfaceIndex);

		void					SetBaseName(const char *baseName);
		const char *			Name() { return fName; };

		void					SetParentCookie(int32 cookie);
		int32					ParentCookie() { return fParentCookie; };

		status_t				InitCheck() { return fStatus; };

virtual	status_t				Open(uint32 flags);
		bool					IsOpen() { return fOpen; };

virtual	status_t				Close();
virtual	status_t				Free();

virtual	status_t				Read(uint8 *buffer, size_t *numBytes);
virtual	status_t				Write(const uint8 *buffer, size_t *numBytes);
virtual	status_t				Control(uint32 op, void *buffer, size_t length);

virtual	void					Removed();
		bool					IsRemoved() { return fRemoved; };

protected:
		void					_SetTransferProcessed();
		bool					_IsTransferUnprocessed();
		status_t				_ScheduleTransfer();

		int32					_RingBufferReadable();
		status_t				_RingBufferRead(void *buffer, size_t length);
		status_t				_RingBufferWrite(const void *buffer,
									size_t length);

		status_t				fStatus;
		usb_device				fDevice;
		usb_pipe				fInterruptPipe;
		size_t					fInterfaceIndex;
		report_insn *			fInstructions;
		size_t					fInstructionCount;
		size_t					fTotalReportSize;

		// transfer data
		bool					fTransferUnprocessed;
		status_t				fTransferStatus;
		size_t					fTransferActualLength;
		uint8 *					fTransferBuffer;
		sem_id					fTransferNotifySem;

private:
static	void					_TransferCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);

		char *					fName;
		int32					fParentCookie;
volatile bool					fOpen;
		bool					fRemoved;

		struct ring_buffer *	fRingBuffer;
};

#endif // _USB_HID_DEVICE_H_
