/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_POOL_H
#define STRING_POOL_H


#include <SupportDefs.h>

#include <stdlib.h>
#include <string.h>

#include <new>

#include <util/AutoLock.h>
#include <util/OpenHashTable.h>
#include <util/StringHash.h>


class StringData;


class StringDataKey {
public:
	StringDataKey(const char* string, size_t length)
		:
		fString(string),
		fLength(length),
		fHash(hash_hash_string_part(string, length))
	{
	}

	const char* String() const
	{
		return fString;
	}

	size_t Length() const
	{
		return fLength;
	}

	uint32 Hash() const
	{
		return fHash;
	}

private:
	const char*	fString;
	size_t		fLength;
	uint32		fHash;
};


struct StringDataHashDefinition;
typedef BOpenHashTable<StringDataHashDefinition> StringDataHash;


class StringPool {
public:
	static	status_t			Init();
	static	void				Cleanup();

	static	StringData*			Get(const char* string, size_t length);
	static	void				LastReferenceReleased(StringData* data);

	static	void				DumpUsageStatistics();

private:
	static	StringData*			_GetLocked(const StringDataKey& key);

private:
	static	mutex				sLock;
	static	StringDataHash*		sStrings;
};


class StringData {
public:
	static void Init();

	static StringData* Create(const StringDataKey& key)
	{
		void* data = malloc(sizeof(StringData) + key.Length());
		if (data == NULL)
			return NULL;

		return new(data) StringData(key);
	}

	static StringData* Empty()
	{
		return &fEmptyString;
	}

	static StringData* GetEmpty()
	{
		fEmptyString.AcquireReference();
		return &fEmptyString;
	}

	void Delete()
	{
		free(this);
	}

	bool AcquireReference()
	{
		return atomic_add(&fReferenceCount, 1) == 0;
	}

	void ReleaseReference()
	{
		if (atomic_add(&fReferenceCount, -1) == 1)
			StringPool::LastReferenceReleased(this);
	}

	// debugging only
	int32 CountReferences() const
	{
		return fReferenceCount;
	}

	const char* String() const
	{
		return fString;
	}

	uint32 Hash() const
	{
		return fHash;
	}

	StringData*& HashNext()
	{
		return fHashNext;
	}

private:
	StringData(const StringDataKey& key)
		:
		fReferenceCount(1),
		fHash(key.Hash())
	{
		memcpy(fString, key.String(), key.Length());
		fString[key.Length()] = '\0';
	}

	~StringData()
	{
	}

private:
	static StringData	fEmptyString;

	StringData*	fHashNext;
	int32		fReferenceCount;
	uint32		fHash;
	char		fString[1];
};


struct StringDataHashDefinition {
	typedef StringDataKey	KeyType;
	typedef	StringData		ValueType;

	size_t HashKey(const StringDataKey& key) const
	{
		return key.Hash();
	}

	size_t Hash(const StringData* value) const
	{
		return value->Hash();
	}

	bool Compare(const StringDataKey& key, const StringData* value) const
	{
		return key.Hash() == value->Hash()
			&& strncmp(value->String(), key.String(), key.Length()) == 0
			&& value->String()[key.Length()] == '\0';
	}

	StringData*& GetLink(StringData* value) const
	{
		return value->HashNext();
	}
};


#endif	// STRING_POOL_H
