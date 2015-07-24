/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>

#include "Architecture.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "TeamTypeInformation.h"
#include "Tracing.h"
#include "Value.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeContainer.h"
#include "ValueWriter.h"


WriteValueNodeValueJob::WriteValueNodeValueJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	CpuState* cpuState, TeamTypeInformation* typeInformation,
	ValueNode* valueNode, Value* newValue)
	:
	fKey(valueNode, JOB_TYPE_WRITE_VALUE_NODE_VALUE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fCpuState(cpuState),
	fTypeInformation(typeInformation),
	fValueNode(valueNode),
	fNewValue(newValue)
{
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
	fValueNode->AcquireReference();
	fNewValue->AcquireReference();
}


WriteValueNodeValueJob::~WriteValueNodeValueJob()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
	fValueNode->ReleaseReference();
	fNewValue->ReleaseReference();
}


const JobKey&
WriteValueNodeValueJob::Key() const
{
	return fKey;
}


status_t
WriteValueNodeValueJob::Do()
{
	ValueNodeContainer* container = fValueNode->Container();
	if (container == NULL)
		return B_BAD_VALUE;

	ValueWriter writer(fArchitecture, fDebuggerInterface,
		fCpuState, -1);

	BVariant value;
	fNewValue->ToVariant(value);

	status_t error = writer.WriteValue(fValueNode->Location(), value);
	if (error != B_OK)
		return error;

	AutoLocker<ValueNodeContainer> containerLocker(container);
	fValueNode->SetLocationAndValue(fValueNode->Location(), fNewValue, B_OK);

	return B_OK;
}
