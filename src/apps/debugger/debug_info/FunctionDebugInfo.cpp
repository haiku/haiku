/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FunctionDebugInfo.h"

#include "SourceCode.h"


FunctionDebugInfo::FunctionDebugInfo()
	:
	fSourceCode(NULL),
	fSourceCodeState(FUNCTION_SOURCE_NOT_LOADED)
{
}


FunctionDebugInfo::~FunctionDebugInfo()
{
	SetSourceCode(NULL, FUNCTION_SOURCE_NOT_LOADED);
}


void
FunctionDebugInfo::SetSourceCode(SourceCode* source,
	function_source_state state)
{
	if (source == fSourceCode && state == fSourceCodeState)
		return;

	if (fSourceCode != NULL)
		fSourceCode->RemoveReference();

	fSourceCode = source;
	fSourceCodeState = state;

	if (fSourceCode != NULL)
		fSourceCode->AddReference();

	// notify listeners
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->FunctionSourceCodeChanged(this);
	}
}


void
FunctionDebugInfo::AddListener(Listener* listener)
{
	fListeners.Add(listener);
}


void
FunctionDebugInfo::RemoveListener(Listener* listener)
{
	fListeners.Remove(listener);
}


// #pragma mark - Listener


FunctionDebugInfo::Listener::~Listener()
{
}


void
FunctionDebugInfo::Listener::FunctionSourceCodeChanged(
	FunctionDebugInfo* function)
{
}
