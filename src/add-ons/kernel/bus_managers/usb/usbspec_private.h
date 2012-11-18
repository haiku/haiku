/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */
#ifndef _USBSPEC_PRIVATE_H
#define _USBSPEC_PRIVATE_H


#include <KernelExport.h>
#include <USB3.h>


#define USB_MAX_AREAS					8
#define USB_MAX_FRAGMENT_SIZE			B_PAGE_SIZE * 96
#define USB_MAX_PORT_COUNT				16

#define USB_DELAY_BUS_RESET				100000
#define USB_DELAY_DEVICE_POWER_UP		300000
#define USB_DELAY_HUB_POWER_UP			200000
#define USB_DELAY_PORT_RESET			50000
#define USB_DELAY_PORT_RESET_RECOVERY	250000
#define USB_DELAY_SET_ADDRESS_RETRY		200000
#define USB_DELAY_SET_ADDRESS			10000
#define USB_DELAY_SET_CONFIGURATION		50000
#define USB_DELAY_HUB_EXPLORE			1000000

// For bandwidth calculation
#define	USB_BW_HOST_DELAY					1000
#define	USB_BW_SETUP_LOW_SPEED_PORT_DELAY	333


/*
	Important data from the USB spec (not interesting for drivers)
*/

struct memory_chunk
{
	uint32 next_item;
	uint32 physical;
};


struct usb_request_data
{
	uint8   RequestType;
	uint8   Request;
	uint16  Value;
	uint16  Index;
	uint16  Length;
};


struct usb_isochronous_data {
	usb_iso_packet_descriptor *packet_descriptors;
	uint32 packet_count;
	uint32 *starting_frame_number;
	uint32 flags;
};


struct usb_hub_descriptor
{
	uint8 length;
	uint8 descriptor_type;
	uint8 num_ports;
	uint16 characteristics;
	uint8 power_on_to_power_good;
	uint8 max_power;
	uint8 device_removeable;	//Should be variable!!!
	uint8 power_control_mask;	//Deprecated
} _PACKED;

#define USB_DESCRIPTOR_HUB 0x29


// USB Spec 1.1 page 273
struct usb_port_status
{
	uint16 status;
	uint16 change;
};


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
// USB 3.0 spec table 10-11
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

// USB 3.0 spec table 10-8
#define PORT_LINK_STATE				5
#define PORT_U1_TIMEOUT				23
#define PORT_U2_TIMEOUT				24
#define C_PORT_LINK_STATE			25
#define C_PORT_CONFIG_ERROR			26
#define C_PORT_REMOTE_WAKE_MASK		27
#define PORT_BH_PORT_RESET			28
#define C_PORT_BH_PORT_RESET		29
#define PORT_FORCE_LINKPM_STATE		30


#endif // _USBSPEC_PRIVATE_H
