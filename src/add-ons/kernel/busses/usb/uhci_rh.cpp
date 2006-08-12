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


//#define TRACE_UHCI_ROOT_HUB
#ifdef TRACE_UHCI_ROOT_HUB
#define TRACE(x) dprintf x
#else
#define TRACE(x) /* nothing */
#endif


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
};


static uhci_root_hub_string_s sUHCIRootHubStrings[3] = {
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
		'U', 'H', 'C', 'I', ' ', 'R',	// Characters
		'o', 'o', 't', 'H', 'u', 'b'
	}
};


UHCIRootHub::UHCIRootHub(UHCI *uhci, int8 devicenum)
	:	Hub(uhci, NULL, sUHCIRootHubDevice, devicenum, false)
{
	fUHCI = uhci;
}


status_t
UHCIRootHub::SubmitTransfer(Transfer *transfer)
{
	usb_request_data *request = transfer->RequestData();
	TRACE(("usb_uhci_roothub: rh_submit_packet called. request: %u\n", request->Request));

	status_t result = B_ERROR;
	switch (request->Request) {
		case RH_GET_STATUS:
			if (request->Index == 0) {
				// Get the hub status -- everything as 0 means all-right
				memset(transfer->Data(), 0, sizeof(get_status_buffer));
				result = B_OK;
				break;
			} else if (request->Index > sUHCIRootHubConfig.hub.bNbrPorts) {
				// This port doesn't exist
				result = EINVAL;
				break;
			}

			// Get port status
			UpdatePortStatus();
			memcpy(transfer->Data(),
				(void *)&fPortStatus[request->Index - 1],
				transfer->DataLength());

			*(transfer->ActualLength()) = transfer->DataLength();
			result = B_OK;
			break;

		case RH_SET_ADDRESS:
			if (request->Value >= 128) {
				result = EINVAL;
				break;
			}

			TRACE(("usb_uhci_roothub: rh_submit_packet RH_ADDRESS: %d\n", request->Value));
			result = B_OK;
			break;

		case RH_GET_DESCRIPTOR:
			TRACE(("usb_uhci_roothub: rh_submit_packet GET_DESC: %d\n", request->Value));

			switch (request->Value & 0xff00) {
				case RH_DEVICE_DESCRIPTOR: {
					size_t length = MIN(sizeof(usb_device_descriptor),
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubDevice,
						length);
					*(transfer->ActualLength()) = length; 
					result = B_OK;
					break;
				}

				case RH_CONFIG_DESCRIPTOR: {
					size_t length = MIN(sizeof(uhci_root_hub_configuration_s),
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubConfig,
						length);
					*(transfer->ActualLength()) = length;
					result =  B_OK;
					break;
				}

				case RH_STRING_DESCRIPTOR: {
					uint8 index = request->Value & 0x00ff;
					if (index > 2) {
						*(transfer->ActualLength()) = 0;
						result = EINVAL;
						break;
					}

					size_t length = MIN(sUHCIRootHubStrings[index].length,
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubStrings[index],
						length);
					*(transfer->ActualLength()) = length;
					result = B_OK;
					break;
				}

				case RH_HUB_DESCRIPTOR: {
					size_t length = MIN(sizeof(usb_hub_descriptor),
						transfer->DataLength());
					memcpy(transfer->Data(), (void *)&sUHCIRootHubConfig.hub,
						length);
					*(transfer->ActualLength()) = length;
					result = B_OK;
					break;
				}

				default:
					result = EINVAL;
					break;
			}
			break;

		case RH_SET_CONFIG:
			result = B_OK;
			break;

		case RH_CLEAR_FEATURE: {
			if (request->Index == 0) {
				// We don't support any hub changes
				TRACE(("usb_uhci_roothub: RH_CLEAR_FEATURE no hub changes!\n"));
				result = EINVAL;
				break;
			} else if (request->Index > sUHCIRootHubConfig.hub.bNbrPorts) {
				// Invalid port number
				TRACE(("usb_uhci_roothub: RH_CLEAR_FEATURE invalid port!\n"));
				result = EINVAL;
				break;
			}

			TRACE(("usb_uhci_roothub: RH_CLEAR_FEATURE called. Feature: %u!\n", request->Value));
			uint16 status;
			switch(request->Value) {
				case PORT_RESET:
					status = fUHCI->PortStatus(request->Index - 1);
					result = fUHCI->SetPortStatus(request->Index - 1,
						status & UHCI_PORTSC_DATAMASK & ~UHCI_PORTSC_RESET);
					break;

				case C_PORT_CONNECTION:
					status = fUHCI->PortStatus(request->Index - 1);
					result = fUHCI->SetPortStatus(request->Index - 1,
						(status & UHCI_PORTSC_DATAMASK) | UHCI_PORTSC_STATCHA);
					break;
				default:
					result = EINVAL;
					break;
			}
			break;
		}

		case RH_SET_FEATURE: {
			if (request->Index == 0) {
				// We don't support any hub changes
				TRACE(("usb_uhci_roothub: RH_SET_FEATURE no hub changes!\n"));
				result = EINVAL;
				break;
			} else if (request->Index > sUHCIRootHubConfig.hub.bNbrPorts) {
				// Invalid port number
				TRACE(("usb_uhci_roothub: RH_SET_FEATURE invalid port!\n"));
				result = EINVAL;
				break;
			}

			TRACE(("usb_uhci_roothub: RH_SET_FEATURE called. Feature: %u!\n", request->Value));
			uint16 status;
			switch(request->Value) {
				case PORT_RESET:
					result = fUHCI->ResetPort(request->Index - 1);
					break;

				case PORT_ENABLE:
					status = fUHCI->PortStatus(request->Index - 1);
					result = fUHCI->SetPortStatus(request->Index - 1,
						(status & UHCI_PORTSC_DATAMASK) | UHCI_PORTSC_ENABLED);
					break;
				default:
					result = EINVAL;
					break;
			}
			break;
		}

		default: 
			result = EINVAL;
			break;
	}

	transfer->Finished(result);
	return result;
}


void
UHCIRootHub::UpdatePortStatus()
{
	for (int32 i = 0; i < sUHCIRootHubConfig.hub.bNbrPorts; i++) {
		uint16 newStatus = 0;
		uint16 newChange = 0;

		uint16 portStatus = fUHCI->PortStatus(i);
		TRACE(("usb_uhci_roothub: port: %d status: 0x%04x\n", i, portStatus));

		// Set all individual bits
		if (portStatus & UHCI_PORTSC_CURSTAT)
			newStatus |= PORT_STATUS_CONNECTION;

		if (portStatus & UHCI_PORTSC_STATCHA)
			newChange |= PORT_STATUS_CONNECTION;

		if (portStatus & UHCI_PORTSC_ENABLED)
			newStatus |= PORT_STATUS_ENABLE;

		if (portStatus & UHCI_PORTSC_ENABCHA)
			newChange |= PORT_STATUS_ENABLE;

		// TODO: work out suspended/resume

		if (portStatus & UHCI_PORTSC_RESET)
			newStatus |= PORT_STATUS_RESET;

		//TODO: work out reset change...

		//The port is automagically powered on
		newStatus |= PORT_POWER;

		if (portStatus & UHCI_PORTSC_LOWSPEED)
			newStatus |= PORT_STATUS_LOW_SPEED;

		//Update the stored port status
		fPortStatus[i].status = newStatus;
		fPortStatus[i].change = newChange;
	}
}
