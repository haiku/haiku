/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VariablesViewStateHistory.h"

#include <new>

#include "FunctionID.h"
#include "VariablesViewState.h"


// #pragma mark - Key


struct VariablesViewStateHistory::Key {
	thread_id	threadID;
	FunctionID*	functionID;

	Key(thread_id threadID, FunctionID* functionID)
		:
		threadID(threadID),
		functionID(functionID)
	{
	}

	uint32 HashValue() const
	{
		return functionID->HashValue() ^ threadID;
	}

	bool operator==(const Key& other) const
	{
		return threadID == other.threadID && *functionID == *other.functionID;
	}
};


// #pragma mark - StateEntry


struct VariablesViewStateHistory::StateEntry : Key, VariablesViewNodeInfo {
	StateEntry*			next;
	VariablesViewState*	state;

	StateEntry(thread_id threadID, FunctionID* functionID)
		:
		Key(threadID, functionID),
		state(NULL)
	{
		functionID->AcquireReference();
	}

	~StateEntry()
	{
		functionID->ReleaseReference();
		if (state != NULL)
			state->ReleaseReference();
	}

	void SetState(VariablesViewState* state)
	{
		if (state == this->state)
			return;

		if (state != NULL)
			state->AcquireReference();

		if (this->state != NULL)
			this->state->ReleaseReference();

		this->state = state;
	}
};


struct VariablesViewStateHistory::StateEntryHashDefinition {
	typedef Key			KeyType;
	typedef	StateEntry	ValueType;

	size_t HashKey(const Key& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const StateEntry* value) const
	{
		return HashKey(*value);
	}

	bool Compare(const Key& key, const StateEntry* value) const
	{
		return key == *value;
	}

	StateEntry*& GetLink(StateEntry* value) const
	{
		return value->next;
	}
};


VariablesViewStateHistory::VariablesViewStateHistory()
	:
	fStates(NULL)
{
}


VariablesViewStateHistory::~VariablesViewStateHistory()
{
	if (fStates != NULL) {
		StateEntry* entry = fStates->Clear(true);

		while (entry != NULL) {
			StateEntry* next = entry->next;
			delete entry;
			entry = next;
		}

		delete fStates;
	}
}


status_t
VariablesViewStateHistory::Init()
{
	fStates = new(std::nothrow) StateTable;
	if (fStates == NULL)
		return B_NO_MEMORY;

	return fStates->Init();
}


VariablesViewState*
VariablesViewStateHistory::GetState(thread_id threadID, FunctionID* functionID)
	const
{
	// first try an exact match with the thread ID
	if (threadID >= 0) {
		StateEntry* stateEntry = fStates->Lookup(Key(threadID, functionID));
		if (stateEntry != NULL)
			return stateEntry->state;
	}

	// just match the function ID
	StateEntry* stateEntry = fStates->Lookup(Key(-1, functionID));
	return stateEntry != NULL ? stateEntry->state : NULL;
}


VariablesViewState*
VariablesViewStateHistory::GetState(FunctionID* functionID) const
{
	StateEntry* stateEntry = fStates->Lookup(Key(-1, functionID));
	return stateEntry != NULL ? stateEntry->state : NULL;
}


status_t
VariablesViewStateHistory::SetState(thread_id threadID, FunctionID* functionID,
	VariablesViewState* state)
{
	// Make sure the default entry for the function exists.
	StateEntry* defaultEntry = fStates->Lookup(Key(-1, functionID));
	bool newDefaultEntry = false;

	if (defaultEntry == NULL) {
		defaultEntry = new(std::nothrow) StateEntry(-1, functionID);
		if (defaultEntry == NULL)
			return B_NO_MEMORY;
		fStates->Insert(defaultEntry);
		newDefaultEntry = true;
	}

	// If we have a valid thread ID, make sure the respective entry for the
	// function exists.
	StateEntry* threadEntry = NULL;
	if (threadID >= 0) {
		threadEntry = fStates->Lookup(Key(threadID, functionID));

		if (threadEntry == NULL) {
			threadEntry = new(std::nothrow) StateEntry(threadID, functionID);
			if (threadEntry == NULL) {
				if (newDefaultEntry) {
					fStates->Remove(defaultEntry);
					delete defaultEntry;
				}
				return B_NO_MEMORY;
			}
			fStates->Insert(threadEntry);
		}
	}

	defaultEntry->SetState(state);
	if (threadEntry != NULL)
		threadEntry->SetState(state);

	return B_OK;
}
