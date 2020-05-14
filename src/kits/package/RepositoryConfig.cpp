/*
 * Copyright 2011-2020, Haiku, Inc. All Rights Reserved.
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

#define KEY_BASE_URL_V1 "url"
#define KEY_IDENTIFIER_V1 "url"

#define KEY_BASE_URL_V2 "baseurl"
#define KEY_IDENTIFIER_V2 "identifier"
#define KEY_IDENTIFIER_V2_ALT "url"
	// should not be used any more in favour of 'identifier'

#define KEY_PRIORITY "priority"
#define KEY_CONFIG_VERSION "cfgversion"

namespace BPackageKit {


// these are mappings of known legacy identifier URLs that are possibly
// still present in some installations.  These are in pairs; the first
// being the legacy URL and the next being the replacement.  This can
// be phased out over time.

static const char* kLegacyUrlMappings[] = {
	"https://eu.hpkg.haiku-os.org/haikuports/master/x86_gcc2/current",
	"https://hpkg.haiku-os.org/haikuports/master/x86_gcc2/current",
	"https://eu.hpkg.haiku-os.org/haikuports/master/x86_64/current",
	"https://hpkg.haiku-os.org/haikuports/master/x86_64/current",
	NULL,
	NULL
};


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
	configString << "# the url where the repository data can be "
		"accessed.\n";
	configString << KEY_BASE_URL_V2 << "=" << fBaseURL << "\n";
	configString << "\n";

	configString << "# an identifier for the repository that is "
		"consistent across mirrors\n";
	if (fIdentifier.IsEmpty())
		configString << "# " << KEY_IDENTIFIER_V2 << "=???\n";
	else
		configString << KEY_IDENTIFIER_V2 << "=" << fIdentifier << "\n";
	configString << "\n";

	configString << "# a deprecated copy of the ["
		<< KEY_IDENTIFIER_V2 << "] key above for older os versions\n";
	if (fIdentifier.IsEmpty())
		configString << "# " << KEY_IDENTIFIER_V2_ALT << "=???\n";
	else
		configString << KEY_IDENTIFIER_V2_ALT << "=" << fIdentifier << "\n";
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


static const char*
repository_config_swap_legacy_identifier_v1(const char* identifier)
{
	for (int32 i = 0; kLegacyUrlMappings[i] != NULL; i += 2) {
		if (strcmp(identifier, kLegacyUrlMappings[i]) == 0)
			return kLegacyUrlMappings[i + 1];
	}
	return identifier;
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

	const char* version = driverSettings.GetParameterValue(KEY_CONFIG_VERSION);
	int versionNumber = version == NULL ? 1 : atoi(version);
	const char *baseUrlKey;
	const char *identifierKeys[3] = { NULL, NULL, NULL };

	switch (versionNumber) {
		case 1:
			baseUrlKey = KEY_BASE_URL_V1;
			identifierKeys[0] = KEY_IDENTIFIER_V1;
			break;
		case 2:
			baseUrlKey = KEY_BASE_URL_V2;
			identifierKeys[0] = KEY_IDENTIFIER_V2;
			identifierKeys[1] = KEY_IDENTIFIER_V2_ALT;
			break;
		default:
			return B_BAD_DATA;
	}

	const char* baseUrl = driverSettings.GetParameterValue(baseUrlKey);
	const char* priorityString = driverSettings.GetParameterValue(KEY_PRIORITY);
	const char* identifier = NULL;

	for (int32 i = 0; identifier == NULL && identifierKeys[i] != NULL; i++)
		identifier = driverSettings.GetParameterValue(identifierKeys[i]);

	if (baseUrl == NULL || *baseUrl == '\0')
		return B_BAD_DATA;
	if (identifier == NULL || *identifier == '\0')
		return B_BAD_DATA;

	if (versionNumber == 1)
		identifier = repository_config_swap_legacy_identifier_v1(identifier);

	fName = entry.Name();
	fBaseURL = baseUrl;
	fPriority = priorityString == NULL
		? kUnsetPriority : atoi(priorityString);
	fIdentifier = identifier;

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
