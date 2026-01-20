/*
 * Copyright 2013-2025, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "PackageInfoListener.h"

#include <stdio.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "PackageInfo.h"


static const char* const kKeyEvents = "events";
static const char* const kKeyChanges = "changes";

// #pragma mark - PackageInfoChangeEvent


PackageInfoChangeEvent::PackageInfoChangeEvent(PackageInfoRef package, uint32 changes)
	:
	fPackage(package),
	fChanges(changes)
{
}


PackageInfoChangeEvent::~PackageInfoChangeEvent()
{
}


// #pragma mark - PackageChangeEvent


PackageChangeEvent::PackageChangeEvent()
	:
	fPackageName(),
	fChanges(0)
{
}


PackageChangeEvent::PackageChangeEvent(const BString& packageName, uint32 changes)
	:
	fPackageName(packageName),
	fChanges(changes)
{
}


PackageChangeEvent::PackageChangeEvent(const PackageInfoRef& package, uint32 changes)
	:
	fPackageName(package->Name()),
	fChanges(changes)
{
}


PackageChangeEvent::PackageChangeEvent(const PackageChangeEvent& other)
	:
	fPackageName(other.fPackageName),
	fChanges(other.fChanges)
{
}


PackageChangeEvent::PackageChangeEvent(const BMessage* from)
{
	if (from->FindString(shared_message_keys::kKeyPackageName, &fPackageName) != B_OK)
		HDERROR("expected key [%s] in the message data", shared_message_keys::kKeyPackageName);
	if (from->FindUInt32(kKeyChanges, &fChanges) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyChanges);
}


PackageChangeEvent::~PackageChangeEvent()
{
}


bool
PackageChangeEvent::IsValid() const
{
	return !fPackageName.IsEmpty();
}


bool
PackageChangeEvent::operator==(const PackageChangeEvent& other)
{
	if (this == &other)
		return true;
	return fPackageName == other.fPackageName && fChanges == other.fChanges;
}


bool
PackageChangeEvent::operator!=(const PackageChangeEvent& other)
{
	return !(*this == other);
}


PackageChangeEvent&
PackageChangeEvent::operator=(const PackageChangeEvent& other)
{
	if (this != &other) {
		fPackageName = other.fPackageName;
		fChanges = other.fChanges;
	}

	return *this;
}


status_t
PackageChangeEvent::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK && into == NULL)
		result = B_ERROR;
	if (result == B_OK)
		result = into->AddUInt32(kKeyChanges, fChanges);
	if (result == B_OK)
		result = into->AddString(shared_message_keys::kKeyPackageName, fPackageName);
	return result;
}


// #pragma mark - PackageChangeEvents


PackageChangeEvents::PackageChangeEvents()
{
}


PackageChangeEvents::PackageChangeEvents(const PackageChangeEvent& event)
{
	AddEvent(event);
}


PackageChangeEvents::PackageChangeEvents(const PackageChangeEvents& other)
{
	for (int32 i = other.CountEvents() - 1; i >= 0; i--)
		AddEvent(other.EventAtIndex(i));
}


PackageChangeEvents::PackageChangeEvents(const BMessage* from)
{
	int32 i = 0;
	BMessage eventMessage;

	while (from->FindMessage(kKeyEvents, i, &eventMessage) == B_OK) {
		PackageChangeEvent event = PackageChangeEvent(&eventMessage);

		if (event.IsValid())
			AddEvent(event);
		else
			HDERROR("unable to deserialize package info event");

		i++;
	}
}


void
PackageChangeEvents::AddEvent(const PackageChangeEvent event)
{
	fEvents.push_back(event);
}


bool
PackageChangeEvents::IsEmpty() const
{
	return fEvents.empty();
}


int32
PackageChangeEvents::CountEvents() const
{
	return fEvents.size();
}


const PackageChangeEvent&
PackageChangeEvents::EventAtIndex(int32 index) const
{
	return fEvents[index];
}


status_t
PackageChangeEvents::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	BString indexString;
	std::vector<PackageChangeEvent>::const_iterator it;

	for (it = fEvents.begin(); it != fEvents.end(); it++) {
		BMessage eventMessage;
		result = (*it).Archive(&eventMessage);
		if (result == B_OK)
			result = into->AddMessage(kKeyEvents, &eventMessage);
	}

	return result;
}


// #pragma mark - PackageInfoListener


PackageInfoListener::PackageInfoListener()
{
}


PackageInfoListener::~PackageInfoListener()
{
}
