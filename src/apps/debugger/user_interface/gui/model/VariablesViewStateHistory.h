/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLES_VIEW_STATE_HISTORY_H
#define VARIABLES_VIEW_STATE_HISTORY_H


#include <util/OpenHashTable.h>


class FunctionID;
class VariablesViewState;


class VariablesViewStateHistory {
public:
								VariablesViewStateHistory();
	virtual						~VariablesViewStateHistory();

			status_t			Init();

			VariablesViewState*	GetState(thread_id threadID,
									FunctionID* functionID) const;
			VariablesViewState*	GetState(FunctionID* functionID) const;

			status_t			SetState(thread_id threadID,
									FunctionID* functionID,
									VariablesViewState* state);

private:
			struct Key;
			struct StateEntry;
			struct StateEntryHashDefinition;

			typedef BOpenHashTable<StateEntryHashDefinition> StateTable;

private:
			StateTable*			fStates;
};


#endif	// VARIABLES_VIEW_STATE_HISTORY_H
