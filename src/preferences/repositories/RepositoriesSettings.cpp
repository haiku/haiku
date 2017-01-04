/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */


#include "RepositoriesSettings.h"

#include <FindDirectory.h>
#include <StringList.h>

#include "constants.h"

const char* settingsFilename = "Repositories_settings";


RepositoriesSettings::RepositoriesSettings()
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &fFilePath);
	if (status == B_OK)
		status = fFilePath.Append(settingsFilename);
	BEntry fileEntry(fFilePath.Path());
	if (!fileEntry.Exists()) {
		// Create default repos
		BStringList nameList, urlList;
		int32 count = (sizeof(kDefaultRepos) / sizeof(Repository));
		for (int16 index = 0; index < count; index++) {
			nameList.Add(kDefaultRepos[index].name);
			urlList.Add(kDefaultRepos[index].url);
		}
		SetRepositories(nameList, urlList);
	}
	fInitStatus = status;
}


BRect
RepositoriesSettings::GetFrame()
{
	BMessage settings(_ReadFromFile());
	BRect frame;
	status_t status = settings.FindRect(key_frame, &frame);
	// Set default off screen so it will center itself
	if (status != B_OK)
		frame.Set(-10, -10, 750, 300);
	return frame;
}


void
RepositoriesSettings::SetFrame(BRect frame)
{
	BMessage settings(_ReadFromFile());
	settings.RemoveData(key_frame);
	settings.AddRect(key_frame, frame);
	_SaveToFile(settings);
}


status_t
RepositoriesSettings::GetRepositories(int32& repoCount, BStringList& nameList,
	BStringList& urlList)
{
	BMessage settings(_ReadFromFile());
	type_code type;
	int32 count;
	settings.GetInfo(key_name, &type, &count);

	status_t result = B_OK;
	int32 index, total = 0;
	BString foundName, foundUrl;
	// get each repository and add to lists
	for (index = 0; index < count; index++) {
		status_t result1 = settings.FindString(key_name, index, &foundName);
		status_t result2 = settings.FindString(key_url, index, &foundUrl);
		if (result1 == B_OK && result2 == B_OK) {
			nameList.Add(foundName);
			urlList.Add(foundUrl);
			total++;
		} else
			result = B_ERROR;
	}
	repoCount = total;
	return result;
}


void
RepositoriesSettings::SetRepositories(BStringList& nameList, BStringList& urlList)
{
	BMessage settings(_ReadFromFile());
	settings.RemoveName(key_name);
	settings.RemoveName(key_url);

	int32 index, count = nameList.CountStrings();
	for (index = 0; index < count; index++) {
		settings.AddString(key_name, nameList.StringAt(index));
		settings.AddString(key_url, urlList.StringAt(index));
	}
	_SaveToFile(settings);
}


BMessage
RepositoriesSettings::_ReadFromFile()
{
	BMessage settings;
	status_t status = fFile.SetTo(fFilePath.Path(), B_READ_ONLY);
	if (status == B_OK)
		status = settings.Unflatten(&fFile);
	fFile.Unset();
	return settings;
}


status_t
RepositoriesSettings::_SaveToFile(BMessage settings)
{
	status_t status = fFile.SetTo(fFilePath.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status == B_OK)
		status = settings.Flatten(&fFile);
	fFile.Unset();
	return status;
}
