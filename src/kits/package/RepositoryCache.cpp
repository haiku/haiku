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
	RepositoryContentHandler(BRepositoryInfo& repositoryInfo,
		BPackageInfoSet& packages, BErrorOutput* errorOutput)
		:
		fRepositoryInfo(repositoryInfo),
		fPackageInfo(),
		fPackages(packages),
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

		result = fPackages.AddInfo(fPackageInfo);
		if (result != B_OK)
			return result;

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
	BPackageInfoSet&			fPackages;
	BPackageInfoContentHandler	fPackageInfoContentHandler;
};


// #pragma mark - BRepositoryCache


BRepositoryCache::BRepositoryCache()
	:
	fIsUserSpecific(false),
	fPackages()
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


void
BRepositoryCache::SetIsUserSpecific(bool isUserSpecific)
{
	fIsUserSpecific = isUserSpecific;
}


status_t
BRepositoryCache::SetTo(const BEntry& entry)
{
	// unset
	fPackages.MakeEmpty();
	fEntry.Unset();

	// get cache file path
	fEntry = entry;

	BPath repositoryCachePath;
	status_t result;
	if ((result = entry.GetPath(&repositoryCachePath)) != B_OK)
		return result;

	// read repository cache
	BStandardErrorOutput errorOutput;
	BRepositoryReader repositoryReader(&errorOutput);
	if ((result = repositoryReader.Init(repositoryCachePath.Path())) != B_OK)
		return result;

	RepositoryContentHandler handler(fInfo, fPackages, &errorOutput);
	if ((result = repositoryReader.ParseContent(&handler)) != B_OK)
		return result;

	BPath userSettingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &userSettingsPath) == B_OK) {
		BDirectory userSettingsDir(userSettingsPath.Path());
		fIsUserSpecific = userSettingsDir.Contains(&entry);
	} else
		fIsUserSpecific = false;

	return B_OK;
}


uint32
BRepositoryCache::CountPackages() const
{
	return fPackages.CountInfos();
}


BRepositoryCache::Iterator
BRepositoryCache::GetIterator() const
{
	return fPackages.GetIterator();
}


}	// namespace BPackageKit
