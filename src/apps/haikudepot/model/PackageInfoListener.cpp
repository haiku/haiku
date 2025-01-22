/*
 * Copyright 2013-2025, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "PackageInfoListener.h"

#include <stdio.h>

#include "PackageInfo.h"


const char* kPackageInfoChangesKey = "change";
const char* kPackageInfoPackageNameKey = "packageName";
const char* kPackageInfoEventsKey = "events";


// #pragma mark - PackageInfoEvent


PackageInfoEvent::PackageInfoEvent()
	:
	fPackage(),
	fChanges(0)
{
}


PackageInfoEvent::PackageInfoEvent(const PackageInfoRef& package, uint32 changes)
	:
	fPackage(package),
	fChanges(changes)
{
}


PackageInfoEvent::PackageInfoEvent(const PackageInfoEvent& other)
	:
	fPackage(other.fPackage),
	fChanges(other.fChanges)
{
}


PackageInfoEvent::~PackageInfoEvent()
{
}


bool
PackageInfoEvent::operator==(const PackageInfoEvent& other)
{
	if (this == &other)
		return true;
	return fPackage == other.fPackage && fChanges == other.fChanges;
}


bool
PackageInfoEvent::operator!=(const PackageInfoEvent& other)
{
	return !(*this == other);
}


PackageInfoEvent&
PackageInfoEvent::operator=(const PackageInfoEvent& other)
{
	if (this != &other) {
		fPackage = other.fPackage;
		fChanges = other.fChanges;
	}

	return *this;
}


status_t
PackageInfoEvent::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddUInt32(kPackageInfoChangesKey, fChanges);
	if (result == B_OK)
		result = into->AddString(kPackageInfoPackageNameKey, fPackage->Name());
	return result;
}


// #pragma mark - PackageInfoEvents


PackageInfoEvents::PackageInfoEvents()
{
}


PackageInfoEvents::PackageInfoEvents(const PackageInfoEvent& event)
{
	fEvents.push_back(event);
}


PackageInfoEvents::PackageInfoEvents(const PackageInfoEvents& other)
{
	for (int32 i = other.CountEvents() - 1; i >= 0; i--)
		fEvents.push_back(other.EventAtIndex(i));
}


void
PackageInfoEvents::AddEvent(const PackageInfoEvent event)
{
	fEvents.push_back(event);
}


bool
PackageInfoEvents::IsEmpty() const
{
	return fEvents.empty();
}


int32
PackageInfoEvents::CountEvents() const
{
	return fEvents.size();
}


const PackageInfoEvent&
PackageInfoEvents::EventAtIndex(int32 index) const
{
	return fEvents[index];
}


status_t
PackageInfoEvents::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	BString indexString;
	std::vector<PackageInfoEvent>::const_iterator it;

	for (it = fEvents.begin(); it != fEvents.end(); it++) {
		BMessage eventMessage;
		result = (*it).Archive(&eventMessage);
		if (result == B_OK)
			result = into->AddMessage(kPackageInfoEventsKey, &eventMessage);
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
