/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "Function.h"

#include "FileSourceCode.h"
#include "FunctionID.h"


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
	if (FirstInstance() != NULL) {
		FirstInstance()->SourceFile()->RemoveListener(this);
		FirstInstance()->SourceFile()->ReleaseReference();
	}
}


void
Function::SetSourceCode(FileSourceCode* source, function_source_state state)
{
	if (source == fSourceCode && state == fSourceCodeState)
		return;

	if (fSourceCode != NULL)
		fSourceCode->ReleaseReference();

	fSourceCode = source;
	fSourceCodeState = state;

	if (fSourceCode != NULL) {
		fSourceCode->AcquireReference();

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
	bool firstInstance = fInstances.IsEmpty();
	fInstances.Add(instance);
	if (firstInstance && SourceFile() != NULL) {
		instance->SourceFile()->AcquireReference();
		instance->SourceFile()->AddListener(this);
	}
}


void
Function::RemoveInstance(FunctionInstance* instance)
{
	fInstances.Remove(instance);
	if (fInstances.IsEmpty() && instance->SourceFile() != NULL) {
		instance->SourceFile()->RemoveListener(this);
		instance->SourceFile()->ReleaseReference();
	}
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


void
Function::LocatableFileChanged(LocatableFile* file)
{
	BString locatedPath;
	BString path;
	file->GetPath(path);
	if (file->GetLocatedPath(locatedPath) && locatedPath != path) {
		SetSourceCode(NULL, FUNCTION_SOURCE_NOT_LOADED);
		for (FunctionInstanceList::Iterator it = fInstances.GetIterator();
				FunctionInstance* instance = it.Next();) {
			instance->SetSourceCode(NULL, FUNCTION_SOURCE_NOT_LOADED);
		}
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
