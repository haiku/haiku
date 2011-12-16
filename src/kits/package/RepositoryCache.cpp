/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
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
#include <package/PackageInfo.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {


using namespace BHPKG;


// #pragma mark - RepositoryContentHandler


struct BRepositoryCache::RepositoryContentHandler : BRepositoryContentHandler {
	RepositoryContentHandler(BRepositoryInfo* repositoryInfo,
		BPackageInfoSet& packages)
		:
		fRepositoryInfo(repositoryInfo),
		fPackages(packages)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		switch (value.attributeID) {
			case B_PACKAGE_INFO_NAME:
				fPackageInfo.SetName(value.string);
				break;

			case B_PACKAGE_INFO_SUMMARY:
				fPackageInfo.SetSummary(value.string);
				break;

			case B_PACKAGE_INFO_DESCRIPTION:
				fPackageInfo.SetDescription(value.string);
				break;

			case B_PACKAGE_INFO_VENDOR:
				fPackageInfo.SetVendor(value.string);
				break;

			case B_PACKAGE_INFO_PACKAGER:
				fPackageInfo.SetPackager(value.string);
				break;

			case B_PACKAGE_INFO_FLAGS:
				fPackageInfo.SetFlags(value.unsignedInt);
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				fPackageInfo.SetArchitecture(
					(BPackageArchitecture)value.unsignedInt);
				break;

			case B_PACKAGE_INFO_VERSION:
				fPackageInfo.SetVersion(value.version);
				break;

			case B_PACKAGE_INFO_COPYRIGHTS:
				fPackageInfo.AddCopyright(value.string);
				break;

			case B_PACKAGE_INFO_LICENSES:
				fPackageInfo.AddLicense(value.string);
				break;

			case B_PACKAGE_INFO_PROVIDES:
				fPackageInfo.AddProvides(value.resolvable);
				break;

			case B_PACKAGE_INFO_REQUIRES:
				fPackageInfo.AddRequires(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_SUPPLEMENTS:
				fPackageInfo.AddSupplements(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_CONFLICTS:
				fPackageInfo.AddConflicts(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_FRESHENS:
				fPackageInfo.AddFreshens(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_REPLACES:
				fPackageInfo.AddReplaces(value.string);
				break;

			case B_PACKAGE_INFO_URLS:
				fPackageInfo.AddURL(value.string);
				break;

			case B_PACKAGE_INFO_SOURCE_URLS:
				fPackageInfo.AddSourceURL(value.string);
				break;

			case B_PACKAGE_INFO_INSTALL_PATH:
				fPackageInfo.SetInstallPath(value.string);
				break;

			case B_PACKAGE_INFO_CHECKSUM:
			{
				fPackageInfo.SetChecksum(value.string);
				status_t result = fPackageInfo.InitCheck();
				if (result != B_OK)
					return result;

				result = fPackages.AddInfo(fPackageInfo);
				if (result != B_OK)
					return result;

				fPackageInfo.Clear();
				break;
			}

			default:
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual status_t HandleRepositoryInfo(const BRepositoryInfo& repositoryInfo)
	{
		*fRepositoryInfo = repositoryInfo;

		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	BRepositoryInfo*	fRepositoryInfo;
	BPackageInfo		fPackageInfo;
	BPackageInfoSet&	fPackages;
};


// #pragma mark - StandardErrorOutput


class BRepositoryCache::StandardErrorOutput : public BErrorOutput {
	virtual	void PrintErrorVarArgs(const char* format, va_list args)
	{
		vfprintf(stderr, format, args);
	}
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

	// init package info set
	status_t result = fPackages.Init();
	if (result != B_OK)
		return result;

	// get cache file path
	fEntry = entry;

	BPath repositoryCachePath;
	if ((result = entry.GetPath(&repositoryCachePath)) != B_OK)
		return result;

	// read repository cache
	StandardErrorOutput errorOutput;
	BRepositoryReader repositoryReader(&errorOutput);
	if ((result = repositoryReader.Init(repositoryCachePath.Path())) != B_OK)
		return result;

	RepositoryContentHandler handler(&fInfo, fPackages);
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
