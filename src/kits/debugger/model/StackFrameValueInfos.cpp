/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "StackFrameValueInfos.h"

#include <new>

#include "FunctionID.h"
#include "Type.h"
#include "TypeComponentPath.h"
#include "ValueLocation.h"


struct StackFrameValueInfos::Key {
	ObjectID*			variable;
	TypeComponentPath*	path;

	Key(ObjectID* variable, TypeComponentPath* path)
		:
		variable(variable),
		path(path)
	{
	}

	uint32 HashValue() const
	{
		return variable->HashValue() ^ path->HashValue();
	}

	bool operator==(const Key& other) const
	{
		return *variable == *other.variable && *path == *other.path;
	}
};


struct StackFrameValueInfos::InfoEntry : Key {
	Type*				type;
	ValueLocation*		location;
	InfoEntry*			next;

	InfoEntry(ObjectID* variable, TypeComponentPath* path)
		:
		Key(variable, path),
		type(NULL),
		location(NULL)
	{
		variable->AcquireReference();
		path->AcquireReference();
	}

	~InfoEntry()
	{
		SetInfo(NULL, NULL);
		variable->ReleaseReference();
		path->ReleaseReference();
	}


	void SetInfo(Type* type, ValueLocation* location)
	{
		if (type != NULL)
			type->AcquireReference();
		if (location != NULL)
			location->AcquireReference();

		if (this->type != NULL)
			this->type->ReleaseReference();
		if (this->location != NULL)
			this->location->ReleaseReference();

		this->type = type;
		this->location = location;
	}
};


struct StackFrameValueInfos::InfoEntryHashDefinition {
	typedef Key			KeyType;
	typedef	InfoEntry	ValueType;

	size_t HashKey(const Key& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const InfoEntry* value) const
	{
		return value->HashValue();
	}

	bool Compare(const Key& key, const InfoEntry* value) const
	{
		return key == *value;
	}

	InfoEntry*& GetLink(InfoEntry* value) const
	{
		return value->next;
	}
};


StackFrameValueInfos::StackFrameValueInfos()
	:
	fValues(NULL)
{
}


StackFrameValueInfos::~StackFrameValueInfos()
{
	_Cleanup();
}


status_t
StackFrameValueInfos::Init()
{
	fValues = new(std::nothrow) ValueTable;
	if (fValues == NULL)
		return B_NO_MEMORY;

	return fValues->Init();
}


bool
StackFrameValueInfos::GetInfo(ObjectID* variable,
	const TypeComponentPath* path, Type** _type,
	ValueLocation** _location) const
{
	InfoEntry* entry = fValues->Lookup(
		Key(variable, (TypeComponentPath*)path));
	if (entry == NULL)
		return false;

	if (_type != NULL) {
		entry->type->AcquireReference();
		*_type = entry->type;
	}

	if (_location != NULL) {
		entry->location->AcquireReference();
		*_location = entry->location;
	}

	return true;
}


bool
StackFrameValueInfos::HasInfo(ObjectID* variable,
	const TypeComponentPath* path) const
{
	return fValues->Lookup(Key(variable, (TypeComponentPath*)path)) != NULL;
}


status_t
StackFrameValueInfos::SetInfo(ObjectID* variable, TypeComponentPath* path,
	Type* type, ValueLocation* location)
{
	InfoEntry* entry = fValues->Lookup(Key(variable, path));
	if (entry == NULL) {
		entry = new(std::nothrow) InfoEntry(variable, path);
		if (entry == NULL)
			return B_NO_MEMORY;
		fValues->Insert(entry);
	}

	entry->SetInfo(type, location);
	return B_OK;
}


void
StackFrameValueInfos::_Cleanup()
{
	if (fValues != NULL) {
		InfoEntry* entry = fValues->Clear(true);

		while (entry != NULL) {
			InfoEntry* next = entry->next;
			delete entry;
			entry = next;
		}

		delete fValues;
		fValues = NULL;
	}
}
