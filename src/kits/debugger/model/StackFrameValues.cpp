/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "StackFrameValues.h"

#include <new>

#include "FunctionID.h"
#include "TypeComponentPath.h"


struct StackFrameValues::Key {
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


struct StackFrameValues::ValueEntry : Key {
	BVariant			value;
	ValueEntry*			next;

	ValueEntry(ObjectID* variable, TypeComponentPath* path)
		:
		Key(variable, path)
	{
		variable->AcquireReference();
		path->AcquireReference();
	}

	~ValueEntry()
	{
		variable->ReleaseReference();
		path->ReleaseReference();
	}
};


struct StackFrameValues::ValueEntryHashDefinition {
	typedef Key			KeyType;
	typedef	ValueEntry	ValueType;

	size_t HashKey(const Key& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const ValueEntry* value) const
	{
		return value->HashValue();
	}

	bool Compare(const Key& key, const ValueEntry* value) const
	{
		return key == *value;
	}

	ValueEntry*& GetLink(ValueEntry* value) const
	{
		return value->next;
	}
};


StackFrameValues::StackFrameValues()
	:
	fValues(NULL)
{
}


StackFrameValues::StackFrameValues(const StackFrameValues& other)
	:
	fValues(NULL)
{
	try {
		// init
		if (Init() != B_OK)
			throw std::bad_alloc();

		// clone all values
		for (ValueTable::Iterator it = other.fValues->GetIterator();
				ValueEntry* entry = it.Next();) {
			if (SetValue(entry->variable, entry->path, entry->value) != B_OK)
				throw std::bad_alloc();
		}
	} catch (...) {
		_Cleanup();
		throw;
	}
}


StackFrameValues::~StackFrameValues()
{
	_Cleanup();
}


status_t
StackFrameValues::Init()
{
	fValues = new(std::nothrow) ValueTable;
	if (fValues == NULL)
		return B_NO_MEMORY;

	return fValues->Init();
}


bool
StackFrameValues::GetValue(ObjectID* variable, const TypeComponentPath* path,
	BVariant& _value) const
{
	ValueEntry* entry = fValues->Lookup(
		Key(variable, (TypeComponentPath*)path));
	if (entry == NULL)
		return false;

	_value = entry->value;
	return true;
}


bool
StackFrameValues::HasValue(ObjectID* variable, const TypeComponentPath* path)
	const
{
	return fValues->Lookup(Key(variable, (TypeComponentPath*)path)) != NULL;
}


status_t
StackFrameValues::SetValue(ObjectID* variable, TypeComponentPath* path,
	const BVariant& value)
{
	ValueEntry* entry = fValues->Lookup(Key(variable, path));
	if (entry == NULL) {
		entry = new(std::nothrow) ValueEntry(variable, path);
		if (entry == NULL)
			return B_NO_MEMORY;
		fValues->Insert(entry);
	}

	entry->value = value;
	return B_OK;
}


void
StackFrameValues::_Cleanup()
{
	if (fValues != NULL) {
		ValueEntry* entry = fValues->Clear(true);

		while (entry != NULL) {
			ValueEntry* next = entry->next;
			delete entry;
			entry = next;
		}

		delete fValues;
		fValues = NULL;
	}
}
