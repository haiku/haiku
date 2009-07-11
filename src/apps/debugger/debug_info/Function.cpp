/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Function.h"

#include "FileSourceCode.h"


Function::Function()
	:
	fSourceCode(NULL),
	fSourceCodeState(FUNCTION_SOURCE_NOT_LOADED),
	fNotificationsDisabled(0)
{
}


Function::~Function()
{
	SetSourceCode(NULL, FUNCTION_SOURCE_NOT_LOADED);
}


void
Function::SetSourceCode(FileSourceCode* source, function_source_state state)
{
	if (source == fSourceCode && state == fSourceCodeState)
		return;

	if (fSourceCode != NULL)
		fSourceCode->RemoveReference();

	fSourceCode = source;
	fSourceCodeState = state;

	if (fSourceCode != NULL) {
		fSourceCode->AddReference();

		// unset all instances' source codes
		fNotificationsDisabled++;
		for (FunctionInstanceList::Iterator it = fInstances.GetIterator();
				FunctionInstance* instance = it.Next();) {
			instance->SetSourceCode(NULL, FUNCTION_SOURCE_NOT_LOADED);
		}
		fNotificationsDisabled--;
	}

	// notify listeners
	NotifySourceCodeChanged();
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


void
Function::AddInstance(FunctionInstance* instance)
{
	fInstances.Add(instance);
}


void
Function::RemoveInstance(FunctionInstance* instance)
{
	fInstances.Remove(instance);
}


void
Function::NotifySourceCodeChanged()
{
	if (fNotificationsDisabled > 0)
		return;

	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->FunctionSourceCodeChanged(this);
	}
}


// #pragma mark - Listener


Function::Listener::~Listener()
{
}


void
Function::Listener::FunctionSourceCodeChanged(Function* function)
{
}
