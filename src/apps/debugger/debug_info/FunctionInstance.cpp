/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FunctionInstance.h"

#include <new>

#include "DisassembledCode.h"
#include "Function.h"
#include "FunctionID.h"
#include "ImageDebugInfo.h"
#include "LocatableFile.h"


FunctionInstance::FunctionInstance(ImageDebugInfo* imageDebugInfo,
	FunctionDebugInfo* functionDebugInfo)
	:
	fImageDebugInfo(imageDebugInfo),
	fFunction(NULL),
	fFunctionDebugInfo(functionDebugInfo),
	fSourceCode(NULL),
	fSourceCodeState(FUNCTION_SOURCE_NOT_LOADED)
{
	fFunctionDebugInfo->AcquireReference();
	// TODO: What about fImageDebugInfo? We must be careful regarding cyclic
	// references.
}


FunctionInstance::~FunctionInstance()
{
	SetFunction(NULL);
	fFunctionDebugInfo->ReleaseReference();
}


FunctionID*
FunctionInstance::GetFunctionID() const
{
	BString idString;
	BString path;
	if (LocatableFile* file = SourceFile()) {
		idString << "s:";
		file->GetPath(path);
	} else {
		idString << "i:";
		path << GetImageDebugInfo()->GetImageInfo().Name();
	}

	idString << path << "//" << Name();

	return new(std::nothrow) FunctionID(idString);
}


void
FunctionInstance::SetFunction(Function* function)
{
	if (fFunction != NULL)
		fFunction->ReleaseReference();

	fFunction = function;

	if (fFunction != NULL)
		fFunction->AcquireReference();
}


void
FunctionInstance::SetSourceCode(DisassembledCode* source,
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

	if (fFunction != NULL)
		fFunction->NotifySourceCodeChanged();
}
