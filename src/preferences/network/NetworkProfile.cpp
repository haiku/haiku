/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkProfile.h>

#include <stdlib.h>


using namespace BNetworkKit;


BNetworkProfile::BNetworkProfile()
	:
	fIsDefault(false),
	fIsCurrent(false),
	fName(NULL)
{
}


BNetworkProfile::BNetworkProfile(const char* path)
	:
	fIsDefault(false),
	fIsCurrent(false)
{
	SetTo(path);
}


BNetworkProfile::BNetworkProfile(const entry_ref& ref)
	:
	fIsDefault(false),
	fIsCurrent(false)
{
	SetTo(ref);
}


BNetworkProfile::BNetworkProfile(const BEntry& entry)
	:
	fIsDefault(false),
	fIsCurrent(false)
{
	SetTo(entry);
}


BNetworkProfile::~BNetworkProfile()
{
}


status_t
BNetworkProfile::SetTo(const char* path)
{
	status_t status = fEntry.SetTo(path, true);
	if (status != B_OK)
		return status;

	fPath.Unset();
	fName = NULL;
	return B_OK;
}


status_t
BNetworkProfile::SetTo(const entry_ref& ref)
{
	status_t status = fEntry.SetTo(&ref);
	if (status != B_OK)
		return status;

	fPath.Unset();
	fName = ref.name;
	return B_OK;
}


status_t
BNetworkProfile::SetTo(const BEntry& entry)
{
	fEntry = entry;
	fPath.Unset();
	fName = NULL;
	return B_OK;
}


const char*
BNetworkProfile::Name()
{
	if (fName == NULL) {
		if (fEntry.GetPath(&fPath) == B_OK)
			fName = fPath.Leaf();
	}

	return fName;
}


status_t
BNetworkProfile::SetName(const char* name)
{
	return B_OK;
}


bool
BNetworkProfile::Exists()
{
	return fEntry.Exists();
}


status_t
BNetworkProfile::Delete()
{
	return B_ERROR;
}


bool
BNetworkProfile::IsDefault()
{
	return fIsDefault;
}


bool
BNetworkProfile::IsCurrent()
{
	return fIsCurrent;
}


status_t
BNetworkProfile::MakeCurrent()
{
	return B_ERROR;
}


// #pragma mark -


BNetworkProfile*
BNetworkProfile::Default()
{
	return NULL;
}


BNetworkProfile*
BNetworkProfile::Current()
{
	return NULL;
}
