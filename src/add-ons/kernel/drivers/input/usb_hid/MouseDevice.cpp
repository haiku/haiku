/*
 * Copyright 2008-2009 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */


//!	Driver for USB Human Interface Devices.


#include "Driver.h"
#include "MouseDevice.h"

#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDReportItem.h"

#include <new>
#include <string.h>
#include <usb/USB_hid.h>

#include <keyboard_mouse_driver.h>


MouseDevice::MouseDevice(HIDReport *report, HIDReportItem *xAxis,
	HIDReportItem *yAxis)
	:
	ProtocolHandler(report->Device(), "input/mouse/usb/", 512),
	fReport(report),
	fXAxis(xAxis),
	fYAxis(yAxis),
	fWheel(NULL),
	fLastButtons(0),
	fClickCount(0),
	fLastClickTime(0),
	fClickSpeed(250000)
{
	uint32 buttonCount = 0;
	for (uint32 i = 0; i < report->CountItems(); i++) {
		HIDReportItem *item = report->ItemAt(i);
		if (!item->HasData())
			continue;

		if (item->UsagePage() == B_HID_USAGE_PAGE_BUTTON
			&& item->UsageID() - 1 < B_MAX_MOUSE_BUTTONS)
			fButtons[buttonCount++] = item;
	}

	fButtons[buttonCount] = NULL;

	fWheel = report->FindItem(B_HID_USAGE_PAGE_GENERIC_DESKTOP,
		B_HID_UID_GD_WHEEL);

	TRACE("mouse device with %lu buttons and %swheel\n", buttonCount,
		fWheel == NULL ? "no " : "");
	TRACE("report id: %u\n", report->ID());
}


ProtocolHandler *
MouseDevice::AddHandler(HIDDevice *device, HIDReport *report)
{
	// try to find at least an x and y axis
	HIDReportItem *xAxis = report->FindItem(B_HID_USAGE_PAGE_GENERIC_DESKTOP,
		B_HID_UID_GD_X);
	if (xAxis == NULL)
		return NULL;

	HIDReportItem *yAxis = report->FindItem(B_HID_USAGE_PAGE_GENERIC_DESKTOP,
		B_HID_UID_GD_Y);
	if (yAxis == NULL)
		return NULL;

	return new(std::nothrow) MouseDevice(report, xAxis, yAxis);
}


status_t
MouseDevice::Control(uint32 *cookie, uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case MS_READ:
			while (RingBufferReadable() == 0) {
				status_t result = _ReadReport();
				if (result != B_OK)
					return result;
			}

			return RingBufferRead(buffer, sizeof(mouse_movement));

		case MS_NUM_EVENTS:
		{
			int32 count = RingBufferReadable() / sizeof(mouse_movement);
			if (count == 0 && fReport->Device()->IsRemoved())
				return B_DEV_NOT_READY;
			return count;
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
MouseDevice::_ReadReport()
{
	status_t result = fReport->WaitForReport(B_INFINITE_TIMEOUT);
	if (result != B_OK) {
		if (fReport->Device()->IsRemoved()) {
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

	mouse_movement info;
	memset(&info, 0, sizeof(info));

	if (fXAxis->Extract() == B_OK && fXAxis->Valid())
		info.xdelta = fXAxis->Data();
	if (fYAxis->Extract() == B_OK && fYAxis->Valid())
		info.ydelta = -fYAxis->Data();

	if (fWheel != NULL && fWheel->Extract() == B_OK && fWheel->Valid())
		info.wheel_ydelta = -fWheel->Data();

	for (uint32 i = 0; i < B_MAX_MOUSE_BUTTONS; i++) {
		HIDReportItem *button = fButtons[i];
		if (button == NULL)
			break;

		if (button->Extract() == B_OK && button->Valid())
			info.buttons |= (button->Data() & 1) << (button->UsageID() - 1);
	}

	fReport->DoneProcessing();
	TRACE("got mouse report\n");

	bigtime_t timestamp = system_time();
	if (info.buttons != 0) {
		if (fLastButtons == 0) {
			if (fLastClickTime + fClickSpeed > timestamp)
				fClickCount++;
			else
				fClickCount = 1;
		}

		fLastClickTime = timestamp;
		info.clicks = fClickCount;
	}

	fLastButtons = info.buttons;
	info.timestamp = timestamp;
	return RingBufferWrite(&info, sizeof(mouse_movement));
}
