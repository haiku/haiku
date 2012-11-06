/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "WatchpointSetting.h"

#include <Message.h>

#include "Watchpoint.h"


WatchpointSetting::WatchpointSetting()
	:
	fAddress(0),
	fType(0),
	fLength(0),
	fEnabled(false)
{
}


WatchpointSetting::WatchpointSetting(const WatchpointSetting& other)
	:
	fAddress(other.fAddress),
	fType(other.fType),
	fLength(other.fLength),
	fEnabled(other.fEnabled)
{
}


WatchpointSetting::~WatchpointSetting()
{
}


status_t
WatchpointSetting::SetTo(const Watchpoint& watchpoint, bool enabled)
{
	fAddress = watchpoint.Address();
	fType = watchpoint.Type();
	fLength = watchpoint.Length();
	fEnabled = enabled;

	return B_OK;
}


status_t
WatchpointSetting::SetTo(const BMessage& archive)
{
	if (archive.FindUInt64("address", &fAddress) != B_OK)
		fAddress = 0;

	if (archive.FindUInt32("type", &fType) != B_OK)
		fType = 0;

	if (archive.FindInt32("length", &fLength) != B_OK)
		fLength = 0;

	if (archive.FindBool("enabled", &fEnabled) != B_OK)
		fEnabled = false;

	return B_OK;
}


status_t
WatchpointSetting::WriteTo(BMessage& archive) const
{
	archive.MakeEmpty();

	status_t error;
	if ((error = archive.AddUInt64("address", fAddress)) != B_OK
		|| (error = archive.AddUInt32("type", fType)) != B_OK
		|| (error = archive.AddInt32("length", fLength)) != B_OK
		|| (error = archive.AddBool("enabled", fEnabled)) != B_OK) {
		return error;
	}

	return B_OK;
}


WatchpointSetting&
WatchpointSetting::operator=(const WatchpointSetting& other)
{
	if (this == &other)
		return *this;

	fAddress = other.fAddress;
	fType = other.fType;
	fLength = other.fLength;
	fEnabled = other.fEnabled;

	return *this;
}
