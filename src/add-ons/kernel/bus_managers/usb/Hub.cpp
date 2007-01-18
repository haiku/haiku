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
	TRACE(("USB Hub %d: creating hub\n", DeviceAddress()));

	if (!fInitOK) {
		TRACE_ERROR(("USB Hub %d: device failed to initialize\n", DeviceAddress()));
		return;
	}

	// Set to false again for the hub init.
	fInitOK = false;

	if (benaphore_init(&fLock, "usb hub lock") < B_OK) {
		TRACE_ERROR(("USB Hub %d: failed to create hub lock\n", DeviceAddress()));
		return;
	}

	for (int32 i = 0; i < 8; i++)
		fChildren[i] = NULL;

	if (fDeviceDescriptor.device_class != 9) {
		TRACE_ERROR(("USB Hub %d: wrong class! bailing out\n", DeviceAddress()));
		return;
	}

	TRACE(("USB Hub %d: Getting hub descriptor...\n", DeviceAddress()));
	size_t actualLength;
	status_t status = GetDescriptor(USB_DESCRIPTOR_HUB, 0, 0,
		(void *)&fHubDescriptor, sizeof(usb_hub_descriptor), &actualLength);

	// we need at least 8 bytes
	if (status < B_OK || actualLength < 8) {
		TRACE_ERROR(("USB Hub %d: Error getting hub descriptor\n", DeviceAddress()));
		return;
	}

	TRACE(("USB Hub %d: hub descriptor (%ld bytes):\n", DeviceAddress(), actualLength));
	TRACE(("\tlength:..............%d\n", fHubDescriptor.length));
	TRACE(("\tdescriptor_type:.....0x%02x\n", fHubDescriptor.descriptor_type));
	TRACE(("\tnum_ports:...........%d\n", fHubDescriptor.num_ports));
	TRACE(("\tcharacteristics:.....0x%04x\n", fHubDescriptor.characteristics));
	TRACE(("\tpower_on_to_power_g:.%d\n", fHubDescriptor.power_on_to_power_good));
	TRACE(("\tdevice_removeable:...0x%02x\n", fHubDescriptor.device_removeable));
	TRACE(("\tpower_control_mask:..0x%02x\n", fHubDescriptor.power_control_mask));

	Object *object = GetStack()->GetObject(Configuration()->interface->active->endpoint[0].handle);
	if (!object || (object->Type() & USB_OBJECT_INTERRUPT_PIPE) == 0) {
		TRACE_ERROR(("USB Hub %d: no interrupt pipe found\n", DeviceAddress()));
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
			TRACE_ERROR(("USB Hub %d: power up failed on port %ld\n", DeviceAddress(), i));
	}

	// Wait for power to stabilize
	snooze(fHubDescriptor.power_on_to_power_good * 2000);

	fInitOK = true;
	TRACE(("USB Hub %d: initialised ok\n", DeviceAddress()));
}


Hub::~Hub()
{
	Lock();
	benaphore_destroy(&fLock);

	// Remove all child devices
	for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
		if (!fChildren[i])
			continue;

		TRACE(("USB Hub %d: removing device 0x%08lx\n", DeviceAddress(), fChildren[i]));
		GetStack()->NotifyDeviceChange(fChildren[i], false);
		GetBusManager()->FreeDevice(fChildren[i]);
	}

	delete fInterruptPipe;
}


bool
Hub::Lock()
{
	return (benaphore_lock(&fLock) == B_OK);
}


void
Hub::Unlock()
{
	benaphore_unlock(&fLock);
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
		TRACE_ERROR(("USB Hub %d: error updating port status\n", DeviceAddress()));
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

		if (fPortStatus[index].change & C_PORT_RESET) {
			// reset is done
			break;
		}
	}

	if ((fPortStatus[index].change & C_PORT_RESET) == 0) {
		TRACE_ERROR(("USB Hub %d: port %d won't reset\n", DeviceAddress(), index));
		return B_ERROR;
	}

	// clear the reset change
	result = DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
		USB_REQUEST_CLEAR_FEATURE, C_PORT_RESET, index + 1, 0, NULL, 0, NULL);
	if (result < B_OK)
		return result;

	// wait for reset recovery
	snooze(USB_DELAY_PORT_RESET_RECOVERY);
	TRACE(("USB Hub %d: port %d was reset successfully\n", DeviceAddress(), index));
	return B_OK;
}


