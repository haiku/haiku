/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "ehci.h"


static usb_device_descriptor sEHCIRootHubDevice =
{
	18,								// Descriptor length
	USB_DESCRIPTOR_DEVICE,			// Descriptor type
	0x200,							// USB 2.0
	0x09,							// Class (9 = Hub)
	0,								// Subclass
	0,								// Protocol
	64,								// Max packet size on endpoint 0
	0,								// Vendor ID
	0,								// Product ID
	0x200,							// Version
	1,								// Index of manufacturer string
	2,								// Index of product string
	0,								// Index of serial number string
	1								// Number of configurations
};


struct ehci_root_hub_configuration_s {
	usb_configuration_descriptor	configuration;
	usb_interface_descriptor		interface;
	usb_endpoint_descriptor			endpoint;
	usb_hub_descriptor				hub;
} _PACKED;


static ehci_root_hub_configuration_s sEHCIRootHubConfig =
{
	{ // configuration descriptor
		9,								// Descriptor length
		USB_DESCRIPTOR_CONFIGURATION,	// Descriptor type
		34,								// Total length of configuration (including
										// interface, endpoint and hub descriptors)
		1,								// Number of interfaces
		1,								// Value of this configuration
		0,								// Index of configuration string
		0x40,							// Attributes (0x40 = self powered)
		0								// Max power (0, since self powered)
	},

	{ // interface descriptor
		9,								// Descriptor length
		USB_DESCRIPTOR_INTERFACE,		// Descriptor type
		0,								// Interface number
		0,								// Alternate setting
		1,								// Number of endpoints
		0x09,							// Interface class (9 = Hub)
		0,								// Interface subclass
		0,								// Interface protocol
		0,								// Index of interface string
	},

	{ // endpoint descriptor
		7,								// Descriptor length
		USB_DESCRIPTOR_ENDPOINT,		// Descriptor type
		USB_REQTYPE_DEVICE_IN | 1,		// Endpoint address (first in IN endpoint)
		0x03,							// Attributes (0x03 = interrupt endpoint)
		8,								// Max packet size
		0xFF							// Interval
	},

	{ // hub descriptor
		9,								// Descriptor length (including
										// deprecated power control mask)
		USB_DESCRIPTOR_HUB,				// Descriptor type
		2,								// Number of ports
		0x0000,							// Hub characteristics
		50,								// Power on to power good (in 2ms units)
		0,								// Maximum current (in mA)
		0x00,							// Both ports are removable
		0xff							// Depricated power control mask
	}
};


struct ehci_root_hub_string_s {
	uint8	length;
	uint8	descriptor_type;
	uint16	unicode_string[12];
} _PACKED;


static ehci_root_hub_string_s sEHCIRootHubStrings[3] = {
	{
		4,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		0x0409							// Supported language IDs (English US)
	},

	{
		12,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		'H', 'A', 'I', 'K', 'U', ' ',	// Characters
		'I', 'n', 'c', '.'
	},

	{
		26,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		'E', 'H', 'C', 'I', ' ', 'R',	// Characters
		'o', 'o', 't', 'H', 'u', 'b'
	}
};


EHCIRootHub::EHCIRootHub(EHCI *ehci, int8 deviceAddress)
	:	Hub(ehci, NULL, sEHCIRootHubDevice, deviceAddress, false)
{
	fEHCI = ehci;
}


status_t
EHCIRootHub::SubmitTransfer(Transfer *transfer)
{
	return B_ERROR;
}


void
EHCIRootHub::UpdatePortStatus()
{
	for (int32 i = 0; i < sEHCIRootHubConfig.hub.num_ports; i++) {
		fPortStatus[i].status = 0;
		fPortStatus[i].change = 0;
	}
}
