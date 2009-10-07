/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "GlobalTypeLookup.h"

#include <new>

#include <String.h>

#include "StringUtils.h"
#include "Type.h"


struct GlobalTypeLookupContext::TypeEntry {
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


struct GlobalTypeLookupContext::TypeEntryHashDefinition {
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


// #pragma mark - GlobalTypeLookupContext


GlobalTypeLookupContext::GlobalTypeLookupContext()
	:
	fLock("global type lookup"),
	fCachedTypes(NULL)
{
}


GlobalTypeLookupContext::~GlobalTypeLookupContext()
{
	// release all cached type references
	if (fCachedTypes != NULL) {
		TypeEntry* entry = fCachedTypes->Clear(true);
		while (entry != NULL) {
			TypeEntry* nextEntry = entry->fNext;
			delete entry;
			entry = nextEntry;
		}
	}
}


status_t
GlobalTypeLookupContext::Init()
{
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	fCachedTypes = new(std::nothrow) TypeTable;
	if (fCachedTypes == NULL)
		return B_NO_MEMORY;

	return fCachedTypes->Init();
}


Type*
GlobalTypeLookupContext::CachedType(const BString& name) const
{
	TypeEntry* typeEntry = fCachedTypes->Lookup(name);
	return typeEntry != NULL ? typeEntry->type : NULL;
}


status_t
GlobalTypeLookupContext::AddCachedType(const BString& name, Type* type)
{
	TypeEntry* typeEntry = fCachedTypes->Lookup(name);
	if (typeEntry != NULL)
		return B_BAD_VALUE;

	typeEntry = new(std::nothrow) TypeEntry(name, type);
	if (typeEntry == NULL)
		return B_NO_MEMORY;

	fCachedTypes->Insert(typeEntry);
	return B_OK;
}


void
GlobalTypeLookupContext::RemoveCachedType(const BString& name)
{
	if (TypeEntry* typeEntry = fCachedTypes->Lookup(name)) {
		fCachedTypes->Remove(typeEntry);
		delete typeEntry;
	}
}


// #pragma mark - GlobalTypeLookup


GlobalTypeLookup::~GlobalTypeLookup()
{
}