void
Hub::Explore()
{
	for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
		if (i >= 8) {
			TRACE(("USB Hub %d: hub supports more ports than we do (%d)\n", DeviceAddress(), fHubDescriptor.num_ports));
			fHubDescriptor.num_ports = 8;
			continue;
		}

		status_t result = UpdatePortStatus(i);
		if (result < B_OK)
			continue;

#ifdef TRACE_USB
		if (fPortStatus[i].change) {
			TRACE(("USB Hub %d: port %ld: status: 0x%04x; change: 0x%04x\n", DeviceAddress(), i, fPortStatus[i].status, fPortStatus[i].change));
			TRACE(("USB Hub %d: device at port %ld: 0x%08lx\n", DeviceAddress(), i, fChildren[i]));
		}
#endif

		if (fPortStatus[i].change & PORT_STATUS_CONNECTION) {
			// clear status change
			DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE, C_PORT_CONNECTION, i + 1,
				0, NULL, 0, NULL);

			if (fPortStatus[i].status & PORT_STATUS_CONNECTION) {
				// new device attached!
				TRACE(("USB Hub %d: new device connected\n", DeviceAddress()));

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
					TRACE(("USB Hub %d: device disappeared on reset\n", DeviceAddress()));
					continue;
				}

				usb_speed speed = USB_SPEED_FULLSPEED;
				if (fPortStatus[i].status & PORT_STATUS_LOW_SPEED)
					speed = USB_SPEED_LOWSPEED;
				if (fPortStatus[i].status & PORT_STATUS_HIGH_SPEED)
					speed = USB_SPEED_HIGHSPEED;

				Device *newDevice = GetBusManager()->AllocateDevice(this, speed);

				if (newDevice && Lock()) {
					fChildren[i] = newDevice;
					Unlock();
					GetStack()->NotifyDeviceChange(fChildren[i], true);
				} else {
					if (newDevice)
						GetBusManager()->FreeDevice(newDevice);

					// the device failed to setup correctly, disable the port
					// so that the device doesn't get in the way of future
					// addressing.
					DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
						USB_REQUEST_CLEAR_FEATURE, PORT_ENABLE, i + 1,
						0, NULL, 0, NULL);
				}
			} else {
				// Device removed...
				TRACE(("USB Hub %d: device removed\n", DeviceAddress()));
				if (fChildren[i]) {
					TRACE(("USB Hub %d: removing device 0x%08lx\n", DeviceAddress(), fChildren[i]));
					GetStack()->NotifyDeviceChange(fChildren[i], false);

					if (Lock()) {
						GetBusManager()->FreeDevice(fChildren[i]);
						fChildren[i] = NULL;
						Unlock();
					}
				}
			}
		}

		// other port changes we do not really handle, report and clear them
		if (fPortStatus[i].change & PORT_STATUS_ENABLE) {
			TRACE_ERROR(("USB Hub %d: port %ld %sabled\n", DeviceAddress(), i, (fPortStatus[i].status & PORT_STATUS_ENABLE) ? "en" : "dis"));
			DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE, C_PORT_ENABLE, i + 1,
				0, NULL, 0, NULL);
		}

		if (fPortStatus[i].change & PORT_STATUS_SUSPEND) {
			TRACE_ERROR(("USB Hub %d: port %ld is %ssuspended\n", DeviceAddress(), i, (fPortStatus[i].status & PORT_STATUS_SUSPEND) ? "" : "not "));
			DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE, C_PORT_SUSPEND, i + 1,
				0, NULL, 0, NULL);
		}

		if (fPortStatus[i].change & PORT_STATUS_OVER_CURRENT) {
			TRACE_ERROR(("USB Hub %d: port %ld is %sin an over current state\n", DeviceAddress(), i, (fPortStatus[i].status & PORT_STATUS_OVER_CURRENT) ? "" : "not "));
			DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE, C_PORT_OVER_CURRENT, i + 1,
				0, NULL, 0, NULL);
		}

		if (fPortStatus[i].change & PORT_RESET) {
			TRACE_ERROR(("USB Hub %d: port %ld was reset\n", DeviceAddress(), i));
			DefaultPipe()->SendRequest(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT,
				USB_REQUEST_CLEAR_FEATURE, C_PORT_RESET, i + 1,
				0, NULL, 0, NULL);
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
Hub::InterruptCallback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	TRACE(("USB Hub %d: interrupt callback!\n", ((Hub *)data)->DeviceAddress()));
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
	TRACE(("USB Hub %d: reporting hub\n", DeviceAddress()));

	// Report ourselfs first
	status_t result = Device::ReportDevice(supportDescriptors,
		supportDescriptorCount, hooks, cookies, added);

	// Then report all of our children
	if (!Lock())
		return B_ERROR;

	for (int32 i = 0; i < fHubDescriptor.num_ports; i++) {
		if (!fChildren[i])
			continue;

		if (fChildren[i]->ReportDevice(supportDescriptors,
				supportDescriptorCount, hooks, cookies, added) == B_OK)
			result = B_OK;
	}

	Unlock();
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
