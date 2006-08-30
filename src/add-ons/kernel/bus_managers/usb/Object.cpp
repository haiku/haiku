/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "usb_p.h"


Object::Object(Stack *stack, BusManager *bus)
	:	fParent(NULL),
		fBusManager(bus),
		fStack(stack),
		fUSBID(fStack->GetUSBID(this))
{
}


Object::Object(Object *parent)
	:	fParent(parent),
		fBusManager(parent->GetBusManager()),
		fStack(parent->GetStack()),
		fUSBID(fStack->GetUSBID(this))
{
}


Object::~Object()
{
	fStack->PutUSBID(fUSBID);
}


status_t
Object::SetFeature(uint16 selector)
{
	// to be implemented in subclasses
	TRACE_ERROR(("USB Object: set feature called\n"));
	return B_ERROR;
}


status_t
Object::ClearFeature(uint16 selector)
{
	// to be implemented in subclasses
	TRACE_ERROR(("USB Object: clear feature called\n"));
	return B_ERROR;
}


status_t
Object::GetStatus(uint16 *status)
{
	// to be implemented in subclasses
	TRACE_ERROR(("USB Object: get status called\n"));
	return B_ERROR;
}
