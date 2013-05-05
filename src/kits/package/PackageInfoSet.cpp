/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/PackageInfoSet.h>

#include <new>

#include <Referenceable.h>

#include <AutoDeleter.h>

#include <util/OpenHashTable.h>

#include <package/PackageInfo.h>


namespace BPackageKit {


// #pragma mark - PackageInfo


struct BPackageInfoSet::PackageInfo : public BPackageInfo {
	PackageInfo*	hashNext;
	PackageInfo*	listNext;

	PackageInfo(const BPackageInfo& other)
		:
		BPackageInfo(other),
		listNext(NULL)
	{
	}

	void DeleteList()
	{
		PackageInfo* info = this;
		while (info != NULL) {
			PackageInfo* next = info->listNext;
			delete info;
			info = next;
		}
	}
};


// #pragma mark - PackageInfoHashDefinition


struct BPackageInfoSet::PackageInfoHashDefinition {
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


struct BPackageInfoSet::PackageMap : public BReferenceable,
	public BOpenHashTable<PackageInfoHashDefinition> {

	PackageMap()
		:
		fCount(0)
	{
	}

	~PackageMap()
	{
		DeleteAllPackageInfos();
	}

	static PackageMap* Create()
	{
		PackageMap* map = new(std::nothrow) PackageMap;
		if (map == NULL || map->Init() != B_OK) {
			delete map;
			return NULL;
		}

		return map;
	}

	PackageMap* Clone() const
	{
		PackageMap* newMap = Create();
		if (newMap == NULL)
			return NULL;
		ObjectDeleter<PackageMap> newMapDeleter(newMap);

		for (BPackageInfoSet::Iterator it(this); it.HasNext();) {
			const BPackageInfo* info = it.Next();
			if (newMap->AddNewPackageInfo(*info) != B_OK)
				return NULL;
		}

		return newMapDeleter.Detach();
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

	status_t AddNewPackageInfo(const BPackageInfo& oldInfo)
	{
		PackageInfo* info = new(std::nothrow) PackageInfo(oldInfo);
		if (info == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<PackageInfo> infoDeleter(info);

		status_t error = info->InitCheck();
		if (error != B_OK)
			return error;

		AddPackageInfo(infoDeleter.Detach());

		return B_OK;
	}

	void DeleteAllPackageInfos()
	{
		PackageInfo* info = Clear(true);
		while (info != NULL) {
			PackageInfo* next = info->hashNext;
			info->DeleteList();
			info = next;
		}
	}

	uint32 CountPackageInfos() const
	{
		return fCount;
	}

private:
	uint32	fCount;
};


// #pragma mark - Iterator


BPackageInfoSet::Iterator::Iterator(const PackageMap* map)
	:
	fMap(map),
	fNextInfo(map != NULL ? map->GetIterator().Next() : NULL)
{
}


bool
BPackageInfoSet::Iterator::HasNext() const
{
	return fNextInfo != NULL;
}


const BPackageInfo*
BPackageInfoSet::Iterator::Next()
{
	BPackageInfo* result = fNextInfo;

	if (fNextInfo != NULL) {
		if (fNextInfo->listNext != NULL) {
			// get next in list
			fNextInfo = fNextInfo->listNext;
		} else {
			// get next in hash table
			PackageMap::Iterator iterator
				= fMap->GetIterator(fNextInfo->Name());
			iterator.Next();
			fNextInfo = iterator.Next();
		}
	}

	return result;
}


// #pragma mark - BPackageInfoSet


BPackageInfoSet::BPackageInfoSet()
	:
	fPackageMap(NULL)
{
}


BPackageInfoSet::~BPackageInfoSet()
{
	if (fPackageMap != NULL)
		fPackageMap->ReleaseReference();
}


BPackageInfoSet::BPackageInfoSet(const BPackageInfoSet& other)
	:
	fPackageMap(other.fPackageMap)
{
	if (fPackageMap != NULL)
		fPackageMap->AcquireReference();
}


status_t
BPackageInfoSet::AddInfo(const BPackageInfo& info)
{
	if (!_CopyOnWrite())
		return B_NO_MEMORY;

	return fPackageMap->AddNewPackageInfo(info);
}


void
BPackageInfoSet::MakeEmpty()
{
	if (fPackageMap == NULL || fPackageMap->CountPackageInfos() == 0)
		return;

	// If our map is shared, just set it to NULL.
	if (fPackageMap->CountReferences() != 1) {
		fPackageMap->ReleaseReference();
		fPackageMap = NULL;
		return;
	}

	// Our map is not shared -- make it empty.
	fPackageMap->DeleteAllPackageInfos();
}


uint32
BPackageInfoSet::CountInfos() const
{
	if (fPackageMap == NULL)
		return 0;

	return fPackageMap->CountPackageInfos();
}


BPackageInfoSet::Iterator
BPackageInfoSet::GetIterator() const
{
	return Iterator(fPackageMap);
}


BPackageInfoSet&
BPackageInfoSet::operator=(const BPackageInfoSet& other)
{
	if (other.fPackageMap == fPackageMap)
		return *this;

	if (fPackageMap != NULL)
		fPackageMap->ReleaseReference();

	fPackageMap = other.fPackageMap;

	if (fPackageMap != NULL)
		fPackageMap->AcquireReference();

	return *this;
}


bool
BPackageInfoSet::_CopyOnWrite()
{
	if (fPackageMap == NULL) {
		fPackageMap = PackageMap::Create();
		return fPackageMap != NULL;
	}

	if (fPackageMap->CountReferences() == 1)
		return true;

	PackageMap* newMap = fPackageMap->Clone();
	if (newMap == NULL)
		return false;

	fPackageMap->ReleaseReference();
	fPackageMap = newMap;
	return true;
}


}	// namespace BPackageKit
