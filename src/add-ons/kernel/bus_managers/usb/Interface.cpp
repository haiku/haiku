/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "usb_p.h"


Interface::Interface(BusManager *bus, int8 deviceAddress, pipeSpeed speed)
	:	ControlPipe(bus, deviceAddress, speed, 0, 8)
{
}


status_t
Interface::SetFeature(uint16 selector)
{
	return SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_INTERFACE_OUT,
		USB_REQUEST_SET_FEATURE,
		selector,
		0,
		0,
		NULL,
		0,
		NULL);
}


status_t
Interface::ClearFeature(uint16 selector)
{
	return SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_INTERFACE_OUT,
		USB_REQUEST_CLEAR_FEATURE,
		selector,
		0,
		0,
		NULL,
		0,
		NULL);
}


status_t
Interface::GetStatus(uint16 *status)
{
	return SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_INTERFACE_IN,
		USB_REQUEST_GET_STATUS,
		0,
		0,
		2,
		(void *)status,
		2,
		NULL);
}
