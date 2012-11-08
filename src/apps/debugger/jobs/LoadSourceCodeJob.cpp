/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>

#include "Architecture.h"
#include "DebuggerInterface.h"
#include "DisassembledCode.h"
#include "Function.h"
#include "FunctionInstance.h"
#include "FileSourceCode.h"
#include "Team.h"
#include "TeamDebugInfo.h"


LoadSourceCodeJob::LoadSourceCodeJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	Team* team, FunctionInstance* functionInstance, bool loadForFunction)
	:
	fKey(functionInstance, JOB_TYPE_LOAD_SOURCE_CODE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fTeam(team),
	fFunctionInstance(functionInstance),
	fLoadForFunction(loadForFunction)
{
	fFunctionInstance->AcquireReference();
}


LoadSourceCodeJob::~LoadSourceCodeJob()
{
	fFunctionInstance->ReleaseReference();
}


const JobKey&
LoadSourceCodeJob::Key() const
{
	return fKey;
}


status_t
LoadSourceCodeJob::Do()
{
	// if requested, try loading the source code for the function
	Function* function = fFunctionInstance->GetFunction();
	if (fLoadForFunction) {
		FileSourceCode* sourceCode;
		status_t error = fTeam->DebugInfo()->LoadSourceCode(
			function->SourceFile(), sourceCode);

		AutoLocker<Team> locker(fTeam);

		if (error == B_OK) {
			function->SetSourceCode(sourceCode, FUNCTION_SOURCE_LOADED);
			sourceCode->ReleaseReference();
			return B_OK;
		}

		function->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);
	}

	// Only try to load the function instance code, if it's not overridden yet.
	AutoLocker<Team> locker(fTeam);
	if (fFunctionInstance->SourceCodeState() != FUNCTION_SOURCE_LOADING)
		return B_OK;
	locker.Unlock();

	// disassemble the function
	DisassembledCode* sourceCode = NULL;
	status_t error = fTeam->DebugInfo()->DisassembleFunction(fFunctionInstance,
		sourceCode);

	// set the result
	locker.Lock();
	if (error == B_OK) {
		if (fFunctionInstance->SourceCodeState() == FUNCTION_SOURCE_LOADING) {
			fFunctionInstance->SetSourceCode(sourceCode,
				FUNCTION_SOURCE_LOADED);
			sourceCode->ReleaseReference();
		}
	} else
		fFunctionInstance->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);

	return error;
}
