/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "usb_private.h"


Interface::Interface(Object *parent, uint8 interfaceIndex)
	:	Object(parent),
		fInterfaceIndex(interfaceIndex)
{
	TRACE("creating interface\n");
}


status_t
Interface::SetFeature(uint16 selector)
{
	TRACE("set feature %u\n", selector);
	return ((Device *)Parent())->DefaultPipe()->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_INTERFACE_OUT,
		USB_REQUEST_SET_FEATURE,
		selector,
		fInterfaceIndex,
		0,
		NULL,
		0,
		NULL);
}


status_t
Interface::ClearFeature(uint16 selector)
{
	TRACE("clear feature %u\n", selector);
	return ((Device *)Parent())->DefaultPipe()->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_INTERFACE_OUT,
		USB_REQUEST_CLEAR_FEATURE,
		selector,
		fInterfaceIndex,
		0,
		NULL,
		0,
		NULL);
}


status_t
Interface::GetStatus(uint16 *status)
{
	TRACE("get status\n");
	return ((Device *)Parent())->DefaultPipe()->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_INTERFACE_IN,
		USB_REQUEST_GET_STATUS,
		fInterfaceIndex,
		0,
		2,
		(void *)status,
		2,
		NULL);
}
