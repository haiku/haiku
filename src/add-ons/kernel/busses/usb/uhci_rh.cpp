/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "uhci.h"
#include <PCI.h>


static usb_device_descriptor sUHCIRootHubDevice =
{
	18,								// Descriptor length
	USB_DESCRIPTOR_DEVICE,			// Descriptor type
	0x110,							// USB 1.1
	0x09,							// Class (9 = Hub)
	0,								// Subclass
	0,								// Protocol
	64,								// Max packet size on endpoint 0
	0,								// Vendor ID
	0,								// Product ID
	0x110,							// Version
	1,								// Index of manufacturer string
	2,								// Index of product string
	0,								// Index of serial number string
	1								// Number of configurations
};


struct uhci_root_hub_configuration_s {
	usb_configuration_descriptor	configuration;
	usb_interface_descriptor		interface;
	usb_endpoint_descriptor			endpoint;
	usb_hub_descriptor				hub;
} _PACKED;


static uhci_root_hub_configuration_s sUHCIRootHubConfig =
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


struct uhci_root_hub_string_s {
	uint8	length;
	uint8	descriptor_type;
	uint16	unicode_string[12];
} _PACKED;


static uhci_root_hub_string_s sUHCIRootHubStrings[3] = {
	{
		4,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		{
			0x0409						// Supported language IDs (English US)
		}
	},

	{
		22,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		{
			'H', 'A', 'I', 'K', 'U',	// Characters
			' ', 'I', 'n', 'c', '.'
		}

	},

	{
		26,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		{
			'U', 'H', 'C', 'I', ' ',	// Characters
			'R', 'o', 'o', 't', 'H',
			'u', 'b'
		}
	}
};


UHCIRootHub::UHCIRootHub(Object *rootObject, int8 deviceAddress)
	:	Hub(rootObject, sUHCIRootHubDevice, deviceAddress, USB_SPEED_FULLSPEED)
{
}


status_t
UHCIRootHub::ProcessTransfer(UHCI *uhci, Transfer *transfer)
{
	if ((transfer->TransferPipe()->Type() & USB_OBJECT_CONTROL_PIPE) == 0)
		return B_ERROR;

	usb_request_data *request = transfer->RequestData();
	TRACE(("usb_uhci_roothub: request: %d\n", request->Request));

	// ToDo: define better status codes. We should return a request error.
	uint32 status = B_USB_STATUS_DEVICE_TIMEOUT;
	size_t actualLength = 0;
	switch (request->Request) {
		case USB_REQUEST_GET_STATUS: {
			if (request->Index == 0) {
				// Get the hub status -- everything as 0 means all-right
				actualLength = MIN(sizeof(usb_port_status),
					transfer->DataLength());
				memset(transfer->Data(), 0, actualLength);
				status = B_USB_STATUS_SUCCESS;
				break;
			}

			usb_port_status portStatus;
			if (uhci->GetPortStatus(request->Index - 1, &portStatus) >= B_OK) {
				actualLength = MIN(sizeof(usb_port_status), transfer->DataLength());
				memcpy(transfer->Data(), (void *)&portStatus, actualLength);
				status = B_USB_STATUS_SUCCESS;
			}
			break;
		}

		case USB_REQUEST_SET_ADDRESS:
			if (request->Value >= 128) {
				status = B_USB_STATUS_DEVICE_TIMEOUT;
				break;
			}

			TRACE(("usb_uhci_roothub: set address: %d\n", request->Value));
			status = B_USB_STATUS_SUCCESS;
			break;

		case USB_REQUEST_GET_DESCRIPTOR:
			TRACE(("usb_uhci_roothub: get descriptor: %d\n", request->Value >> 8));

			switch (request->Value >> 8) {
				case USB_DESCRIPTOR_DEVICE: {
					actualLength = MIN(sizeof(usb_device_descriptor),
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubDevice,
						actualLength);
					status = B_USB_STATUS_SUCCESS;
					break;
				}

				case USB_DESCRIPTOR_CONFIGURATION: {
					actualLength = MIN(sizeof(uhci_root_hub_configuration_s),
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubConfig,
						actualLength);
					status = B_USB_STATUS_SUCCESS;
					break;
				}

				case USB_DESCRIPTOR_STRING: {
					uint8 index = request->Value & 0x00ff;
					if (index > 2)
						break;

					actualLength = MIN(sUHCIRootHubStrings[index].length,
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubStrings[index],
						actualLength);
					status = B_USB_STATUS_SUCCESS;
					break;
				}

				case USB_DESCRIPTOR_HUB: {
					actualLength = MIN(sizeof(usb_hub_descriptor),
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubConfig.hub,
						actualLength);
					status = B_USB_STATUS_SUCCESS;
					break;
				}
			}
			break;

		case USB_REQUEST_SET_CONFIGURATION:
			status = B_USB_STATUS_SUCCESS;
			break;

		case USB_REQUEST_CLEAR_FEATURE: {
			if (request->Index == 0) {
				// We don't support any hub changes
				TRACE_ERROR(("usb_uhci_roothub: clear feature: no hub changes\n"));
				break;
			}

			TRACE(("usb_uhci_roothub: clear feature: %d\n", request->Value));
			if (uhci->ClearPortFeature(request->Index - 1, request->Value) >= B_OK)
				status = B_USB_STATUS_SUCCESS;
			break;
		}

		case USB_REQUEST_SET_FEATURE: {
			if (request->Index == 0) {
				// we don't support any hub changes
				TRACE_ERROR(("usb_uhci_roothub: set feature: no hub changes\n"));
				break;
			}

			TRACE(("usb_uhci_roothub: set feature: %d!\n", request->Value));
			if (uhci->SetPortFeature(request->Index - 1, request->Value) >= B_OK)
				status = B_USB_STATUS_SUCCESS;
			break;
		}
	}

	transfer->Finished(status, actualLength);
	delete transfer;
	return B_OK;
}
