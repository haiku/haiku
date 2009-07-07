/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FunctionInstance.h"

#include "Function.h"


FunctionInstance::FunctionInstance(ImageDebugInfo* imageDebugInfo,
	FunctionDebugInfo* functionDebugInfo)
	:
	fImageDebugInfo(imageDebugInfo),
	fFunction(NULL),
	fFunctionDebugInfo(functionDebugInfo)
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


void
FunctionInstance::SetFunction(Function* function)
{
	if (fFunction != NULL)
		fFunction->ReleaseReference();

	fFunction = function;

	if (fFunction != NULL)
		fFunction->AcquireReference();
}
