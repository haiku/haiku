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
#include "TypeLookupConstraints.h"


struct GlobalTypeCache::TypeEntry {
	Type*		type;
	TypeEntry*	fNextByName;
	TypeEntry*	fNextByID;

	TypeEntry(Type* type)
		:
		type(type)
	{
		type->AcquireReference();
	}

	~TypeEntry()
	{
		type->ReleaseReference();
	}
};


struct GlobalTypeCache::TypeEntryHashDefinitionByName {
	typedef const BString	KeyType;
	typedef	TypeEntry		ValueType;

	size_t HashKey(const BString& key) const
	{
		return StringUtils::HashValue(key);
	}

	size_t Hash(const TypeEntry* value) const
	{
		return HashKey(value->type->Name());
	}

	bool Compare(const BString& key, const TypeEntry* value) const
	{
		return key == value->type->Name();
	}

	TypeEntry*& GetLink(TypeEntry* value) const
	{
		return value->fNextByName;
	}
};


struct GlobalTypeCache::TypeEntryHashDefinitionByID {
	typedef const BString	KeyType;
	typedef	TypeEntry		ValueType;

	size_t HashKey(const BString& key) const
	{
		return StringUtils::HashValue(key);
	}

	size_t Hash(const TypeEntry* value) const
	{
		return HashKey(value->type->ID());
	}

	bool Compare(const BString& key, const TypeEntry* value) const
	{
		return key == value->type->ID();
	}

	TypeEntry*& GetLink(TypeEntry* value) const
	{
		return value->fNextByID;
	}
};


// #pragma mark - GlobalTypeCache


GlobalTypeCache::GlobalTypeCache()
	:
	fLock("global type cache"),
	fTypesByName(NULL),
	fTypesByID(NULL)
{
}


GlobalTypeCache::~GlobalTypeCache()
{
	if (fTypesByName != NULL)
		fTypesByName->Clear();

	// release all cached type references
	if (fTypesByID != NULL) {
		TypeEntry* entry = fTypesByID->Clear(true);
		while (entry != NULL) {
			TypeEntry* nextEntry = entry->fNextByID;
			delete entry;
			entry = nextEntry;
		}
	}
}


status_t
GlobalTypeCache::Init()
{
	// check lock
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	// create name table
	fTypesByName = new(std::nothrow) NameTable;
	if (fTypesByName == NULL)
		return B_NO_MEMORY;

	error = fTypesByName->Init();
	if (error != B_OK)
		return error;

	// create ID table
	fTypesByID = new(std::nothrow) IDTable;
	if (fTypesByID == NULL)
		return B_NO_MEMORY;

	error = fTypesByID->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


Type*
GlobalTypeCache::GetType(const BString& name,
	const TypeLookupConstraints &constraints) const
{
	TypeEntry* typeEntry = fTypesByName->Lookup(name);
	if (typeEntry != NULL) {
		if (constraints.HasTypeKind()
			&& typeEntry->type->Kind() != constraints.TypeKind())
			typeEntry = NULL;
		else if (constraints.HasSubtypeKind()) {
			if (typeEntry->type->Kind() == TYPE_ADDRESS) {
					AddressType* type = dynamic_cast<AddressType*>(
						typeEntry->type);
					if (type == NULL)
						typeEntry = NULL;
					else if (type->AddressKind() != constraints.SubtypeKind())
						typeEntry = NULL;
			} else if (typeEntry->type->Kind() == TYPE_COMPOUND) {
					CompoundType* type = dynamic_cast<CompoundType*>(
						typeEntry->type);
					if (type == NULL)
						typeEntry = NULL;
					else if (type->CompoundKind() != constraints.SubtypeKind())
						typeEntry = NULL;
			}
		}
	}
	return typeEntry != NULL ? typeEntry->type : NULL;
}


Type*
GlobalTypeCache::GetTypeByID(const BString& id) const
{
	TypeEntry* typeEntry = fTypesByID->Lookup(id);
	return typeEntry != NULL ? typeEntry->type : NULL;
}


status_t
GlobalTypeCache::AddType(Type* type)
{
	const BString& id = type->ID();
	const BString& name = type->Name();

	if (fTypesByID->Lookup(id) != NULL
		|| (name.Length() > 0 && fTypesByID->Lookup(name) != NULL)) {
		return B_BAD_VALUE;
	}

	TypeEntry* typeEntry = new(std::nothrow) TypeEntry(type);
	if (typeEntry == NULL)
		return B_NO_MEMORY;

	fTypesByID->Insert(typeEntry);

	if (name.Length() > 0)
		fTypesByName->Insert(typeEntry);

	return B_OK;
}


void
GlobalTypeCache::RemoveType(Type* type)
{
	if (TypeEntry* typeEntry = fTypesByID->Lookup(type->ID())) {
		if (typeEntry->type == type) {
			fTypesByID->Remove(typeEntry);

			if (type->Name().Length() > 0)
				fTypesByName->Remove(typeEntry);

			delete typeEntry;
		}
	}
}


void
GlobalTypeCache::RemoveTypes(image_id imageID)
{
	AutoLocker<GlobalTypeCache> locker(this);

	for (IDTable::Iterator it = fTypesByID->GetIterator();
			TypeEntry* typeEntry = it.Next();) {
		if (typeEntry->type->ImageID() == imageID) {
			fTypesByID->RemoveUnchecked(typeEntry);

			if (typeEntry->type->Name().Length() > 0)
				fTypesByName->Remove(typeEntry);

			delete typeEntry;
		}
	}
}


// #pragma mark - GlobalTypeLookup


GlobalTypeLookup::~GlobalTypeLookup()
{
}
