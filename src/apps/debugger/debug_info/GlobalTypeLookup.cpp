/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "GlobalTypeLookup.h"

#include <new>

#include <String.h>

#include <AutoLocker.h>

#include "StringUtils.h"
#include "Type.h"


struct GlobalTypeCache::TypeEntry {
	BString		name;
	Type*		type;
	TypeEntry*	fNext;

	TypeEntry(const BString& name, Type* type)
		:
		name(name),
		type(type)
	{
		type->AcquireReference();
	}

	~TypeEntry()
	{
		type->ReleaseReference();
	}
};


struct GlobalTypeCache::TypeEntryHashDefinition {
	typedef const BString	KeyType;
	typedef	TypeEntry		ValueType;

	size_t HashKey(const BString& key) const
	{
		return StringUtils::HashValue(key);
	}

	size_t Hash(const TypeEntry* value) const
	{
		return HashKey(value->name);
	}

	bool Compare(const BString& key, const TypeEntry* value) const
	{
		return key == value->name;
	}

	TypeEntry*& GetLink(TypeEntry* value) const
	{
		return value->fNext;
	}
};


// #pragma mark - GlobalTypeCache


GlobalTypeCache::GlobalTypeCache()
	:
	fLock("global type lookup"),
	fTypes(NULL)
{
}


GlobalTypeCache::~GlobalTypeCache()
{
	// release all cached type references
	if (fTypes != NULL) {
		TypeEntry* entry = fTypes->Clear(true);
		while (entry != NULL) {
			TypeEntry* nextEntry = entry->fNext;
			delete entry;
			entry = nextEntry;
		}
	}
}


status_t
GlobalTypeCache::Init()
{
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	fTypes = new(std::nothrow) TypeTable;
	if (fTypes == NULL)
		return B_NO_MEMORY;

	return fTypes->Init();
}


Type*
GlobalTypeCache::GetType(const BString& name) const
{
	TypeEntry* typeEntry = fTypes->Lookup(name);
	return typeEntry != NULL ? typeEntry->type : NULL;
}


status_t
GlobalTypeCache::AddType(const BString& name, Type* type)
{
	TypeEntry* typeEntry = fTypes->Lookup(name);
	if (typeEntry != NULL)
		return B_BAD_VALUE;

	typeEntry = new(std::nothrow) TypeEntry(name, type);
	if (typeEntry == NULL)
		return B_NO_MEMORY;

	fTypes->Insert(typeEntry);
	return B_OK;
}


void
GlobalTypeCache::RemoveType(const BString& name)
{
	if (TypeEntry* typeEntry = fTypes->Lookup(name)) {
		fTypes->Remove(typeEntry);
		delete typeEntry;
	}
}


void
GlobalTypeCache::RemoveTypes(image_id imageID)
{
	AutoLocker<GlobalTypeCache> locker(this);

	for (TypeTable::Iterator it = fTypes->GetIterator();
			TypeEntry* typeEntry = it.Next();) {
		if (typeEntry->type->ImageID() == imageID) {
			fTypes->RemoveUnchecked(typeEntry);
			delete typeEntry;
		}
	}
}


// #pragma mark - GlobalTypeLookup


GlobalTypeLookup::~GlobalTypeLookup()
{
}
