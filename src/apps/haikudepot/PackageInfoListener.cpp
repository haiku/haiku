/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfoListener.h"

#include <stdio.h>

#include "PackageInfo.h"


// #pragma mark - PackageInfoEvent


PackageInfoEvent::PackageInfoEvent()
	:
	fPackage(),
	fChanges(0)
{
}


PackageInfoEvent::PackageInfoEvent(const PackageInfoRef& package,
		uint32 changes)
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

	return fPackage == other.fPackage
		&& fChanges == other.fChanges;
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


// #pragma mark - PackageInfoListener


PackageInfoListener::PackageInfoListener()
{
}


PackageInfoListener::~PackageInfoListener()
{
}
