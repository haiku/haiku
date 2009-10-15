/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VariablesViewState.h"

#include <new>

#include "FunctionID.h"
#include "StackFrameValues.h"
#include "TypeComponentPath.h"


// #pragma mark - VariablesViewNodeInfo


VariablesViewNodeInfo::VariablesViewNodeInfo()
	:
	fNodeExpanded(false)
{
}


VariablesViewNodeInfo::VariablesViewNodeInfo(const VariablesViewNodeInfo& other)
	:
	fNodeExpanded(other.fNodeExpanded)
{
}


VariablesViewNodeInfo&
VariablesViewNodeInfo::operator=(const VariablesViewNodeInfo& other)
{
	fNodeExpanded = other.fNodeExpanded;
	return *this;
}


void
VariablesViewNodeInfo::SetNodeExpanded(bool expanded)
{
	fNodeExpanded = expanded;
}


// #pragma mark - Key


struct VariablesViewState::Key {
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


// #pragma mark - InfoEntry


struct VariablesViewState::InfoEntry : Key, VariablesViewNodeInfo {
	InfoEntry*	next;

	InfoEntry(ObjectID* variable, TypeComponentPath* path)
		:
		Key(variable, path)
	{
		variable->AcquireReference();
		path->AcquireReference();
	}

	~InfoEntry()
	{
		variable->ReleaseReference();
		path->ReleaseReference();
	}

	void SetInfo(const VariablesViewNodeInfo& info)
	{
		VariablesViewNodeInfo::operator=(info);
	}
};


struct VariablesViewState::InfoEntryHashDefinition {
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


VariablesViewState::VariablesViewState()
	:
	fNodeInfos(NULL),
	fValues(NULL)
{
}


VariablesViewState::~VariablesViewState()
{
	_Cleanup();
}


status_t
VariablesViewState::Init()
{
	fNodeInfos = new(std::nothrow) NodeInfoTable;
	if (fNodeInfos == NULL)
		return B_NO_MEMORY;

	return fNodeInfos->Init();
}

void
VariablesViewState::SetValues(StackFrameValues* values)
{
	if (fValues == values)
		return;

	if (fValues != NULL)
		fValues->ReleaseReference();

	fValues = values;

	if (fValues != NULL)
		fValues->AcquireReference();
}


const VariablesViewNodeInfo*
VariablesViewState::GetNodeInfo(ObjectID* variable,
	const TypeComponentPath* path) const
{
	return fNodeInfos->Lookup(Key(variable, (TypeComponentPath*)path));
}


status_t
VariablesViewState::SetNodeInfo(ObjectID* variable, TypeComponentPath* path,
	const VariablesViewNodeInfo& info)
{
	InfoEntry* entry = fNodeInfos->Lookup(Key(variable, path));
	if (entry == NULL) {
		entry = new(std::nothrow) InfoEntry(variable, path);
		if (entry == NULL)
			return B_NO_MEMORY;
		fNodeInfos->Insert(entry);
	}

	entry->SetInfo(info);
	return B_OK;
}


void
VariablesViewState::_Cleanup()
{
	if (fNodeInfos != NULL) {
		InfoEntry* entry = fNodeInfos->Clear(true);

		while (entry != NULL) {
			InfoEntry* next = entry->next;
			delete entry;
			entry = next;
		}

		delete fNodeInfos;
		fNodeInfos = NULL;
	}

	SetValues(NULL);
}
