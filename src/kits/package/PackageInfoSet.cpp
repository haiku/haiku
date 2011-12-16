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


struct BPackageInfoSet::PackageMap
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


// #pragma mark - Iterator


BPackageInfoSet::Iterator::Iterator()
	:
	fSet(NULL),
	fNextInfo(NULL)
{
}


BPackageInfoSet::Iterator::Iterator(const BPackageInfoSet* set)
	:
	fSet(set),
	fNextInfo(fSet->fPackageMap->GetIterator().Next())
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
				= fSet->fPackageMap->GetIterator(fNextInfo->Name());
			iterator.Next();
			fNextInfo = iterator.Next();
		}
	}

	return result;
}


// #pragma mark - BPackageInfoSet


BPackageInfoSet::BPackageInfoSet()
	:
	fPackageMap(new(std::nothrow) PackageMap)
{
}


BPackageInfoSet::~BPackageInfoSet()
{
	MakeEmpty();
	delete fPackageMap;
}


status_t
BPackageInfoSet::Init()
{
	return fPackageMap->Init();
}


status_t
BPackageInfoSet::AddInfo(const BPackageInfo& _info)
{
	if (fPackageMap == NULL)
		return B_NO_INIT;

	PackageInfo* info = new(std::nothrow) PackageInfo(_info);
	if (info == NULL)
		return B_NO_MEMORY;

	status_t error = info->InitCheck();
	if (error != B_OK) {
		delete info;
		return error;
	}

	if (PackageInfo* oldInfo = fPackageMap->Lookup(info->Name())) {
		// TODO: Check duplicates?
		info->listNext = oldInfo->listNext;
		oldInfo->listNext = info;
	} else
		fPackageMap->Insert(info);

	return B_OK;
}


void
BPackageInfoSet::MakeEmpty()
{
	if (fPackageMap == NULL)
		return;

	PackageInfo* info = fPackageMap->Clear(true);
	while (info != NULL) {
		PackageInfo* next = info->hashNext;
		delete info;
		info = next;
	}
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
	return Iterator(this);
}


}	// namespace BPackageKit
