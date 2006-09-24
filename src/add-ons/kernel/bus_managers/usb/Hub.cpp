/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"
#include <stdio.h>


Hub::Hub(Object *parent, usb_device_descriptor &desc, int8 deviceAddress,
	usb_speed speed)
	:	Device(parent, desc, deviceAddress, speed)
{
	TRACE(("USB Hub is being initialised\n"));

	if (!fInitOK) {
		TRACE_ERROR(("USB Hub: Device failed to initialize\n"));
		return;
	}

	// Set to false again for the hub init.
	fInitOK = false;

	for (int32 i = 0; i < 8; i++)
		fChildren[i] = NULL;

	if (fDeviceDescriptor.device_class != 9) {
		TRACE_ERROR(("USB Hub: wrong class! Bailing out\n"));
		return;
	}

	TRACE(("USB Hub: Getting hub descriptor...\n"));
	size_t actualLength;
	status_t status = GetDescriptor(USB_DESCRIPTOR_HUB, 0, 0,
		(void *)&fHubDescriptor, sizeof(usb_hub_descriptor), &actualLength);

	// we need at least 8 bytes
	if (status < B_OK || actualLength < 8) {
		TRACE_ERROR(("USB Hub: Error getting hub descriptor\n"));
		return;
	}

	TRACE(("USB Hub: Hub descriptor (%d bytes):\n", actualLength));
	TRACE(("\tlength:..............%d\n", fHubDescriptor.length));
	TRACE(("\tdescriptor_type:.....0x%02x\n", fHubDescriptor.descriptor_type));
	TRACE(("\tnum_ports:...........%d\n", fHubDescriptor.num_ports));
	TRACE(("\tcharacteristics:.....0x%04x\n", fHubDescriptor.characteristics));
	TRACE(("\tpower_on_to_power_g:.%d\n", fHubDescriptor.power_on_to_power_good));
	TRACE(("\tdevice_removeable:...0x%02x\n", fHubDescriptor.device_removeable));
	TRACE(("\tpower_control_mask:..0x%02x\n", fHubDescriptor.power_control_mask));

	Object *object = GetStack()->GetObject(Configuration()->interface->active->endpoint[0].handle);
	if (!object || (object->Type() & USB_OBJECT_INTERRUPT_PIPE) == 0) {
		TRACE_ERROR(("USB Hub: no interrupt pipe found\n"));
		return;
	}

	fInterruptPipe = (InterruptPipe *)object;
	fInterruptPipe->QueueInterrupt(fInterruptStatus, sizeof(fInterruptStatus),
		InterruptCallback, this);

	// Wait some time before powering up the ports
	snooze(USB_DELAY_HUB_POWER_UP);

	// Enable port power on all ports
	for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
		status = DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
			USB_REQUEST_SET_FEATURE, PORT_POWER, i + 1, 0, NULL, 0, NULL);

		if (status < B_OK)
			TRACE_ERROR(("USB Hub: power up failed on port %ld\n", i));
	}

	// Wait for power to stabilize
	snooze(fHubDescriptor.power_on_to_power_good * 2000);

	fInitOK = true;
	TRACE(("USB Hub: initialised ok\n"));
}


status_t
Hub::UpdatePortStatus(uint8 index)
{
	// get the current port status
	size_t actualLength = 0;
	status_t result = DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_IN,
		USB_REQUEST_GET_STATUS, 0, index + 1, 4, (void *)&fPortStatus[index],
		4, &actualLength);

	if (result < B_OK || actualLength < 4) {
		TRACE_ERROR(("USB Hub: error updating port status\n"));
		return B_ERROR;
	}

	return B_OK;
}


status_t
Hub::ResetPort(uint8 index)
{
	status_t result = DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
		USB_REQUEST_SET_FEATURE, PORT_RESET, index + 1, 0, NULL, 0, NULL);

	if (result < B_OK)
		return result;

	for (int32 i = 0; i < 10; i++) {
		snooze(USB_DELAY_PORT_RESET);

		result = UpdatePortStatus(index);
		if (result < B_OK)
			return result;

		if ((fPortStatus[index].status & PORT_STATUS_CONNECTION) == 0) {
			// device disappeared, this is no error
			TRACE(("USB Hub: device disappeared on reset\n"));
			return B_OK;
		}

		if (fPortStatus[index].change & C_PORT_RESET) {
			// reset is done
			break;
		}
	}

	if ((fPortStatus[index].change & C_PORT_RESET) == 0) {
		TRACE_ERROR(("USB Hub: port %d won't reset\n", index));
		return B_ERROR;
	}

	// clear the reset change
	result = DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
		USB_REQUEST_CLEAR_FEATURE, C_PORT_RESET, index + 1, 0, NULL, 0, NULL);
	if (result < B_OK)
		return result;

	// wait for reset recovery
	snooze(USB_DELAY_PORT_RESET_RECOVERY);
	TRACE(("USB Hub: port %d was reset successfully\n", index));
	return B_OK;
}


