/*
 * Copyright 2013 Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2010-2011 Enrique Medina Gremaldos <quiqueiii@gmail.com>
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */


//!	Driver for USB Human Interface Devices.


#include "Driver.h"
#include "TabletProtocolHandler.h"

#include "HIDCollection.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDReportItem.h"

#include <new>
#include <kernel.h>
#include <string.h>
#include <usb/USB_hid.h>

#include <keyboard_mouse_driver.h>


TabletProtocolHandler::TabletProtocolHandler(HIDReport &report,
	HIDReportItem &xAxis, HIDReportItem &yAxis)
	:
	ProtocolHandler(report.Device(), "input/tablet/" DEVICE_PATH_SUFFIX "/",
		0),
	fReport(report),

	fXAxis(xAxis),
	fYAxis(yAxis),
	fWheel(NULL),

	fPressure(NULL),
	fRange(NULL),
	fTip(NULL),
	fBarrelSwitch(NULL),
	fEraser(NULL),
	fXTilt(NULL),
	fYTilt(NULL),

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

	fPressure = report.FindItem(B_HID_USAGE_PAGE_DIGITIZER,
		B_HID_UID_DIG_TIP_PRESSURE);

	fRange = report.FindItem(B_HID_USAGE_PAGE_DIGITIZER,
		B_HID_UID_DIG_IN_RANGE);
		
	fTip = report.FindItem(B_HID_USAGE_PAGE_DIGITIZER,
		B_HID_UID_DIG_TIP_SWITCH);

	fBarrelSwitch = report.FindItem(B_HID_USAGE_PAGE_DIGITIZER,
		B_HID_UID_DIG_BARREL_SWITCH);

	fEraser = report.FindItem(B_HID_USAGE_PAGE_DIGITIZER,
		B_HID_UID_DIG_ERASER);

	fXTilt = report.FindItem(B_HID_USAGE_PAGE_DIGITIZER,
		B_HID_UID_DIG_X_TILT);

	fYTilt = report.FindItem(B_HID_USAGE_PAGE_DIGITIZER,
		B_HID_UID_DIG_Y_TILT);

	TRACE("tablet device with %" B_PRIu32 " buttons, %stip, %seraser, "
		"%spressure, and %stilt\n",
		buttonCount,
		fTip == NULL ? "no " : "",
		fEraser == NULL ? "no " : "",
		fPressure == NULL ? "no " : "",
		fXTilt == NULL && fYTilt == NULL ? "no " : "");

	TRACE("report id: %u\n", report.ID());
}


void
TabletProtocolHandler::AddHandlers(HIDDevice &device, HIDCollection &collection,
	ProtocolHandler *&handlerList)
{
	bool supported = false;
	switch (collection.UsagePage()) {
		case B_HID_USAGE_PAGE_GENERIC_DESKTOP:
		{
			switch (collection.UsageID()) {
				case B_HID_UID_GD_MOUSE:
				case B_HID_UID_GD_POINTER:
					// NOTE: Maybe it is supported if X-axis and Y-axis are
					// absolute. This is determined below by scanning the
					// report items for absolute X and Y axis.
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
					TRACE("found tablet/digitizer\n");
					supported = true;
					break;
			}

			break;
		}
	}

	if (!supported) {
		TRACE("collection not a tablet/digitizer\n");
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

		// try to find at least an absolute x and y axis
		HIDReportItem *xAxis = inputReport->FindItem(
			B_HID_USAGE_PAGE_GENERIC_DESKTOP, B_HID_UID_GD_X);
		if (xAxis == NULL || xAxis->Relative())
			continue;

		HIDReportItem *yAxis = inputReport->FindItem(
			B_HID_USAGE_PAGE_GENERIC_DESKTOP, B_HID_UID_GD_Y);
		if (yAxis == NULL || yAxis->Relative())
			continue;

		ProtocolHandler *newHandler = new(std::nothrow) TabletProtocolHandler(
			*inputReport, *xAxis, *yAxis);
		if (newHandler == NULL) {
			TRACE("failed to allocated tablet protocol handler\n");
			continue;
		}

		newHandler->SetNextHandler(handlerList);
		handlerList = newHandler;
	}
}


