/*
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */


//!	Driver for USB Human Interface Devices.


#include "Driver.h"
#include "MouseProtocolHandler.h"

#include "HIDCollection.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDReportItem.h"

#include <new>
#include <string.h>
#include <usb/USB_hid.h>

#include <keyboard_mouse_driver.h>


MouseProtocolHandler::MouseProtocolHandler(HIDReport &report, bool tablet,
	HIDReportItem &xAxis, HIDReportItem &yAxis)
	:
	ProtocolHandler(report.Device(),
		tablet ? "input/tablet/usb" : "input/mouse/usb/", 0),
	fReport(report),
	fTablet(tablet),
	fXAxis(xAxis),
	fYAxis(yAxis),
	fWheel(NULL),
	fLastButtons(0),
	fClickCount(0),
	fLastClickTime(0),
	fClickSpeed(250000)
{
	uint32 buttonCount = 0;
	for (uint32 i = 0; i < report.CountItems(); i++) {
		HIDReportItem *item = report.ItemAt(i);
		if (!item->HasData())
			continue;

		if (item->UsagePage() == B_HID_USAGE_PAGE_BUTTON
			&& item->UsageID() - 1 < B_MAX_MOUSE_BUTTONS) {
			fButtons[buttonCount++] = item;
		}
	}

	fButtons[buttonCount] = NULL;

	fWheel = report.FindItem(B_HID_USAGE_PAGE_GENERIC_DESKTOP,
		B_HID_UID_GD_WHEEL);

	TRACE("mouse device with %lu buttons and %swheel\n", buttonCount,
		fWheel == NULL ? "no " : "");
	TRACE("report id: %u\n", report.ID());
}


void
MouseProtocolHandler::AddHandlers(HIDDevice &device, HIDCollection &collection,
	ProtocolHandler *&handlerList)
{
	bool tablet = false;
	bool supported = false;
	switch (collection.UsagePage()) {
		case B_HID_USAGE_PAGE_GENERIC_DESKTOP:
		{
			switch (collection.UsageID()) {
				case B_HID_UID_GD_MOUSE:
				case B_HID_UID_GD_POINTER:
					supported = true;
					break;
			}

			break;
		}

		case B_HID_USAGE_PAGE_DIGITIZER:
		{
			switch (collection.UsageID()) {
				case B_HID_UID_DIG_DIGITIZER:
				case B_HID_UID_DIG_PEN:
				case B_HID_UID_DIG_LIGHT_PEN:
				case B_HID_UID_DIG_TOUCH_SCREEN:
				case B_HID_UID_DIG_TOUCH_PAD:
				case B_HID_UID_DIG_WHITE_BOARD:
					supported = true;
					tablet = true;
			}

			break;
		}
	}

	if (!supported) {
		TRACE("collection not a mouse/tablet/digitizer\n");
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

		// try to find at least an x and y axis
		HIDReportItem *xAxis = inputReport->FindItem(
			B_HID_USAGE_PAGE_GENERIC_DESKTOP, B_HID_UID_GD_X);
		if (xAxis == NULL)
			continue;

		HIDReportItem *yAxis = inputReport->FindItem(
			B_HID_USAGE_PAGE_GENERIC_DESKTOP, B_HID_UID_GD_Y);
		if (yAxis == NULL)
			continue;

		if (!xAxis->Relative() && !yAxis->Relative())
			tablet = true;

		ProtocolHandler *newHandler = new(std::nothrow) MouseProtocolHandler(
			*inputReport, tablet, *xAxis, *yAxis);
		if (newHandler == NULL) {
			TRACE("failed to allocated mouse protocol handler\n");
			continue;
		}

		newHandler->SetNextHandler(handlerList);
		handlerList = newHandler;
	}
}


status_t
MouseProtocolHandler::Control(uint32 *cookie, uint32 op, void *buffer,
	size_t length)
{
	switch (op) {
		case MS_READ:
		{
			if ((!fTablet && length < sizeof(mouse_movement))
				|| (fTablet && length < sizeof(tablet_movement))) {
				return B_BUFFER_OVERFLOW;
			}

			while (true) {
				status_t result = _ReadReport(buffer);
				if (result != B_INTERRUPTED)
					return result;
			}
		}

		case MS_NUM_EVENTS:
		{
			if (fReport.Device()->IsRemoved())
				return B_DEV_NOT_READY;

			// we are always on demand, so 0 queued events
			return 0;
		}

		case MS_SET_CLICKSPEED:
#ifdef __HAIKU__
				return user_memcpy(&fClickSpeed, buffer, sizeof(bigtime_t));
#else
				fClickSpeed = *(bigtime_t *)buffer;
				return B_OK;
#endif
	}

	return B_ERROR;
}


status_t
MouseProtocolHandler::_ReadReport(void *buffer)
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
		return B_INTERRUPTED;
	}

	float axisAbsoluteData[2];
	uint32 axisRelativeData[2];
	if (fXAxis.Extract() == B_OK && fXAxis.Valid()) {
		if (fXAxis.Relative())
			axisRelativeData[0] = fXAxis.Data();
		else
			axisAbsoluteData[0] = fXAxis.ScaledFloatData();
	}

	if (fYAxis.Extract() == B_OK && fYAxis.Valid()) {
		if (fYAxis.Relative())
			axisRelativeData[1] = fYAxis.Data();
		else
			axisAbsoluteData[1] = fYAxis.ScaledFloatData();
	}

	uint32 wheelData = 0;
	if (fWheel != NULL && fWheel->Extract() == B_OK && fWheel->Valid())
		wheelData = fWheel->Data();

	uint32 buttons = 0;
	for (uint32 i = 0; i < B_MAX_MOUSE_BUTTONS; i++) {
		HIDReportItem *button = fButtons[i];
		if (button == NULL)
			break;

		if (button->Extract() == B_OK && button->Valid())
			buttons |= (button->Data() & 1) << (button->UsageID() - 1);
	}

	fReport.DoneProcessing();
	TRACE("got mouse report\n");

	int32 clicks = 0;
	bigtime_t timestamp = system_time();
	if (buttons != 0) {
		if (fLastButtons == 0) {
			if (fLastClickTime + fClickSpeed > timestamp)
				fClickCount++;
			else
				fClickCount = 1;
		}

		fLastClickTime = timestamp;
		clicks = fClickCount;
	}

	fLastButtons = buttons;

	if (fTablet) {
		tablet_movement *info = (tablet_movement *)buffer;
		memset(info, 0, sizeof(tablet_movement));

		info->xpos = axisAbsoluteData[0];
		info->ypos = axisAbsoluteData[1];
		info->has_contact = true;
		info->pressure = 1.0;
		info->eraser = false;
		info->tilt_x = 0.0;
		info->tilt_y = 0.0;

		info->buttons = buttons;
		info->clicks = clicks;
		info->timestamp = timestamp;
		info->wheel_ydelta = -wheelData;
	} else {
		mouse_movement *info = (mouse_movement *)buffer;
		memset(info, 0, sizeof(mouse_movement));

		info->buttons = buttons;
		info->xdelta = axisRelativeData[0];
		info->ydelta = -axisRelativeData[1];
		info->clicks = clicks;
		info->timestamp = timestamp;
		info->wheel_ydelta = -wheelData;
	}

	return B_OK;
}
