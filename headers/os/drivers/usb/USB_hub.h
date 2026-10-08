/*
 * Copyright 2026, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef USB_HUB_H
#define USB_HUB_H


#define USB_HUB_DEVICE_CLASS 		0x09

#define USB_HUB_DEVICE_SUBCLASS 	0x00

enum {
	USB_HUB_PROTOCOL_FULLSPEED 			= 0,	// Low/Full Speed USB 1.x
	USB_HUB_PROTOCOL_HIGHSPEED_SINGLE_TT,
	USB_HUB_PROTOCOL_HIGHSPEED_TT_PER_PORT,
	USB_HUB_PROTOCOL_USB_3
};

// Device class specific requests

// USB 2.0 specification, chapter 11.24.2
#define USB_REQUEST_CLEAR_TT_BUFFER		8
#define USB_REQUEST_RESET_TT			9
#define USB_REQUEST_GET_TT_STATE		10
#define USB_REQUEST_STOP_TT				11

// USB 3.0 SuperSpeed specification, chapter 10.16.2
#define USB_REQUEST_SET_HUB_DEPTH		12
#define USB_REQUEST_GET_PORT_ERR_COUNT	13

// Device class specific descriptor

#define USB_DESCRIPTOR_HUB				0x29
#define USB_DESCRIPTOR_HUB_SS			0x2A

// USB HUB characteristics
#define USB_HUB_POWER_SWITCHING_MODE_MASK	0x0003
#define USB_HUB_GANGED_POWER_SWITCHING		0x0000
#define USB_HUB_INDIVIDUAL_POWER_SWITCHING	0x0001
#define USB_HUB_NO_POWER_SWITCHING			0x0002
#define USB_HUB_COMPOUND_DEVICE				0x0004
#define USB_HUB_OVERCURRENT_MODE_MASK		0x0018
#define USB_HUB_OVERCURRENT_MODE_GLOBAL		0x0000
#define USB_HUB_OVERCURRENT_MODE_INDIVIDUAL	0x0008
#define USB_HUB_OVERCURRENT_MODE_NONE		0x0010
#define USB_HUB_TT_THINK_TIME_MASK			0x0060
#define USB_HUB_TT_THINK_TIME_8				0x0000
#define USB_HUB_TT_THINK_TIME_16			0x0020
#define USB_HUB_TT_THINK_TIME_24			0x0040
#define USB_HUB_TT_THINK_TIME_32			0x0060
#define USB_HUB_PORTS_INDICATORS			0x0080
#define USB_USB_PORT_POWER_CONTROL_MASK		0x0700

typedef struct usb_hub_descriptor {
	uint8 length;
	uint8 descriptor_type;
	uint8 num_ports;
	uint16 characteristics;
	uint8 power_on_to_power_good;
	uint8 max_power;
	uint8 device_removeable;	//Should be variable!!!
	uint8 power_control_mask;	//Deprecated
} _PACKED usb_hub_descriptor;

// USB 3.0 SuperSpeed specification, chapter 10.15.2.1
typedef struct usb_hub_ss_descriptor {
	uint8 length;
	uint8 descriptor_type;
	uint8 num_ports;
	uint16 characteristics;
	uint8 power_on_to_power_good;	// per 2ms step
	uint8 max_power; 				// in mA
	uint8 decode_latency;			// per 100ns step
	uint16 delay;					// in nanosecond
	uint16 device_removeable;
} _PACKED usb_hub_ss_descriptor;

// USB Spec 1.1 page 273
// USB Spec 2.0 chapter 11.24.2.7
// USB 3.2 spec, chapter 10.16.2.6
typedef struct usb_port_status {
	uint16 status;
	uint16 change;
} _PACKED usb_port_status;

// USB 3.2 spec, chapter 10.16.2.6
// (only for GET_STATUS value == 2 request)
typedef struct usb_extended_port_status {
	uint16 status;
	uint16 change;
	uint32 extended_status;
} _PACKED usb_extended_port_status;

//The bits in the usb_port_status struct
// USB 1.1 spec page 274
// USB2_LinkPowerMangement_ECN[final].pdf page 25
#define PORT_STATUS_CONNECTION		0x0001
#define PORT_STATUS_ENABLE			0x0002
#define PORT_STATUS_SUSPEND			0x0004
#define PORT_STATUS_OVER_CURRENT	0x0008
#define PORT_STATUS_RESET			0x0010
#define PORT_STATUS_L1				0x0020
#define PORT_STATUS_POWER			0x0100
#define PORT_STATUS_LOW_SPEED		0x0200
#define PORT_STATUS_HIGH_SPEED		0x0400
#define PORT_STATUS_TEST			0x0800
#define PORT_STATUS_INDICATOR		0x1000
// USB 3.2 spec table 10-13
#define PORT_STATUS_SS_LINK_STATE	0x01e0
#define PORT_STATUS_SS_POWER		0x0200
#define PORT_STATUS_SS_SPEED		0x1c00


//The feature requests with ports
// USB 1.1 spec page 268
#define PORT_CONNECTION				0
#define PORT_ENABLE					1
#define PORT_SUSPEND				2
#define PORT_OVER_CURRENT			3
#define PORT_RESET					4
#define PORT_POWER					8
#define PORT_LOW_SPEED				9
#define C_PORT_CONNECTION			16
#define C_PORT_ENABLE				17
#define C_PORT_SUSPEND				18
#define C_PORT_OVER_CURRENT			19
#define C_PORT_RESET				20

// USB 3.2 spec table 10-9
#define PORT_LINK_STATE				5
#define PORT_U1_TIMEOUT				23
#define PORT_U2_TIMEOUT				24
#define C_PORT_LINK_STATE			25
#define C_PORT_CONFIG_ERROR			26
#define PORT_REMOTE_WAKE_MASK		27
#define PORT_BH_PORT_RESET			28
#define C_PORT_BH_PORT_RESET		29
#define PORT_FORCE_LINKPM_STATE		30

// USB 3.0 spec table 10-11
#define PORT_CHANGE_BH_PORT_RESET	0x0020
#define PORT_CHANGE_LINK_STATE		0x0040

#endif // USB_HUB_H
