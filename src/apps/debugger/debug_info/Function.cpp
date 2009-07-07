/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Function.h"

#include "SourceCode.h"


Function::Function()
	:
	fSourceCode(NULL),
	fSourceCodeState(FUNCTION_SOURCE_NOT_LOADED)
{
}


Function::~Function()
{
	SetSourceCode(NULL, FUNCTION_SOURCE_NOT_LOADED);
}


void
Function::SetSourceCode(SourceCode* source, function_source_state state)
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
Function::AddListener(Listener* listener)
{
	fListeners.Add(listener);
}


void
Function::RemoveListener(Listener* listener)
{
	fListeners.Remove(listener);
}


#include <stdio.h>
void
Function::AddInstance(FunctionInstance* instance)
{
printf("    %p: added %p\n", this, instance);
	fInstances.Add(instance);
}


void
Function::RemoveInstance(FunctionInstance* instance)
{
printf("    %p: removed %p\n", this, instance);
	fInstances.Remove(instance);
}


// #pragma mark - Listener


Function::Listener::~Listener()
{
}


void
Function::Listener::FunctionSourceCodeChanged(Function* function)
{
}
