/*
 * Copyright 2011-2018, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 */


#include <package/RepositoryConfig.h>

#include <stdlib.h>

#include <new>

#include <Directory.h>
#include <driver_settings.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include <DriverSettings.h>


#define STORAGE_VERSION 2

#define KEY_BASE_URL "baseurl"
#define KEY_BASE_URL_LEGACY "url"
	// deprecated
#define KEY_URL "url"
#define KEY_PRIORITY "priority"
#define KEY_CONFIG_VERSION "cfgversion"

namespace BPackageKit {


BRepositoryConfig::BRepositoryConfig()
	:
	fInitStatus(B_NO_INIT),
	fPriority(kUnsetPriority),
	fIsUserSpecific(false)
{
}


BRepositoryConfig::BRepositoryConfig(const BEntry& entry)
{
	SetTo(entry);
}


BRepositoryConfig::BRepositoryConfig(const BString& name,
	const BString& baseURL, uint8 priority)
	:
	fInitStatus(B_OK),
	fName(name),
	fBaseURL(baseURL),
	fPriority(priority),
	fIsUserSpecific(false)
{
}


BRepositoryConfig::~BRepositoryConfig()
{
}


status_t
BRepositoryConfig::Store(const BEntry& entry) const
{
	BFile file(&entry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	BString configString;
	configString << KEY_CONFIG_VERSION << "=" << STORAGE_VERSION << "\n";
	configString << "\n";
	configString << "# This is the URL where the repository data can be "
		"accessed.\n";
	configString << KEY_BASE_URL << "=" << fBaseURL << "\n";
	configString << "\n";
	configString << "# This URL is an identifier for the repository that is "
		"consistent across mirrors\n";

	if (fIdentifier.IsEmpty())
		configString << "# " << KEY_URL << "=???\n";
	else
		configString << KEY_URL << "=" << fIdentifier << "\n";
	configString << "\n";
	configString << KEY_PRIORITY << "=" << fPriority << "\n";

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

	entry_ref ref;
	status_t result = entry.GetRef(&ref);
	if (result != B_OK)
		return result;

	BDriverSettings driverSettings;
	result = driverSettings.Load(ref);
	if (result != B_OK)
		return result;

	const char* identifier = NULL;
	const char* version = driverSettings.GetParameterValue(KEY_CONFIG_VERSION);
	const char *baseUrlKey = KEY_BASE_URL;

	if (version == NULL || atoi(version) < 2)
		baseUrlKey = KEY_BASE_URL_LEGACY;
	else
		identifier = driverSettings.GetParameterValue(KEY_URL);

	const char* baseUrl = driverSettings.GetParameterValue(baseUrlKey);
	const char* priorityString = driverSettings.GetParameterValue(KEY_PRIORITY);

	if (baseUrl == NULL || *baseUrl == '\0')
		return B_BAD_DATA;

	fName = entry.Name();
	fBaseURL = baseUrl;
	fPriority = priorityString == NULL
		? kUnsetPriority : atoi(priorityString);
	fIdentifier = identifier == NULL ? "" : identifier;

	BPath userSettingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &userSettingsPath) == B_OK) {
		BDirectory userSettingsDir(userSettingsPath.Path());
		fIsUserSpecific = userSettingsDir.Contains(&entry);
	} else
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
BRepositoryConfig::BaseURL() const
{
	return fBaseURL;
}


const BString&
BRepositoryConfig::Identifier() const
{
	return fIdentifier;
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


BString
BRepositoryConfig::PackagesURL() const
{
	if (fBaseURL.IsEmpty())
		return BString();
	return BString().SetToFormat("%s/packages", fBaseURL.String());
}


void
BRepositoryConfig::SetName(const BString& name)
{
	fName = name;
}


void
BRepositoryConfig::SetBaseURL(const BString& baseURL)
{
	fBaseURL = baseURL;
}


void
BRepositoryConfig::SetIdentifier(const BString& identifier)
{
	fIdentifier = identifier;
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
