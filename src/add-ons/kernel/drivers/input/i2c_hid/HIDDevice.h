/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2008-2011, Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef I2C_HID_DEVICE_H
#define I2C_HID_DEVICE_H


#include <i2c.h>

#include "HIDParser.h"


/* 5.1.1 - HID Descriptor Format */
typedef struct i2c_hid_descriptor {
	uint16 wHIDDescLength;
	uint16 bcdVersion;
	uint16 wReportDescLength;
	uint16 wReportDescRegister;
	uint16 wInputRegister;
	uint16 wMaxInputLength;
	uint16 wOutputRegister;
	uint16 wMaxOutputLength;
	uint16 wCommandRegister;
	uint16 wDataRegister;
	uint16 wVendorID;
	uint16 wProductID;
	uint16 wVersionID;
	uint32 reserved;
} _PACKED i2c_hid_descriptor;


enum {
	I2C_HID_CMD_RESET			= 0x1,
	I2C_HID_CMD_GET_REPORT		= 0x2,
	I2C_HID_CMD_SET_REPORT		= 0x3,
	I2C_HID_CMD_GET_IDLE		= 0x4,
	I2C_HID_CMD_SET_IDLE		= 0x5,
	I2C_HID_CMD_GET_PROTOCOL	= 0x6,
	I2C_HID_CMD_SET_PROTOCOL	= 0x7,
	I2C_HID_CMD_SET_POWER		= 0x8,
};

enum {
	I2C_HID_POWER_ON	= 0x0,
	I2C_HID_POWER_OFF	= 0x1,
};


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
	static	void				_TransferCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);
	static	void				_UnstallCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);

			status_t			_Reset();
			status_t			_SetPower(uint8 power);
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
