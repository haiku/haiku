/*
 * Copyright 2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */


//!	Driver for USB Human Interface Devices.


#include "Driver.h"
#include "JoystickProtocolHandler.h"

#include "HIDCollection.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDReportItem.h"

#include <new>
#include <string.h>
#include <usb/USB_hid.h>


JoystickProtocolHandler::JoystickProtocolHandler(HIDReport &report)
	:
	ProtocolHandler(report.Device(), "joystick/usb/", 0),
	fReport(report),
	fAxisCount(0),
	fAxis(NULL),
	fHatCount(0),
	fHats(NULL),
	fButtonCount(0),
	fMaxButton(0),
	fButtons(NULL),
	fOpenCount(0),
	fUpdateThread(-1)
{
	mutex_init(&fUpdateLock, "joystick update lock");
	memset(&fJoystickModuleInfo, 0, sizeof(joystick_module_info));
	memset(&fCurrentValues, 0, sizeof(variable_joystick));

	for (uint32 i = 0; i < report.CountItems(); i++) {
		HIDReportItem *item = report.ItemAt(i);
		if (!item->HasData())
			continue;

		switch (item->UsagePage()) {
			case B_HID_USAGE_PAGE_BUTTON:
			{
				if (item->UsageID() > INT16_MAX)
					break;

				HIDReportItem **newButtons = (HIDReportItem **)realloc(fButtons,
					++fButtonCount * sizeof(HIDReportItem *));
				if (newButtons == NULL) {
					fButtonCount--;
					break;
				}

				fButtons = newButtons;
				fButtons[fButtonCount - 1] = item;

				if (fMaxButton < item->UsageID())
					fMaxButton = item->UsageID();
				break;
			}

			case B_HID_USAGE_PAGE_GENERIC_DESKTOP:
			{
				switch (item->UsageID()) {
					case B_HID_UID_GD_X:
					case B_HID_UID_GD_Y:
					case B_HID_UID_GD_Z:
					case B_HID_UID_GD_RX:
					case B_HID_UID_GD_RY:
					case B_HID_UID_GD_RZ:
						uint16 axis = item->UsageID() - B_HID_UID_GD_X + 1;
						if (axis > INT16_MAX)
							break;

						if (fAxisCount < axis) {
							HIDReportItem **newAxis = (HIDReportItem **)realloc(
								fAxis, axis * sizeof(HIDReportItem *));
							if (newAxis == NULL)
								break;

							for (uint16 i = fAxisCount; i < axis; i++)
								newAxis[i] = NULL;

							fAxis = newAxis;
							fAxisCount = axis;
						}

						fAxis[axis - 1] = item;
						break;
				}

				break;
			}
		}
	}


	fCurrentValues.initialize(fAxisCount, 0, fMaxButton);

	TRACE("joystick device with %lu buttons\n", fButtonCount);
	TRACE("report id: %u\n", report.ID());
}


JoystickProtocolHandler::~JoystickProtocolHandler()
{
	free(fCurrentValues.data);
}


void
JoystickProtocolHandler::AddHandlers(HIDDevice &device,
	HIDCollection &collection, ProtocolHandler *&handlerList)
{
	if (collection.UsagePage() != B_HID_USAGE_PAGE_GENERIC_DESKTOP
		|| (collection.UsageID() != B_HID_UID_GD_JOYSTICK
			&& collection.UsageID() != B_HID_UID_GD_GAMEPAD
			&& collection.UsageID() != B_HID_UID_GD_MULTIAXIS)) {
		TRACE("collection not a joystick or gamepad\n");
		return;
	}

	HIDParser &parser = device.Parser();
	uint32 maxReportCount = parser.CountReports(HID_REPORT_TYPE_INPUT);
	if (maxReportCount == 0)
		return;

	uint32 inputReportCount = 0;
	HIDReport *inputReports[maxReportCount];
	collection.BuildReportList(HID_REPORT_TYPE_INPUT, inputReports,
		inputReportCount);

	for (uint32 i = 0; i < inputReportCount; i++) {
		HIDReport *inputReport = inputReports[i];

		// try to find at least one axis
		bool foundAxis = false;
		for (uint32 j = 0; j < inputReport->CountItems(); j++) {
			HIDReportItem *item = inputReport->ItemAt(j);
			if (item == NULL || !item->HasData())
				continue;

			if (item->UsagePage() != B_HID_USAGE_PAGE_GENERIC_DESKTOP)
				continue;

			if (item->UsageID() >= B_HID_UID_GD_X
				&& item->UsageID() <= B_HID_UID_GD_RZ) {
				foundAxis = true;
				break;
			}
		}

		if (!foundAxis)
			continue;

		ProtocolHandler *newHandler
			= new(std::nothrow) JoystickProtocolHandler(*inputReport);
		if (newHandler == NULL) {
			TRACE("failed to allocated joystick protocol handler\n");
			continue;
		}

		newHandler->SetNextHandler(handlerList);
		handlerList = newHandler;
	}
}


status_t
JoystickProtocolHandler::Open(uint32 flags, uint32 *cookie)
{
	if (fCurrentValues.data == NULL)
		return B_NO_INIT;

	status_t result = mutex_lock(&fUpdateLock);
	if (result != B_OK)
		return result;

	if (fUpdateThread < 0) {
		fUpdateThread = spawn_kernel_thread(_UpdateThread, "joystick update",
			B_NORMAL_PRIORITY, (void *)this);

		if (fUpdateThread < 0)
			result = fUpdateThread;
		else
			resume_thread(fUpdateThread);
	}

	if (result == B_OK)
		fOpenCount++;

	mutex_unlock(&fUpdateLock);
	if (result != B_OK)
		return result;

	return ProtocolHandler::Open(flags, cookie);
}


