/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2008-2011, Michael Lotz <mmlr@mlotz.ch>
 * Copyright 2023 Vladimir Serbinenko <phcoder@gmail.com>
 * Distributed under the terms of the MIT license.
 */
#ifndef I2C_HID_PROTOCOL_H
#define I2C_HID_PROTOCOL_H

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

#endif // I2C_HID_PROTOCOL_H
