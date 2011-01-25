/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/RepositoryHeader.h>

#include <new>


namespace BPackageKit {


const uint8 BRepositoryHeader::kDefaultPriority	= 50;

const char* BRepositoryHeader::kNameField 				= "name";
const char* BRepositoryHeader::kURLField 				= "url";
const char* BRepositoryHeader::kVendorField 			= "vendor";
const char* BRepositoryHeader::kShortDescriptionField 	= "shortDescr";
const char* BRepositoryHeader::kLongDescriptionField 	= "longDescr";
const char* BRepositoryHeader::kPriorityField 			= "priority";


BRepositoryHeader::BRepositoryHeader()
	:
	fInitStatus(B_NO_INIT),
	fPriority(kDefaultPriority)
{
}


BRepositoryHeader::BRepositoryHeader(BMessage* data)
	:
	inherited(data)
{
	fInitStatus = SetTo(data);
}


BRepositoryHeader::~BRepositoryHeader()
{
}


/*static*/ BRepositoryHeader*
BRepositoryHeader::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BPackageKit::BRepositoryHeader"))
		return new (std::nothrow) BRepositoryHeader(data);

	return NULL;
}


status_t
BRepositoryHeader::Archive(BMessage* data, bool deep) const
{
	status_t result = inherited::Archive(data, deep);
	if (result != B_OK)
		return result;

	if ((result = data->AddString(kNameField, fName)) != B_OK)
		return result;
	if ((result = data->AddString(kURLField, fOriginalBaseURL)) != B_OK)
		return result;
	if ((result = data->AddString(kVendorField, fVendor)) != B_OK)
		return result;
	result = data->AddString(kShortDescriptionField, fShortDescription);
	if (result != B_OK)
		return result;
	result = data->AddString(kLongDescriptionField, fLongDescription);
	if (result != B_OK)
		return result;
	if ((result = data->AddUInt8(kPriorityField, fPriority)) != B_OK)
		return result;

	return B_OK;
}


status_t
BRepositoryHeader::InitCheck() const
{
	return fInitStatus;
}


status_t
BRepositoryHeader::SetTo(const BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t result;
	if ((result = data->FindString(kNameField, &fName)) != B_OK)
		return result;
	if ((result = data->FindString(kURLField, &fOriginalBaseURL)) != B_OK)
		return result;
	if ((result = data->FindString(kVendorField, &fVendor)) != B_OK)
		return result;
	result = data->FindString(kShortDescriptionField, &fShortDescription);
	if (result != B_OK)
		return result;
	result = data->FindString(kLongDescriptionField, &fLongDescription);
	if (result != B_OK)
		return result;
	if ((result = data->FindUInt8(kPriorityField, &fPriority)) != B_OK)
		return result;

	return B_OK;
}


const BString&
BRepositoryHeader::Name() const
{
	return fName;
}


const BString&
BRepositoryHeader::OriginalBaseURL() const
{
	return fOriginalBaseURL;
}


const BString&
BRepositoryHeader::Vendor() const
{
	return fVendor;
}


const BString&
BRepositoryHeader::ShortDescription() const
{
	return fShortDescription;
}


const BString&
BRepositoryHeader::LongDescription() const
{
	return fLongDescription;
}


uint8
BRepositoryHeader::Priority() const
{
	return fPriority;
}


void
BRepositoryHeader::SetName(const BString& name)
{
	fName = name;
}


void
BRepositoryHeader::SetOriginalBaseURL(const BString& url)
{
	fOriginalBaseURL = url;
}


void
BRepositoryHeader::SetVendor(const BString& vendor)
{
	fVendor = vendor;
}


void
BRepositoryHeader::SetShortDescription(const BString& description)
{
	fShortDescription = description;
}


void
BRepositoryHeader::SetLongDescription(const BString& description)
{
	fLongDescription = description;
}


void
BRepositoryHeader::SetPriority(uint8 priority)
{
	fPriority = priority;
}


}	// namespace BPackageKit
