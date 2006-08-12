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

	for (int32 i = 0; i < 8; i++)
		fChildren[i] = NULL;

	if (fDeviceDescriptor.device_class != 9
		|| fDeviceDescriptor.device_subclass != 0
		|| fDeviceDescriptor.device_protocol != 0) {
		TRACE(("USB Hub: wrong class/subclass/protocol! Bailing out\n"));
		return;
	}

	if (Configuration()->descr->number_interfaces > 1) {
		TRACE(("USB Hub: too many interfaces\n"));
		return;
	}

	fInterruptInterface = Configuration()->interface->active->descr;
	if (fInterruptInterface->num_endpoints > 1) {
		TRACE(("USB Hub: too many endpoints\n"));
		return;
	}

	fInterruptEndpoint = Configuration()->interface->active->endpoint[0].descr;
	if (fInterruptEndpoint->attributes != 0x03) { // interrupt endpoint
		TRACE(("USB Hub: Not an interrupt endpoint\n"));
		return;
	}

	TRACE(("USB Hub: Getting hub descriptor...\n"));
	size_t actualLength;
	status_t status = GetDescriptor(USB_DESCRIPTOR_HUB, 0, 0,
		(void *)&fHubDescriptor, sizeof(usb_hub_descriptor), &actualLength);
	if (status < B_OK || actualLength != sizeof(usb_hub_descriptor)) {
		TRACE(("USB Hub: Error getting hub descriptor\n"));
		return;
	}

	// Enable port power on all ports
	for (int32 i = 0; i < fHubDescriptor.bNbrPorts; i++) {
		SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
			USB_REQUEST_SET_FEATURE,
			PORT_POWER, 
			i + 1,
			0,
			NULL,
			0,
			&actualLength);
	}

	// Wait for power to stabilize
	snooze(fHubDescriptor.bPwrOn2PwrGood * 2000);

	fInitOK = true;
	TRACE(("USB Hub: initialised ok\n"));
}


void
Hub::Explore()
{
	for (int32 i = 0; i < fHubDescriptor.bNbrPorts; i++) {
		size_t actualLength;

		// Get the current port status
		SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_IN,
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
			SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
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
					SendRequest(
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

				Device *newDevice = fBus->AllocateNewDevice(this,
					(fPortStatus[i].status & PORT_STATUS_LOW_SPEED) > 0);

				if (newDevice) {
					fChildren[i] = newDevice;
					Manager()->GetStack()->NotifyDeviceChange(fChildren[i], true);
				}
			} else {
				// Device removed...
				TRACE(("USB Hub Explore(): Device removed\n"));
				if (fChildren[i]) {
					Manager()->GetStack()->NotifyDeviceChange(fChildren[i], false);
					delete fChildren[i];
					fChildren[i] = NULL;
				}
			}

			// Clear status change
			SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
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


status_t
Hub::GetDescriptor(uint8 descriptorType, uint8 index, uint16 languageID,
	void *data, size_t dataLength, size_t *actualLength)
{
	return SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_CLASS,			// type
		USB_REQUEST_GET_DESCRIPTOR,							// request
		(descriptorType << 8) | index,						// value
		languageID,											// index
		dataLength,											// length										
		data,												// buffer
		dataLength,											// buffer length
		actualLength);										// actual length
}			


void
Hub::ReportDevice(usb_support_descriptor *supportDescriptors,
	uint32 supportDescriptorCount, const usb_notify_hooks *hooks, bool added)
{
	TRACE(("USB Hub ReportDevice\n"));

	// Report ourselfs first
	Device::ReportDevice(supportDescriptors, supportDescriptorCount, hooks, added);

	// Then report all of our children
	for (int32 i = 0; i < fHubDescriptor.bNbrPorts; i++) {
		if (!fChildren[i])
			continue;

		fChildren[i]->ReportDevice(supportDescriptors,
				supportDescriptorCount, hooks, added);
	}
}
