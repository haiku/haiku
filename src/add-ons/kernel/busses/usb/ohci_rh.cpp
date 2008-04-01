/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *             Jan-Rixt Van Hoye
 *             Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include "ohci.h"

static usb_device_descriptor sOHCIRootHubDevice =
{
	0x12,					// Descriptor size
	USB_DESCRIPTOR_DEVICE,	// Type of descriptor
	0x110,					// USB 1.1
	0x09,					// Hub type
	0,						// Subclass
	0,						// Protocol
	64,						// Max packet size
	0,						// Vendor
	0,						// Product
	0x110,					// Version
	1,						// Index of manufacture string
	2,						// Index of product string
	0,						// Index of serial number string
	1						// Number of configurations
};


struct ohci_root_hub_configuration_s {
	usb_configuration_descriptor	configuration;
	usb_interface_descriptor		interface;
	usb_endpoint_descriptor			endpoint;
	usb_hub_descriptor				hub;
} _PACKED;


static ohci_root_hub_configuration_s sOHCIRootHubConfig =
{
	{ // configuration descriptor
		9,								// Descriptor length
		USB_DESCRIPTOR_CONFIGURATION,	// Descriptor type
		34,								// Total size of the configuration
		1,								// Number interfaces
		1,								// Value of configuration
		0,								// Number of configuration
		0x40,							// Self powered
		0								// Max power (0, because of self power)
	},

	{ // interface descriptor
		9,								// Size
		USB_DESCRIPTOR_INTERFACE,		// Type
		0,								// Interface number
		0,								// Alternate setting
		1,								// Num endpoints
		0x09,							// Interface class
		0,								// Interface subclass
		0,								// Interface protocol
		0								// Interface
	},

	{ // endpoint descriptor
		7,								// Size
		USB_DESCRIPTOR_ENDPOINT,		// Type
		USB_REQTYPE_DEVICE_IN | 1, 		// Endpoint address (first in IN endpoint)
		0x03,							// Attributes (0x03 = interrupt endpoint)
		8,								// Max packet size
		0xFF							// Interval 256
	},
	
	{ // hub descriptor	
		9,						// Lenght (including deprecated power
								// control mask)
		USB_DESCRIPTOR_HUB,		// Type
		0,						// Number of ports
		0x0000,					// Hub characteristics
		0,						// Power on to power good
		0,						// Current
		0x00,					// Both ports are removable
		0xff					// Depricated power control mask
	}
};

struct ohci_root_hub_string_s {
	uint8	length;
	uint8	descriptor_type;
	uint16	unicode_string[12];
} _PACKED;


static ohci_root_hub_string_s sOHCIRootHubStrings[3] = {
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
			'O', 'H', 'C', 'I', ' ',	// Characters
			'R', 'o', 'o', 't', 'H',
			'u', 'b'
		}
	}
};


OHCIRootHub::OHCIRootHub(Object *rootObject, int8 deviceAddress)
   :	Hub(rootObject, rootObject->GetStack()->IndexOfBusManager(rootObject->GetBusManager()),
   			sOHCIRootHubDevice, deviceAddress, USB_SPEED_FULLSPEED, true)
{
}

status_t
OHCIRootHub::ProcessTransfer(OHCI *ohci, Transfer *transfer)
{
	if ((transfer->TransferPipe()->Type() & USB_OBJECT_CONTROL_PIPE) == 0)
		return B_ERROR;

	usb_request_data *request = transfer->RequestData();

	TRACE(("usb_ohci_roothub(): request: %d\n", request->Request));

	status_t status = B_TIMED_OUT;
	size_t actualLength = 0;
	switch (request->Request) {
		case USB_REQUEST_GET_STATUS: {
			if (request->Index == 0) {
				// get hub status
				actualLength = MIN(sizeof(usb_port_status),
					transfer->DataLength());
				// the hub reports whether the local power failed (bit 0)
				// and if there is a over-current condition (bit 1).
				// everything as 0 means all is ok.
				// TODO (?) actually check for the value
				memset(transfer->Data(), 0, actualLength);
				status = B_OK;
				break;
			}

			usb_port_status portStatus;
			if (ohci->GetPortStatus(request->Index - 1, &portStatus) >= B_OK) {
				actualLength = MIN(sizeof(usb_port_status), transfer->DataLength());
				memcpy(transfer->Data(), (void *)&portStatus, actualLength);
				status = B_OK;
			}
			break;
		}

		case USB_REQUEST_SET_ADDRESS:
			if (request->Value >= 128) {
				status = B_TIMED_OUT;
				break;
			}

			TRACE(("usb_ohci_roothub():  set address: %d\n", request->Value));
			status = B_OK;
			break;

		case USB_REQUEST_GET_DESCRIPTOR:
			TRACE(("usb_ohci_roothub(): get descriptor: %d\n", request->Value >> 8));

			switch (request->Value >> 8) {
				case USB_DESCRIPTOR_DEVICE: {
					actualLength = MIN(sizeof(usb_device_descriptor),
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sOHCIRootHubDevice,
						actualLength);
					status = B_OK;
					break;
				}

				case USB_DESCRIPTOR_CONFIGURATION: {
					actualLength = MIN(sizeof(ohci_root_hub_configuration_s),
						transfer->DataLength());
					sOHCIRootHubConfig.hub.num_ports = ohci->PortCount();
					memcpy(transfer->Data(), (void *)&sOHCIRootHubConfig,
						actualLength);
					status = B_OK;
					break;
				}

				case USB_DESCRIPTOR_STRING: {
					uint8 index = request->Value & 0x00ff;
					if (index > 2)
						break;

					actualLength = MIN(sOHCIRootHubStrings[index].length,
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sOHCIRootHubStrings[index],
						actualLength);
					status = B_OK;
					break;
				}

				case USB_DESCRIPTOR_HUB: {
					actualLength = MIN(sizeof(usb_hub_descriptor),
						transfer->DataLength());
					sOHCIRootHubConfig.hub.num_ports = ohci->PortCount();
					memcpy(transfer->Data(), (void *)&sOHCIRootHubConfig.hub,
						actualLength);
					status = B_OK;
					break;
				}
			}
			break;

		case USB_REQUEST_SET_CONFIGURATION:
			status = B_OK;
			break;

		case USB_REQUEST_CLEAR_FEATURE: {
			if (request->Index == 0) {
				// we don't support any hub changes
				TRACE_ERROR(("usb_ohci_roothub(): clear feature: no hub changes\n"));
				break;
			}

			TRACE(("usb_ohci_roothub(): clear feature: %d\n", request->Value));
			if (ohci->ClearPortFeature(request->Index - 1, request->Value) >= B_OK)
				status = B_OK;
			break;
		}

		case USB_REQUEST_SET_FEATURE: {
			if (request->Index == 0) {
				// we don't support any hub changes
				TRACE_ERROR(("usb_ohci_roothub(): set feature: no hub changes\n"));
				break;
			}

			TRACE(("usb_ohci_roothub(): set feature: %d\n", request->Value));
			if (ohci->SetPortFeature(request->Index - 1, request->Value) >= B_OK)
				status = B_OK;
			break;
		}
	}

	transfer->Finished(status, actualLength);
	delete transfer;
	return B_OK;
}
