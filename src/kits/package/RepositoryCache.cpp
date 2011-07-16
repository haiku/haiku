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

#include <util/OpenHashTable.h>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/RepositoryContentHandler.h>
#include <package/hpkg/RepositoryReader.h>
#include <package/HashableString.h>
#include <package/PackageInfo.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {


using BPrivate::HashableString;
using namespace BHPKG;


// #pragma mark - PackageInfo


struct BRepositoryCache::PackageInfo : public BPackageInfo {
	PackageInfo*	hashNext;
	PackageInfo*	listNext;

	PackageInfo(const BPackageInfo& other)
		:
		BPackageInfo(other),
		listNext(NULL)
	{
	}
};


// #pragma mark - PackageInfoHashDefinition


struct BRepositoryCache::PackageInfoHashDefinition {
	typedef const char*		KeyType;
	typedef	PackageInfo		ValueType;

	size_t HashKey(const char* key) const
	{
		return BString::HashValue(key);
	}

	size_t Hash(const PackageInfo* value) const
	{
		return value->Name().HashValue();
	}

	bool Compare(const char* key, const PackageInfo* value) const
	{
		return value->Name() == key;
	}

	PackageInfo*& GetLink(PackageInfo* value) const
	{
		return value->hashNext;
	}
};


// #pragma mark - PackageMap


struct BRepositoryCache::PackageMap
	: public BOpenHashTable<PackageInfoHashDefinition> {

	PackageMap()
		:
		fCount(0)
	{
	}

	~PackageMap()
	{
		PackageInfo* info = Clear(true);
		while (info != NULL) {
			PackageInfo* next = info->hashNext;
			delete info;
			info = next;
		}
	}

	void AddPackageInfo(PackageInfo* info)
	{
		if (PackageInfo* oldInfo = Lookup(info->Name())) {
			info->listNext = oldInfo->listNext;
			oldInfo->listNext = info;
		} else
			Insert(info);

		fCount++;
	}

	uint32 CountPackageInfos() const
	{
		return fCount;
	}

private:
	uint32	fCount;
};


// #pragma mark - RepositoryContentHandler


struct BRepositoryCache::RepositoryContentHandler : BRepositoryContentHandler {
	RepositoryContentHandler(BRepositoryInfo* repositoryInfo,
		PackageMap* packageMap)
		:
		fRepositoryInfo(repositoryInfo),
		fPackageMap(packageMap)
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

				PackageInfo* info = new(std::nothrow) PackageInfo(fPackageInfo);
				if (info == NULL)
					return B_NO_MEMORY;

				result = info->InitCheck();
				if (result != B_OK) {
					delete info;
					return result;
				}

				if (PackageInfo* oldInfo = fPackageMap->Lookup(info->Name())) {
					info->listNext = oldInfo->listNext;
					oldInfo->listNext = info;
				} else
					fPackageMap->Insert(info);

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
	PackageMap*			fPackageMap;
};


// #pragma mark - StandardErrorOutput


class BRepositoryCache::StandardErrorOutput : public BErrorOutput {
	virtual	void PrintErrorVarArgs(const char* format, va_list args)
	{
		vfprintf(stderr, format, args);
	}
};


// #pragma mark - Iterator


BRepositoryCache::Iterator::Iterator()
	:
	fCache(NULL),
	fNextInfo(NULL)
{
}


BRepositoryCache::Iterator::Iterator(const BRepositoryCache* cache)
	:
	fCache(cache),
	fNextInfo(fCache->fPackageMap->GetIterator().Next())
{
}

bool
BRepositoryCache::Iterator::HasNext() const
{
	return fNextInfo != NULL;
}


const BPackageInfo*
BRepositoryCache::Iterator::Next()
{
	BPackageInfo* result = fNextInfo;

	if (fNextInfo != NULL) {
		if (fNextInfo->listNext != NULL) {
			// get next in list
			fNextInfo = fNextInfo->listNext;
		} else {
			// get next in hash table
			PackageMap::Iterator iterator
				= fCache->fPackageMap->GetIterator(fNextInfo->Name());
			iterator.Next();
			fNextInfo = iterator.Next();
		}
	}

	return result;
}


// #pragma mark - BRepositoryCache


BRepositoryCache::BRepositoryCache()
	:
	fIsUserSpecific(false),
	fPackageMap(NULL)
{
}


BRepositoryCache::~BRepositoryCache()
{
	delete fPackageMap;
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
	if (fPackageMap != NULL) {
		delete fPackageMap;
		fPackageMap = NULL;
	}

	fEntry.Unset();

	// create the package map
	fPackageMap = new (std::nothrow) PackageMap;
	if (fPackageMap == NULL)
		return B_NO_MEMORY;

	status_t result = fPackageMap->Init();
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

	RepositoryContentHandler handler(&fInfo, fPackageMap);
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
	if (fPackageMap == NULL)
		return 0;

	return fPackageMap->CountPackageInfos();
}


BRepositoryCache::Iterator
BRepositoryCache::GetIterator() const
{
	return Iterator(this);
}


}	// namespace BPackageKit
