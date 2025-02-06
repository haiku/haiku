/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2008-2011, Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef I2C_HID_DEVICE_H
#define I2C_HID_DEVICE_H


#include <i2c.h>

#include "HIDParser.h"
#include "I2CHIDProtocol.h"


class ProtocolHandler;


class HIDDevice {
public:
								HIDDevice(uint16 descriptorAddress, i2c_device_interface* i2c,
									i2c_device i2cCookie);
								~HIDDevice();

			status_t			InitCheck() const { return fStatus; }

			bool				IsOpen() const { return fOpenCount > 0; }
			status_t			Open(ProtocolHandler *handler, uint32 flags);
			status_t			Close(ProtocolHandler *handler);
			int32				OpenCount() const { return fOpenCount; }

			void				Removed();
			bool				IsRemoved() const { return fRemoved; }

			status_t			MaybeScheduleTransfer(HIDReport *report);

			status_t			SendReport(HIDReport *report);

			HIDParser &			Parser() { return fParser; }
			ProtocolHandler *	ProtocolHandlerAt(uint32 index) const;

private:
#if 0
	static	void				_TransferCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);
	static	void				_UnstallCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);
#endif
			status_t			_Reset();
			status_t			_SetPower(uint8 power);
			status_t			_WriteReport(uint8 type, uint8 id, void *data,
									size_t reportSize);
			status_t			_FetchBuffer(uint8* cmd, size_t cmdLength,
									void* buffer, size_t bufferLength);
			status_t			_FetchReport(uint8 type, uint8 id,
									size_t reportSize);
			status_t			_ExecCommand(i2c_op op, uint8* cmd,
									size_t cmdLength, void* buffer,
									size_t bufferLength);

private:
			status_t			fStatus;
			

			bigtime_t			fTransferLastschedule;
			int32				fTransferScheduled;
			size_t				fTransferBufferSize;
			uint8 *				fTransferBuffer;

			int32				fOpenCount;
			bool				fRemoved;

			HIDParser			fParser;

			uint32				fProtocolHandlerCount;
			ProtocolHandler *	fProtocolHandlerList;

			uint16				fDescriptorAddress;
			i2c_hid_descriptor	fDescriptor;

			uint8*				fReportDescriptor;

			i2c_device_interface*	fI2C;
			i2c_device				fI2CCookie;
	
};


#endif // I2C_HID_DEVICE_H
