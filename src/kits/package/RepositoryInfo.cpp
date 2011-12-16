/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/RepositoryInfo.h>

#include <stdlib.h>

#include <new>

#include <driver_settings.h>
#include <File.h>
#include <Message.h>

#include <package/PackageInfo.h>


namespace BPackageKit {


const uint8 BRepositoryInfo::kDefaultPriority	= 50;

const char* BRepositoryInfo::kNameField			= "name";
const char* BRepositoryInfo::kURLField			= "url";
const char* BRepositoryInfo::kVendorField		= "vendor";
const char* BRepositoryInfo::kSummaryField		= "summary";
const char* BRepositoryInfo::kPriorityField		= "priority";
const char* BRepositoryInfo::kArchitectureField	= "architecture";
const char* BRepositoryInfo::kLicenseNameField	= "licenseName";
const char* BRepositoryInfo::kLicenseTextField	= "licenseText";


BRepositoryInfo::BRepositoryInfo()
	:
	fInitStatus(B_NO_INIT),
	fPriority(kDefaultPriority),
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT)
{
}


BRepositoryInfo::BRepositoryInfo(BMessage* data)
	:
	inherited(data),
	fLicenseTexts(5)
{
	fInitStatus = SetTo(data);
}


BRepositoryInfo::BRepositoryInfo(const BEntry& entry)
{
	fInitStatus = SetTo(entry);
}


BRepositoryInfo::~BRepositoryInfo()
{
}


/*static*/ BRepositoryInfo*
BRepositoryInfo::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BPackageKit::BRepositoryInfo"))
		return new (std::nothrow) BRepositoryInfo(data);

	return NULL;
}


status_t
BRepositoryInfo::Archive(BMessage* data, bool deep) const
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
	result = data->AddString(kSummaryField, fSummary);
	if (result != B_OK)
		return result;
	if ((result = data->AddUInt8(kPriorityField, fPriority)) != B_OK)
		return result;
	if ((result = data->AddUInt8(kArchitectureField, fArchitecture)) != B_OK)
		return result;
	for (int i = 0; i < fLicenseNames.CountStrings(); ++i) {
		result = data->AddString(kLicenseNameField, fLicenseNames.StringAt(i));
		if (result != B_OK)
			return result;
	}
	for (int i = 0; i < fLicenseTexts.CountStrings(); ++i) {
		result = data->AddString(kLicenseTextField, fLicenseTexts.StringAt(i));
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


status_t
BRepositoryInfo::InitCheck() const
{
	return fInitStatus;
}


status_t
BRepositoryInfo::SetTo(const BMessage* data)
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
	result = data->FindString(kSummaryField, &fSummary);
	if (result != B_OK)
		return result;
	if ((result = data->FindUInt8(kPriorityField, &fPriority)) != B_OK)
		return result;
	result = data->FindUInt8(kArchitectureField, (uint8*)&fArchitecture);
	if (result != B_OK)
		return result;
	if (fArchitecture == B_PACKAGE_ARCHITECTURE_ANY)
		return B_BAD_DATA;

	const char* licenseName;
	const char* licenseText;
	for (int i = 0;
		data->FindString(kLicenseNameField, i, &licenseName) == B_OK
			&& data->FindString(kLicenseTextField, i, &licenseText) == B_OK;
		++i) {
		if (!fLicenseNames.Add(licenseName) || !fLicenseTexts.Add(licenseText))
			return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
BRepositoryInfo::SetTo(const BEntry& entry)
{
	BFile file(&entry, B_READ_ONLY);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	off_t size;
	if ((result = file.GetSize(&size)) != B_OK)
		return result;

	BString configString;
	char* buffer = configString.LockBuffer(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if ((result = file.Read(buffer, size)) < size) {
		configString.UnlockBuffer(0);
		return (result >= 0) ? B_IO_ERROR : result;
	}

	buffer[size] = '\0';
	configString.UnlockBuffer(size);

	void* settingsHandle = parse_driver_settings_string(configString.String());
	if (settingsHandle == NULL)
		return B_BAD_DATA;

	const char* name = get_driver_parameter(settingsHandle, "name", NULL, NULL);
	const char* url = get_driver_parameter(settingsHandle, "url", NULL, NULL);
	const char* vendor
		= get_driver_parameter(settingsHandle, "vendor", NULL, NULL);
	const char* summary
		= get_driver_parameter(settingsHandle, "summary", NULL, NULL);
	const char* priorityString
		= get_driver_parameter(settingsHandle, "priority", NULL, NULL);
	const char* architectureString
		= get_driver_parameter(settingsHandle, "architecture", NULL, NULL);

	unload_driver_settings(settingsHandle);

	if (name == NULL || *name == '\0' || url == NULL || *url == '\0'
		|| vendor == NULL || *vendor == '\0'
		|| summary == NULL || *summary == '\0'
		|| priorityString == NULL || *priorityString == '\0'
		|| architectureString == NULL || *architectureString == '\0') {
		return B_BAD_DATA;
	}

	BPackageArchitecture architecture;
	if (BPackageInfo::GetArchitectureByName(architectureString, architecture)
			!= B_OK || architecture == B_PACKAGE_ARCHITECTURE_ANY) {
		return B_BAD_DATA;
	}

	fName = name;
	fOriginalBaseURL = url;
	fVendor = vendor;
	fSummary = summary;
	fPriority = atoi(priorityString);
	fArchitecture = architecture;

	fInitStatus = B_OK;

	return B_OK;
}


const BString&
BRepositoryInfo::Name() const
{
	return fName;
}


const BString&
BRepositoryInfo::OriginalBaseURL() const
{
	return fOriginalBaseURL;
}


const BString&
BRepositoryInfo::Vendor() const
{
	return fVendor;
}


const BString&
BRepositoryInfo::Summary() const
{
	return fSummary;
}


uint8
BRepositoryInfo::Priority() const
{
	return fPriority;
}


BPackageArchitecture
BRepositoryInfo::Architecture() const
{
	return fArchitecture;
}


const BStringList&
BRepositoryInfo::LicenseNames() const
{
	return fLicenseNames;
}


const BStringList&
BRepositoryInfo::LicenseTexts() const
{
	return fLicenseTexts;
}


void
BRepositoryInfo::SetName(const BString& name)
{
	fName = name;
}


void
BRepositoryInfo::SetOriginalBaseURL(const BString& url)
{
	fOriginalBaseURL = url;
}


void
BRepositoryInfo::SetVendor(const BString& vendor)
{
	fVendor = vendor;
}


void
BRepositoryInfo::SetSummary(const BString& summary)
{
	fSummary = summary;
}


void
BRepositoryInfo::SetPriority(uint8 priority)
{
	fPriority = priority;
}


void
BRepositoryInfo::SetArchitecture(BPackageArchitecture architecture)
{
	fArchitecture = architecture;
}


status_t
BRepositoryInfo::AddLicense(const BString& licenseName,
	const BString& licenseText)
{
	if (!fLicenseNames.Add(licenseName) || !fLicenseTexts.Add(licenseText))
		return B_NO_MEMORY;

	return B_OK;
}


void
BRepositoryInfo::ClearLicenses()
{
	fLicenseNames.MakeEmpty();
	fLicenseTexts.MakeEmpty();
}


}	// namespace BPackageKit
