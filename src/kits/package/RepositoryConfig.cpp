/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/RepositoryConfig.h>

#include <stdlib.h>

#include <new>

#include <Directory.h>
#include <driver_settings.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


namespace BPackageKit {


const uint8 BRepositoryConfig::kDefaultPriority	= 50;
const char* BRepositoryConfig::kNameField 		= "name";
const char* BRepositoryConfig::kURLField 		= "url";
const char* BRepositoryConfig::kPriorityField 	= "priority";


BRepositoryConfig::BRepositoryConfig()
	:
	fInitStatus(B_NO_INIT),
	fPriority(kDefaultPriority),
	fIsUserSpecific(false)
{
}


BRepositoryConfig::BRepositoryConfig(const BEntry& entry)
{
	SetTo(entry);
}


BRepositoryConfig::BRepositoryConfig(BMessage* data)
	:
	inherited(data)
{
	SetTo(data);
}


BRepositoryConfig::BRepositoryConfig(const BString& name, const BString& url,
	uint8 priority)
	:
	fInitStatus(B_OK),
	fName(name),
	fURL(url),
	fPriority(priority),
	fIsUserSpecific(false)
{
}


BRepositoryConfig::~BRepositoryConfig()
{
}


/*static*/ BRepositoryConfig*
BRepositoryConfig::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BPackageKit::BRepositoryConfig"))
		return new (std::nothrow) BRepositoryConfig(data);

	return NULL;
}


status_t
BRepositoryConfig::Archive(BMessage* data, bool deep) const
{
	status_t result = inherited::Archive(data, deep);
	if (result != B_OK)
		return result;

	if ((result = data->AddString(kNameField, fName)) != B_OK)
		return result;
	if ((result = data->AddString(kURLField, fURL)) != B_OK)
		return result;
	if ((result = data->AddUInt8(kPriorityField, fPriority)) != B_OK)
		return result;

	return B_OK;
}


status_t
BRepositoryConfig::StoreAsConfigFile(const BEntry& entry) const
{
	BFile file(&entry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	BString configString;
	configString
		<< "url=" << fURL << "\n"
		<< "priority=" << fPriority << "\n";

	int32 size = configString.Length();
	if ((result = file.Write(configString.String(), size)) < size)
		return (result >= 0) ? B_ERROR : result;

	return B_OK;
}


status_t
BRepositoryConfig::InitCheck() const
{
	return fInitStatus;
}


status_t
BRepositoryConfig::SetTo(const BEntry& entry)
{
	fEntry = entry;
	fInitStatus = B_NO_INIT;

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

	if ((result = file.Read(buffer, size)) < size)
		return (result >= 0) ? B_IO_ERROR : result;

	buffer[size] = '\0';
	configString.UnlockBuffer(size);

	void* settingsHandle = parse_driver_settings_string(configString.String());
	if (settingsHandle == NULL)
		return B_BAD_DATA;

	const char* url = get_driver_parameter(settingsHandle, "url", NULL, NULL);
	const char* priorityString
		= get_driver_parameter(settingsHandle, "priority", NULL, NULL);

	unload_driver_settings(settingsHandle);

	if (url == NULL || *url == '\0')
		return B_BAD_DATA;

	char name[B_FILE_NAME_LENGTH];
	if ((result = entry.GetName(name)) != B_OK)
		return result;

	fName = name;
	fURL = url;
	fPriority = priorityString == NULL
		? kDefaultPriority : atoi(priorityString);

	BPath userSettingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &userSettingsPath) == B_OK) {
		BDirectory userSettingsDir(userSettingsPath.Path());
		fIsUserSpecific = userSettingsDir.Contains(&entry);
	} else
		fIsUserSpecific = false;

	fInitStatus = B_OK;

	return B_OK;
}


status_t
BRepositoryConfig::SetTo(const BMessage* data)
{
	fInitStatus = B_NO_INIT;

	if (data == NULL)
		return B_BAD_VALUE;

	status_t result;
	if ((result = data->FindString(kNameField, &fName)) != B_OK)
		return result;
	if ((result = data->FindString(kURLField, &fURL)) != B_OK)
		return result;
	if ((result = data->FindUInt8(kPriorityField, &fPriority)) != B_OK)
		return result;

	fIsUserSpecific = false;

	fInitStatus = B_OK;

	return B_OK;
}


const BString&
BRepositoryConfig::Name() const
{
	return fName;
}


const BString&
BRepositoryConfig::URL() const
{
	return fURL;
}


uint8
BRepositoryConfig::Priority() const
{
	return fPriority;
}


bool
BRepositoryConfig::IsUserSpecific() const
{
	return fIsUserSpecific;
}


const BEntry&
BRepositoryConfig::Entry() const
{
	return fEntry;
}


void
BRepositoryConfig::SetName(const BString& name)
{
	fName = name;
}


void
BRepositoryConfig::SetURL(const BString& url)
{
	fURL = url;
}


void
BRepositoryConfig::SetPriority(uint8 priority)
{
	fPriority = priority;
}


void
BRepositoryConfig::SetIsUserSpecific(bool isUserSpecific)
{
	fIsUserSpecific = isUserSpecific;
}


}	// namespace BPackageKit