status_t
TabletProtocolHandler::Control(uint32 *cookie, uint32 op, void *buffer,
	size_t length)
{
	switch (op) {
		case MS_READ:
		{
			if (length < sizeof(tablet_movement))
				return B_BUFFER_OVERFLOW;

			while (true) {
				tablet_movement movement;
				status_t result = _ReadReport(&movement, cookie);
				if (result == B_INTERRUPTED)
					continue;
				if (!IS_USER_ADDRESS(buffer)
					|| user_memcpy(buffer, &movement, sizeof(movement))
						!= B_OK) {
					return B_BAD_ADDRESS;
				}

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
				if (!IS_USER_ADDRESS(buffer)
					|| user_memcpy(&fClickSpeed, buffer, sizeof(bigtime_t))
						!= B_OK) {
					return B_BAD_ADDRESS;
				}

				return B_OK;
	}

	return B_ERROR;
}


status_t
TabletProtocolHandler::_ReadReport(void *buffer, uint32 *cookie)
{
	status_t result = fReport.WaitForReport(B_INFINITE_TIMEOUT);
	if (result != B_OK) {
		if (fReport.Device()->IsRemoved()) {
			TRACE("device has been removed\n");
			return B_DEV_NOT_READY;
		}

		if ((*cookie & PROTOCOL_HANDLER_COOKIE_FLAG_CLOSED) != 0)
			return B_CANCELED;

		if (result != B_INTERRUPTED) {
			// interrupts happen when other reports come in on the same
			// input as ours
			TRACE_ALWAYS("error waiting for report: %s\n", strerror(result));
		}

		// signal that we simply want to try again
		return B_INTERRUPTED;
	}

	float axisAbsoluteData[2];
	axisAbsoluteData[0] = 0.0f;
	axisAbsoluteData[1] = 0.0f;

	if (fXAxis.Extract() == B_OK && fXAxis.Valid())
		axisAbsoluteData[0] = fXAxis.ScaledFloatData();

	if (fYAxis.Extract() == B_OK && fYAxis.Valid())
		axisAbsoluteData[1] = fYAxis.ScaledFloatData();

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

	float pressure = 1.0f;
	if (fPressure != NULL && fPressure->Extract() == B_OK
		&& fPressure->Valid()) {
		pressure = fPressure->ScaledFloatData();
	}

	float xTilt = 0.0f;
	if (fXTilt != NULL && fXTilt->Extract() == B_OK && fXTilt->Valid())
		xTilt = fXTilt->ScaledFloatData();

	float yTilt = 0.0f;
	if (fYTilt != NULL && fYTilt->Extract() == B_OK && fYTilt->Valid())
		yTilt = fYTilt->ScaledFloatData();

	bool inRange = true;
	if (fRange != NULL && fRange->Extract() == B_OK && fRange->Valid())
		inRange = ((fRange->Data() & 1) != 0);

	bool eraser = false;
	if (fEraser != NULL && fEraser->Extract() == B_OK && fEraser->Valid())
		eraser = ((fEraser->Data() & 1) != 0);

	fReport.DoneProcessing();
	TRACE("got tablet report\n");

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

	tablet_movement *info = (tablet_movement *)buffer;
	memset(info, 0, sizeof(tablet_movement));

	info->xpos = axisAbsoluteData[0];
	info->ypos = axisAbsoluteData[1];
	info->has_contact = inRange;
	info->pressure = pressure;
	info->eraser = eraser;
	info->tilt_x = xTilt;
	info->tilt_y = yTilt;

	info->buttons = buttons;
	info->clicks = clicks;
	info->timestamp = timestamp;
	info->wheel_ydelta = -wheelData;

	return B_OK;
}
