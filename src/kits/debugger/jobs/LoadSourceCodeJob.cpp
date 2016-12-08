/*
 * Copyright 2012-2016, Rene Gollent, rene@gollent.com.
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

	SetDescription("Loading source code for function %s",
		fFunctionInstance->PrettyName().String());
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
			// various parts of the debugger expect functions to have only
			// one of source or disassembly available. As such, if the current
			// function had source code previously active, unset it when
			// explicitly asked for disassembly. This needs to be done first
			// since Function will clear the disassembled code states of all
			// its child instances.
			function_source_state state
				= fLoadForFunction ? FUNCTION_SOURCE_LOADED
					: FUNCTION_SOURCE_SUPPRESSED;
			if (function->SourceCodeState() == FUNCTION_SOURCE_LOADED) {
				FileSourceCode* functionSourceCode = function->GetSourceCode();
				function->SetSourceCode(functionSourceCode, state);
			}

			fFunctionInstance->SetSourceCode(sourceCode, state);
			sourceCode->ReleaseReference();
		}
	} else
		fFunctionInstance->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);

	return error;
}
