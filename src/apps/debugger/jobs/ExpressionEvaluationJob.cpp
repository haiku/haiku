/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <String.h>

#include <AutoLocker.h>

#include "DebuggerInterface.h"
#include "ExpressionInfo.h"
#include "SourceLanguage.h"
#include "StackFrame.h"
#include "Team.h"
#include "Thread.h"
#include "Type.h"
#include "Value.h"
#include "ValueNode.h"
#include "ValueNodeManager.h"
#include "Variable.h"


ExpressionEvaluationJob::ExpressionEvaluationJob(Team* team,
	DebuggerInterface* debuggerInterface, SourceLanguage* language,
	ExpressionInfo* info, StackFrame* frame,
	Thread* thread)
	:
	fKey(info->Expression(), JOB_TYPE_EVALUATE_EXPRESSION),
	fTeam(team),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(debuggerInterface->GetArchitecture()),
	fTypeInformation(team->GetTeamTypeInformation()),
	fLanguage(language),
	fExpressionInfo(info),
	fFrame(frame),
	fThread(thread),
	fManager(NULL),
	fResultValue(NULL)
{
	fLanguage->AcquireReference();
	fExpressionInfo->AcquireReference();

	if (fFrame != NULL)
		fFrame->AcquireReference();
	if (fThread != NULL)
		fThread->AcquireReference();
}


ExpressionEvaluationJob::~ExpressionEvaluationJob()
{
	fLanguage->ReleaseReference();
	fExpressionInfo->ReleaseReference();
	if (fFrame != NULL)
		fFrame->ReleaseReference();
	if (fThread != NULL)
		fThread->ReleaseReference();
	if (fManager != NULL)
		fManager->ReleaseReference();
	if (fResultValue != NULL)
		fResultValue->ReleaseReference();
}


const JobKey&
ExpressionEvaluationJob::Key() const
{
	return fKey;
}


status_t
ExpressionEvaluationJob::Do()
{
	BReference<Value> reference;
	status_t result = B_OK;
	if (fFrame != NULL && fManager == NULL) {
		fManager = new(std::nothrow) ValueNodeManager();
		if (fManager == NULL)
			result = B_NO_MEMORY;
		else
			result = fManager->SetStackFrame(fThread, fFrame);
	}

	if (result != B_OK) {
		fExpressionInfo->NotifyExpressionEvaluated(result, NULL);
		return result;
	}

	ValueNode* neededNode = NULL;
	result = fLanguage->EvaluateExpression(fExpressionInfo->Expression(),
		fManager, fTeam->GetTeamTypeInformation(), fResultValue, neededNode);
	if (neededNode != NULL) {
		result = ResolveNodeValue(neededNode);
		if (State() == JOB_STATE_WAITING)
			return B_OK;
		// if result != B_OK, fall through
	}

	fExpressionInfo->NotifyExpressionEvaluated(result, fResultValue);

	return B_OK;
}


status_t
ExpressionEvaluationJob::ResolveNodeValue(ValueNode* node)
{
	AutoLocker<Worker> workerLocker(GetWorker());
	SimpleJobKey jobKey(node, JOB_TYPE_RESOLVE_VALUE_NODE_VALUE);

	status_t error = B_OK;
	if (GetWorker()->GetJob(jobKey) == NULL) {
		workerLocker.Unlock();

		// schedule the job
		error = GetWorker()->ScheduleJob(
			new(std::nothrow) ResolveValueNodeValueJob(fDebuggerInterface,
				fArchitecture, fThread->GetCpuState(), fTypeInformation,
				fManager->GetContainer(), node));
		if (error != B_OK) {
			// scheduling failed -- set the value to invalid
			node->SetLocationAndValue(NULL, NULL, error);
			return error;
		}
	}

	// wait for the job to finish
	workerLocker.Unlock();


	switch (WaitFor(jobKey)) {
		case JOB_DEPENDENCY_SUCCEEDED:
		case JOB_DEPENDENCY_NOT_FOUND:
		case JOB_DEPENDENCY_ACTIVE:
			error = B_OK;
			break;
		case JOB_DEPENDENCY_FAILED:
		case JOB_DEPENDENCY_ABORTED:
		default:
			error = B_ERROR;
			break;
	}

	return error;
}