status_t
JoystickProtocolHandler::Close(uint32 *cookie)
{
	status_t result = mutex_lock(&fUpdateLock);
	if (result == B_OK) {
		if (--fOpenCount == 0)
			fUpdateThread = -1;
		mutex_unlock(&fUpdateLock);
	}

	return ProtocolHandler::Close(cookie);
}



status_t
JoystickProtocolHandler::Read(uint32 *cookie, off_t position, void *buffer,
	size_t *numBytes)
{
	if (*numBytes < fCurrentValues.data_size)
		return B_BUFFER_OVERFLOW;

	// this is a polling interface, we just return the current value
	status_t result = mutex_lock(&fUpdateLock);
	if (result != B_OK) {
		*numBytes = 0;
		return result;
	}

	memcpy(buffer, fCurrentValues.data, fCurrentValues.data_size);
	mutex_unlock(&fUpdateLock);

	*numBytes = fCurrentValues.data_size;
	return B_OK;
}


status_t
JoystickProtocolHandler::Write(uint32 *cookie, off_t position,
	const void *buffer, size_t *numBytes)
{
	*numBytes = 0;
	return B_NOT_SUPPORTED;
}


status_t
JoystickProtocolHandler::Control(uint32 *cookie, uint32 op, void *buffer,
	size_t length)
{
	switch (op) {
		case B_JOYSTICK_SET_DEVICE_MODULE:
		{
			if (length < sizeof(joystick_module_info))
				return B_BAD_VALUE;

			status_t result = mutex_lock(&fUpdateLock);
			if (result != B_OK)
				return result;

			fJoystickModuleInfo = *(joystick_module_info *)buffer;

			bool supportsVariable = (fJoystickModuleInfo.flags
				& js_flag_variable_size_reads) != 0;
			if (!supportsVariable) {
				// We revert to a structure that we can support using only
				// the data available in an extended_joystick structure.
				free(fCurrentValues.data);
				fCurrentValues.initialize_to_extended_joystick();
				if (fAxisCount > MAX_AXES)
					fAxisCount = MAX_AXES;
				if (fHatCount > MAX_HATS)
					fHatCount = MAX_HATS;
				if (fMaxButton > MAX_BUTTONS)
					fMaxButton = MAX_BUTTONS;

				TRACE_ALWAYS("using joystick in extended_joystick mode\n");
			} else {
				TRACE_ALWAYS("using joystick in variable mode\n");
			}

			fJoystickModuleInfo.num_axes = fAxisCount;
			fJoystickModuleInfo.num_buttons = fMaxButton;
			fJoystickModuleInfo.num_hats = fHatCount;
			fJoystickModuleInfo.num_sticks = 1;
			fJoystickModuleInfo.config_size = 0;
			mutex_unlock(&fUpdateLock);
			break;
		}

		case B_JOYSTICK_GET_DEVICE_MODULE:
			if (length < sizeof(joystick_module_info))
				return B_BAD_VALUE;

			*(joystick_module_info *)buffer = fJoystickModuleInfo;
			break;
	}

	return B_ERROR;
}


int32
JoystickProtocolHandler::_UpdateThread(void *data)
{
	JoystickProtocolHandler *handler = (JoystickProtocolHandler *)data;
	while (handler->fUpdateThread == find_thread(NULL)) {
		status_t result = handler->_Update();
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


status_t
JoystickProtocolHandler::_Update()
{
	status_t result = fReport.WaitForReport(B_INFINITE_TIMEOUT);
	if (result != B_OK) {
		if (fReport.Device()->IsRemoved()) {
			TRACE("device has been removed\n");
			return B_DEV_NOT_READY;
		}

		if (result != B_INTERRUPTED) {
			// interrupts happen when other reports come in on the same
			// input as ours
			TRACE_ALWAYS("error waiting for report: %s\n", strerror(result));
		}

		// signal that we simply want to try again
		return B_OK;
	}

	result = mutex_lock(&fUpdateLock);
	if (result != B_OK) {
		fReport.DoneProcessing();
		return result;
	}

	memset(fCurrentValues.data, 0, fCurrentValues.data_size);

	for (uint32 i = 0; i < fAxisCount; i++) {
		if (fAxis[i] == NULL)
			continue;

		if (fAxis[i]->Extract() == B_OK && fAxis[i]->Valid())
			fCurrentValues.axes[i] = (int16)fAxis[i]->ScaledData(16, true);
	}

	for (uint32 i = 0; i < fButtonCount; i++) {
		HIDReportItem *button = fButtons[i];
		if (button == NULL)
			break;

		uint16 index = button->UsageID() - 1;
		if (index >= fMaxButton)
			continue;

		if (button->Extract() == B_OK && button->Valid()) {
			fCurrentValues.buttons[index / 32]
				|= (button->Data() & 1) << (index % 32);
		}
	}

	fReport.DoneProcessing();
	TRACE("got joystick report\n");

	*fCurrentValues.timestamp = system_time();
	mutex_unlock(&fUpdateLock);
	return B_OK;
}