void
Hub::Explore()
{
	for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
		status_t result = UpdatePortStatus(i);
		if (result < B_OK)
			continue;

#ifdef TRACE_USB
		if (fPortStatus[i].change)
			TRACE(("USB Hub: port %d: status: 0x%04x; change: 0x%04x\n", i, fPortStatus[i].status, fPortStatus[i].change));
#endif

		if (fPortStatus[i].change & PORT_STATUS_CONNECTION) {
			// clear status change
			DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE, C_PORT_CONNECTION, i + 1,
				0, NULL, 0, NULL);

			if (fPortStatus[i].status & PORT_STATUS_CONNECTION) {
				// new device attached!
				TRACE(("USB Hub: Explore(): New device connected\n"));

				// wait some time for the device to power up
				snooze(USB_DELAY_DEVICE_POWER_UP);

				// reset the port, this will also enable it
				result = ResetPort(i);
				if (result < B_OK)
					continue;

				result = UpdatePortStatus(i);
				if (result < B_OK)
					continue;

				if ((fPortStatus[i].status & PORT_STATUS_CONNECTION) == 0) {
					// device has vanished after reset, ignore
					continue;
				}

				usb_speed speed = USB_SPEED_FULLSPEED;
				if (fPortStatus[i].status & PORT_STATUS_LOW_SPEED)
					speed = USB_SPEED_LOWSPEED;
				if (fPortStatus[i].status & PORT_STATUS_HIGH_SPEED)
					speed = USB_SPEED_HIGHSPEED;

				Device *newDevice = GetBusManager()->AllocateNewDevice(this,
					speed);

				if (newDevice) {
					fChildren[i] = newDevice;
					GetStack()->NotifyDeviceChange(fChildren[i], true);
				} else {
					// the device failed to setup correctly, disable the port
					// so that the device doesn't get in the way of future
					// addressing.
					DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
						USB_REQUEST_CLEAR_FEATURE, PORT_ENABLE, i + 1,
						0, NULL, 0, NULL);
				}
			} else {
				// Device removed...
				TRACE(("USB Hub Explore(): Device removed\n"));
				if (fChildren[i]) {
					GetStack()->NotifyDeviceChange(fChildren[i], false);
					delete fChildren[i];
					fChildren[i] = NULL;
				}
			}
		}
	}

	// explore down the tree if we have hubs connected
	for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
		if (!fChildren[i] || (fChildren[i]->Type() & USB_OBJECT_HUB) == 0)
			continue;

		((Hub *)fChildren[i])->Explore();
	}
}


void
Hub::InterruptCallback(void *cookie, uint32 status, void *data,
	size_t actualLength)
{
	TRACE(("USB Hub: interrupt callback!\n"));
}


status_t
Hub::GetDescriptor(uint8 descriptorType, uint8 index, uint16 languageID,
	void *data, size_t dataLength, size_t *actualLength)
{
	return DefaultPipe()->SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_CLASS,			// type
		USB_REQUEST_GET_DESCRIPTOR,							// request
		(descriptorType << 8) | index,						// value
		languageID,											// index
		dataLength,											// length										
		data,												// buffer
		dataLength,											// buffer length
		actualLength);										// actual length
}			


status_t
Hub::ReportDevice(usb_support_descriptor *supportDescriptors,
	uint32 supportDescriptorCount, const usb_notify_hooks *hooks,
	usb_driver_cookie **cookies, bool added)
{
	TRACE(("USB Hub ReportDevice\n"));

	// Report ourselfs first
	status_t result = Device::ReportDevice(supportDescriptors,
		supportDescriptorCount, hooks, cookies, added);

	// Then report all of our children
	for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
		if (!fChildren[i])
			continue;

		if (fChildren[i]->ReportDevice(supportDescriptors,
				supportDescriptorCount, hooks, cookies, added) == B_OK)
			result = B_OK;
	}

	return result;
}


status_t
Hub::BuildDeviceName(char *string, uint32 *index, size_t bufferSize,
	Device *device)
{
	status_t result = Device::BuildDeviceName(string, index, bufferSize, device);
	if (result < B_OK) {
		// recursion to parent failed, we're at the root(hub)
		int32 managerIndex = GetStack()->IndexOfBusManager(GetBusManager());
		*index += snprintf(string + *index, bufferSize - *index, "%ld", managerIndex);
	}

	if (!device) {
		// no device was specified - report the hub
		*index += snprintf(string + *index, bufferSize - *index, "/hub");
	} else {
		// find out where the requested device sitts
		for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
			if (fChildren[i] == device) {
				*index += snprintf(string + *index, bufferSize - *index, "/%ld", i);
				break;
			}
		}
	}

	return B_OK;
}
