/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "uhci.h"
#include <PCI.h>


#define TRACE_UHCI_ROOT_HUB
#ifdef TRACE_UHCI_ROOT_HUB
#define TRACE(x) dprintf x
#else
#define TRACE(x) /* nothing */
#endif


usb_device_descriptor uhci_devd =
{
	0x12,					//Descriptor size
	USB_DESCRIPTOR_DEVICE,	//Type of descriptor
	0x110,					//USB 1.1
	0x09,					//Hub type
	0,						//Subclass
	0,						//Protocol
	64,						//Max packet size
	0,						//Vendor
	0,						//Product
	0x110,					//Version
	1, 2, 0,				//Other data
	1						//Number of configurations
};


usb_configuration_descriptor uhci_confd =
{
	0x09,					//Size
	USB_DESCRIPTOR_CONFIGURATION,
	25,						//Total size (taken from BSD source)
	1,						//Number interfaces
	1,						//Value of configuration
	0,						//Number of configuration
	0x40,					//Self powered
	0						//Max power (0, because of self power)
};


usb_interface_descriptor uhci_intd =
{
	0x09,					//Size
	USB_DESCRIPTOR_INTERFACE,
	0,						//Interface number
	0,						//Alternate setting
	1,						//Num endpoints
	0x09,					//Interface class
	0,						//Interface subclass
	0,						//Interface protocol
	0,						//Interface
};


usb_endpoint_descriptor uhci_endd =
{
	0x07,					//Size
	USB_DESCRIPTOR_ENDPOINT,
	USB_REQTYPE_DEVICE_IN | 1, //1 from freebsd driver
	0x3,					// Interrupt
	8,						// Max packet size
	0xFF					// Interval 256
};


usb_hub_descriptor uhci_hubd =
{
	0x09,					//Including deprecated powerctrlmask
	USB_DESCRIPTOR_HUB,
	2,						//Number of ports
	0x02 | 0x01,			//Hub characteristics FIXME
	50,						//Power on to power good
	0,						// Current
	0x00					//Both ports are removable
};


UHCIRootHub::UHCIRootHub(UHCI *uhci, int8 devicenum)
	:	Hub(uhci, NULL, uhci_devd, devicenum, false)
{
	fUHCI = uhci;
}


status_t
UHCIRootHub::SubmitTransfer(Transfer *transfer)
{
	usb_request_data *request = transfer->RequestData();
	TRACE(("usb_uhci_roothub: rh_submit_packet called. request: %u\n", request->Request));

	status_t result = B_ERROR;
	switch(request->Request) {
		case RH_GET_STATUS:
			if (request->Index == 0) {
				// Get the hub status -- everything as 0 means all-right
				memset(transfer->Buffer(), 0, sizeof(get_status_buffer));
				result = B_OK;
				break;
			} else if (request->Index > uhci_hubd.bNbrPorts) {
				// This port doesn't exist
				result = EINVAL;
				break;
			}

			// Get port status
			UpdatePortStatus();
			memcpy(transfer->Buffer(),
				(void *)&fPortStatus[request->Index - 1],
				transfer->BufferLength());

			*(transfer->ActualLength()) = transfer->BufferLength();
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

			switch (request->Value) {
				case RH_DEVICE_DESCRIPTOR:
					memcpy(transfer->Buffer(), (void *)&uhci_devd,
						transfer->BufferLength());
					*(transfer->ActualLength()) = transfer->BufferLength(); 
					result = B_OK;
					break;
				case RH_CONFIG_DESCRIPTOR:
					memcpy(transfer->Buffer(), (void *)&uhci_confd,
						transfer->BufferLength());
					*(transfer->ActualLength()) = transfer->BufferLength();
					result =  B_OK;
					break;
				case RH_INTERFACE_DESCRIPTOR:
					memcpy(transfer->Buffer(), (void *)&uhci_intd,
						transfer->BufferLength());
					*(transfer->ActualLength()) = transfer->BufferLength();
					result = B_OK ;
					break;
				case RH_ENDPOINT_DESCRIPTOR:
					memcpy(transfer->Buffer(), (void *)&uhci_endd,
						transfer->BufferLength());
					*(transfer->ActualLength()) = transfer->BufferLength();
					result = B_OK ;
					break;
				case RH_HUB_DESCRIPTOR:
					memcpy(transfer->Buffer(), (void *)&uhci_hubd,
						transfer->BufferLength());
					*(transfer->ActualLength()) = transfer->BufferLength();
					result = B_OK;
					break;
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
			} else if (request->Index > uhci_hubd.bNbrPorts) {
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
			} else if (request->Index > uhci_hubd.bNbrPorts) {
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

	// Clean up the transfer - we own it, so we clean it up
	transfer->Finish();
	delete transfer;
	return result;
}


void
UHCIRootHub::UpdatePortStatus()
{
	for (int32 i = 0; i < uhci_hubd.bNbrPorts; i++) {
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
