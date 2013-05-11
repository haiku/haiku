/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "StringPool.h"

#include "DebugSupport.h"


static const size_t kInitialStringTableSize = 128;

static char sStringsBuffer[sizeof(StringDataHash)];

StringData StringData::fEmptyString(StringDataKey("", 0));

mutex StringPool::sLock;
StringDataHash* StringPool::sStrings;


// #pragma mark - StringData


/*static*/ void
StringData::Init()
{
	new(&fEmptyString) StringData(StringDataKey("", 0));
}


// #pragma mark - StringPool


/*static*/ status_t
StringPool::Init()
{
	sStrings = new(sStringsBuffer) StringDataHash;
	status_t error = sStrings->Init(kInitialStringTableSize);
	if (error != B_OK) {
		sStrings->~StringDataHash();
		sStrings = NULL;
		return error;
	}

	mutex_init(&sLock, "string pool");

	StringData::Init();
	sStrings->Insert(StringData::Empty());

	return B_OK;
}


/*static*/ void
StringPool::Cleanup()
{
	sStrings->Remove(StringData::Empty());

	sStrings->~StringDataHash();
	sStrings = NULL;

	mutex_destroy(&sLock);
}


/*static*/ inline StringData*
StringPool::_GetLocked(const StringDataKey& key)
{
	if (StringData* string = sStrings->Lookup(key)) {
		if (!string->AcquireReference())
			return string;

		// The object was fully dereferenced and will be deleted. Remove it
		// from the hash table, so it isn't in the way.
		sStrings->Remove(string);
	}

	return NULL;
}


/*static*/ StringData*
StringPool::Get(const char* string, size_t length)
{
	MutexLocker locker(sLock);
	StringDataKey key(string, length);
	StringData* data = _GetLocked(key);
	if (data != NULL)
		return data;

	locker.Unlock();

	StringData* newString = StringData::Create(key);
	if (newString == NULL)
		return NULL;

	locker.Lock();

	data = _GetLocked(key);
	if (data != NULL) {
		locker.Unlock();
		newString->Delete();
		return data;
	}

	sStrings->Insert(newString);
	return newString;
}


/*static*/ void
StringPool::LastReferenceReleased(StringData* data)
{
	MutexLocker locker(sLock);
	sStrings->Remove(data);
	locker.Unlock();
	data->Delete();
}


/*static*/ void
StringPool::DumpUsageStatistics()
{
	size_t unsharedStringCount = 0;
	size_t totalReferenceCount = 0;
	size_t totalStringSize = 0;
	size_t totalStringSizeWithDuplicates = 0;

	MutexLocker locker(sLock);
	for (StringDataHash::Iterator it = sStrings->GetIterator(); it.HasNext();) {
		StringData* data = it.Next();
		int32 referenceCount = data->CountReferences();
		totalReferenceCount += referenceCount;
		if (referenceCount == 1)
			unsharedStringCount++;

		size_t stringSize = strlen(data->String() + 1);
		totalStringSize += stringSize;
		totalStringSizeWithDuplicates += stringSize * referenceCount;
	}

	size_t stringCount = sStrings->CountElements();
	size_t overhead = stringCount * (sizeof(StringData) - 1);

	INFORM("StringPool usage:\n");
	INFORM("  total unique strings:    %8zu, %8zu bytes, overhead: %zu bytes\n",
		stringCount, totalStringSize, overhead);
	INFORM("  total strings with dups: %8zu, %8zu bytes\n", totalReferenceCount,
		totalStringSizeWithDuplicates);
	INFORM("  unshared strings:        %8zu\n", unsharedStringCount);
	INFORM("  bytes saved:             %8zd\n",
		(ssize_t)(totalStringSizeWithDuplicates - totalStringSize - overhead));
}
