/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/RepositoryCache.h>

#include <stdlib.h>

#include <new>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


namespace Haiku {

namespace Package {


RepositoryCache::RepositoryCache()
	:
	fInitStatus(B_NO_INIT),
	fIsUserSpecific(false)
{
}


RepositoryCache::RepositoryCache(const BEntry& entry)
{
	SetTo(entry);
}


RepositoryCache::~RepositoryCache()
{
}


status_t
RepositoryCache::InitCheck() const
{
	return fInitStatus;
}


const BEntry&
RepositoryCache::Entry() const
{
	return fEntry;
}


bool
RepositoryCache::IsUserSpecific() const
{
	return fIsUserSpecific;
}


void
RepositoryCache::SetIsUserSpecific(bool isUserSpecific)
{
	fIsUserSpecific = isUserSpecific;
}


status_t
RepositoryCache::SetTo(const BEntry& entry)
{
	fEntry = entry;
	fInitStatus = B_NO_INIT;

	BFile file(&entry, B_READ_ONLY);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	BMessage headerMsg;
	if ((result = headerMsg.Unflatten(&file)) != B_OK)
		return result;

	// TODO: unarchive header and read packages

	BPath userSettingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &userSettingsPath) == B_OK) {
		BDirectory userSettingsDir(userSettingsPath.Path());
		fIsUserSpecific = userSettingsDir.Contains(&entry);
	} else
		fIsUserSpecific = false;

	fInitStatus = B_OK;

	return B_OK;
}


}	// namespace Package

}	// namespace Haiku
