/*
 * Copyright 2006-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "usb_private.h"


Object::Object(Stack *stack, BusManager *bus)
	:	fParent(NULL),
		fBusManager(bus),
		fStack(stack),
		fUSBID(fStack->GetUSBID(this)),
		fBusy(0)
{
}


Object::Object(Object *parent)
	:	fParent(parent),
		fBusManager(parent->GetBusManager()),
		fStack(parent->GetStack()),
		fUSBID(fStack->GetUSBID(this)),
		fBusy(0)
{
}


Object::~Object()
{
	PutUSBID();
}


void
Object::PutUSBID()
{
	if (fUSBID == UINT32_MAX)
		return;

	fStack->PutUSBID(this);

	int32 retries = 20;
	while (atomic_get(&fBusy) != 0 && retries--)
		snooze(100);
	if (retries <= 0)
		panic("USB object did not become unbusy!");

	fUSBID = UINT32_MAX;
}


status_t
Object::SetFeature(uint16 selector)
{
	// to be implemented in subclasses
	TRACE_ERROR("set feature called\n");
	return B_ERROR;
}


status_t
Object::ClearFeature(uint16 selector)
{
	// to be implemented in subclasses
	TRACE_ERROR("clear feature called\n");
	return B_ERROR;
}


status_t
Object::GetStatus(uint16 *status)
{
	// to be implemented in subclasses
	TRACE_ERROR("get status called\n");
	return B_ERROR;
}
