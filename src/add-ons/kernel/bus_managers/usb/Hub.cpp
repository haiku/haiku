/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "usb_p.h"


#define TRACE_HUB
#ifdef TRACE_HUB
#define TRACE(x)	dprintf	x
#else
#define TRACE(x)	/* nothing */
#endif


Hub::Hub(BusManager *bus, Device *parent, usb_device_descriptor &desc,
	int8 deviceAddress, bool lowSpeed)
	:	Device(bus, parent, desc, deviceAddress, lowSpeed)
{
	TRACE(("USB Hub is being initialised\n"));

	if (!fInitOK) {
		TRACE(("USB Hub: Device failed to initialize\n"));
		return;
	}

	// Set to false again for the hub init.
	fInitOK = false; 

	if (fDeviceDescriptor.device_subclass != 0
		|| fDeviceDescriptor.device_protocol != 0) {
		TRACE(("USB Hub: wrong class/subclass/protocol! Bailing out\n"));
		return;
	}

	if (fCurrentConfiguration->number_interfaces > 1) {
		TRACE(("USB Hub: too many interfaces\n"));
		return;
	}

	size_t actualLength;
	if (GetDescriptor(USB_DESCRIPTOR_INTERFACE, 0, (void *)&fInterruptInterface,
		sizeof(usb_interface_descriptor)) != sizeof(usb_interface_descriptor)) {
		TRACE(("USB Hub: error getting the interrupt interface\n"));
		return;
	}

	if (fInterruptInterface.num_endpoints > 1) {
		TRACE(("USB Hub: too many endpoints\n"));
		return;
	}

	if (GetDescriptor(USB_DESCRIPTOR_ENDPOINT, 0, (void *)&fInterruptEndpoint,
		sizeof(usb_endpoint_descriptor)) != sizeof(usb_endpoint_descriptor)) {
		TRACE(("USB Hub: Error getting the endpoint\n"));
		return;
	}

	if (fInterruptEndpoint.attributes != 0x03) { // interrupt transfer
		TRACE(("USB Hub: Not an interrupt endpoint\n"));
		return;
	}

	TRACE(("USB Hub: Getting hub descriptor...\n"));
	if (GetDescriptor(USB_DESCRIPTOR_HUB, 0, (void *)&fHubDescriptor,
		sizeof(usb_hub_descriptor)) != sizeof(usb_hub_descriptor)) {
		TRACE(("USB Hub: Error getting hub descriptor\n"));
		return;
	}

	// Enable port power on all ports
	for (int32 i = 0; i < fHubDescriptor.bNbrPorts; i++) {
		fDefaultPipe->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
			USB_REQUEST_SET_FEATURE,
			PORT_POWER, 
			i + 1,
			0,
			NULL,
			0,
			&actualLength);
	}

	// Wait for power to stabilize
	snooze(fHubDescriptor.bPwrOn2PwrGood * 2);

	fInitOK = true;
	TRACE(("USB Hub: initialised ok\n"));
}


void
Hub::Explore()
{
	for (int32 i = 0; i < fHubDescriptor.bNbrPorts; i++) {
		size_t actualLength;

		// Get the current port status
		fDefaultPipe->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_IN,
			USB_REQUEST_GET_STATUS,
			0,
			i + 1,
			4,
			(void *)&fPortStatus[i],
			4,
			&actualLength);

		if (actualLength < 4) {
			TRACE(("USB Hub: error getting port status\n"));
			return;
		}

		//TRACE(("status: 0x%04x; change: 0x%04x\n", fPortStatus[i].status, fPortStatus[i].change));

		// We need to test the port change against a number of things
		if (fPortStatus[i].status & PORT_RESET) {
			fDefaultPipe->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE,
				PORT_RESET,
				i + 1,
				0,
				NULL,
				0,
				&actualLength);
		}

		if (fPortStatus[i].change & PORT_STATUS_CONNECTION) {
			if (fPortStatus[i].status & PORT_STATUS_CONNECTION) {
				// New device attached!

				if ((fPortStatus[i].status & PORT_STATUS_ENABLE) == 0) {
					// enable the port if it isn't
					fDefaultPipe->SendRequest(
						USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
						USB_REQUEST_SET_FEATURE,
						PORT_ENABLE, 
						i + 1,
						0,
						NULL,
						0,
						NULL);
		        }

				TRACE(("USB Hub: Explore(): New device connected\n"));

				Device *newDevice;
				if (fPortStatus[i].status & PORT_STATUS_LOW_SPEED)
					newDevice = fBus->AllocateNewDevice(this, true);
				else
					newDevice = fBus->AllocateNewDevice(this, false);

				if (newDevice)
					fChildren[i] = newDevice;
			} else {
				// Device removed...
				// ToDo: do something
				TRACE(("USB Hub Explore(): Device removed\n"));
			}

			// Clear status change
			fDefaultPipe->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE,
				C_PORT_CONNECTION, 
				i + 1,
				0,
				NULL,
				0,
				NULL);
		}
	}
}
