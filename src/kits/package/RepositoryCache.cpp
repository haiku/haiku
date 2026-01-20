/*
 * Copyright 2011-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/RepositoryCache.h>

#include <stdio.h>

#include <new>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/RepositoryContentHandler.h>
#include <package/hpkg/RepositoryReader.h>
#include <package/hpkg/StandardErrorOutput.h>
#include <package/PackageInfo.h>
#include <package/RepositoryInfo.h>

#include <package/PackageInfoContentHandler.h>


namespace BPackageKit {


using namespace BHPKG;


// #pragma mark - RepositoryContentHandler


struct BRepositoryCache::RepositoryContentHandler : BRepositoryContentHandler {
	RepositoryContentHandler(BErrorOutput* errorOutput, BRepositoryInfo& repositoryInfo,
		GetPackageInfosCallback callback, void* callbackContext)
		:
		fRepositoryInfo(repositoryInfo),
		fPackageInfo(),
		fCallback(callback),
		fCallbackContext(callbackContext),
		fPackageInfoContentHandler(fPackageInfo, errorOutput)
	{
	}

	virtual status_t HandlePackage(const char* packageName)
	{
		fPackageInfo.Clear();
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return fPackageInfoContentHandler.HandlePackageAttribute(value);
	}

	virtual status_t HandlePackageDone(const char* packageName)
	{
		status_t result = fPackageInfo.InitCheck();
		if (result != B_OK)
			return result;

		if (fCallback == NULL)
			return B_CANCELED;

		if (!fCallback(fCallbackContext, fPackageInfo))
			return B_CANCELED;

		return B_OK;
	}

	virtual status_t HandleRepositoryInfo(const BRepositoryInfo& repositoryInfo)
	{
		fRepositoryInfo = repositoryInfo;
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	BRepositoryInfo&			fRepositoryInfo;
	BPackageInfo				fPackageInfo;
	GetPackageInfosCallback		fCallback;
	void*						fCallbackContext;

	BPackageInfoContentHandler	fPackageInfoContentHandler;
};


// #pragma mark - BRepositoryCache


BRepositoryCache::BRepositoryCache()
	:
	fIsUserSpecific(false)
{
}


BRepositoryCache::~BRepositoryCache()
{
}


const BEntry&
BRepositoryCache::Entry() const
{
	return fEntry;
}


const BRepositoryInfo&
BRepositoryCache::Info() const
{
	return fInfo;
}


bool
BRepositoryCache::IsUserSpecific() const
{
	return fIsUserSpecific;
}


status_t
BRepositoryCache::SetTo(const BEntry& entry)
{
	fEntry = entry;

	BPath repositoryCachePath;
	status_t result;
	if ((result = entry.GetPath(&repositoryCachePath)) != B_OK)
		return result;

	// read only info from repository cache
	if ((result = _ReadCache(repositoryCachePath, fInfo, NULL, NULL) != B_OK))
		return result;

	BPath userSettingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &userSettingsPath) == B_OK) {
		BDirectory userSettingsDir(userSettingsPath.Path());
		fIsUserSpecific = userSettingsDir.Contains(&entry);
	} else
		fIsUserSpecific = false;

	return B_OK;
}


status_t
BRepositoryCache::GetPackageInfos(GetPackageInfosCallback callback, void* context) const
{
	BPath repositoryCachePath;
	status_t result;
	if ((result = fEntry.GetPath(&repositoryCachePath)) != B_OK)
		return result;

	BRepositoryInfo dummy;
	return _ReadCache(repositoryCachePath, dummy, callback, context);
}


status_t
BRepositoryCache::_ReadCache(const BPath& repositoryCachePath,
	BRepositoryInfo& repositoryInfo, GetPackageInfosCallback callback, void* context) const
{
	status_t result;

	BStandardErrorOutput errorOutput;
	BRepositoryReader repositoryReader(&errorOutput);
	if ((result = repositoryReader.Init(repositoryCachePath.Path())) != B_OK)
		return result;

	RepositoryContentHandler handler(&errorOutput, repositoryInfo, callback, context);
	if ((result = repositoryReader.ParseContent(&handler)) != B_OK && result != B_CANCELED)
		return result;

	return B_OK;
}


}	// namespace BPackageKit
