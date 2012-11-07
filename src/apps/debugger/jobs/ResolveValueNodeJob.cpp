/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
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
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeContainer.h"


ResolveValueNodeValueJob::ResolveValueNodeValueJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	CpuState* cpuState, TeamTypeInformation* typeInformation,
	ValueNodeContainer* container, ValueNode* valueNode)
	:
	fKey(valueNode, JOB_TYPE_RESOLVE_VALUE_NODE_VALUE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fCpuState(cpuState),
	fTypeInformation(typeInformation),
	fContainer(container),
	fValueNode(valueNode)
{
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
	fContainer->AcquireReference();
	fValueNode->AcquireReference();
}


ResolveValueNodeValueJob::~ResolveValueNodeValueJob()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
	fContainer->ReleaseReference();
	fValueNode->ReleaseReference();
}


const JobKey&
ResolveValueNodeValueJob::Key() const
{
	return fKey;
}


status_t
ResolveValueNodeValueJob::Do()
{
	// check whether the node still belongs to the container
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);
	if (fValueNode->Container() != fContainer)
		return B_BAD_VALUE;

	// if already resolved, we're done
	status_t nodeResolutionState
		= fValueNode->LocationAndValueResolutionState();
	if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
		return nodeResolutionState;

	containerLocker.Unlock();

	// resolve
	status_t error = _ResolveNodeValue();
	if (error != B_OK) {
		nodeResolutionState = fValueNode->LocationAndValueResolutionState();
		if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
			return nodeResolutionState;

		containerLocker.Lock();
		fValueNode->SetLocationAndValue(NULL, NULL, error);
		containerLocker.Unlock();
	}

	return error;
}


status_t
ResolveValueNodeValueJob::_ResolveNodeValue()
{
	// get the node child and parent node
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);
	ValueNodeChild* nodeChild = fValueNode->NodeChild();
	BReference<ValueNodeChild> nodeChildReference(nodeChild);

	ValueNode* parentNode = nodeChild->Parent();
	BReference<ValueNode> parentNodeReference(parentNode);

	// Check whether the node child location has been resolved already
	// (successfully).
	status_t nodeChildResolutionState = nodeChild->LocationResolutionState();
	bool nodeChildDone = nodeChildResolutionState != VALUE_NODE_UNRESOLVED;
	if (nodeChildDone && nodeChildResolutionState != B_OK)
		return nodeChildResolutionState;

	// If the child node location has not been resolved yet, check whether the
	// parent node location and value have been resolved already (successfully).
	bool parentDone = true;
	if (!nodeChildDone && parentNode != NULL) {
		status_t parentResolutionState
			= parentNode->LocationAndValueResolutionState();
		parentDone = parentResolutionState != VALUE_NODE_UNRESOLVED;
		if (parentDone && parentResolutionState != B_OK)
			return parentResolutionState;
	}

	containerLocker.Unlock();

	// resolve the parent node location and value, if necessary
	if (!parentDone) {
		status_t error = _ResolveParentNodeValue(parentNode);
		if (error != B_OK) {
			TRACE_LOCALS("ResolveValueNodeValueJob::_ResolveNodeValue(): value "
				"node: %p (\"%s\"): _ResolveParentNodeValue(%p) failed\n",
				fValueNode, fValueNode->Name().String(), parentNode);
			return error;
		}

		if (State() == JOB_STATE_WAITING)
			return B_OK;
	}

	// resolve the node child location, if necessary
	if (!nodeChildDone) {
		status_t error = _ResolveNodeChildLocation(nodeChild);
		if (error != B_OK) {
			TRACE_LOCALS("ResolveValueNodeValueJob::_ResolveNodeValue(): value "
				"node: %p (\"%s\"): _ResolveNodeChildLocation(%p) failed\n",
				fValueNode, fValueNode->Name().String(), nodeChild);
			return error;
		}
	}

	// resolve the node location and value
	ValueLoader valueLoader(fArchitecture, fDebuggerInterface,
		fTypeInformation, fCpuState);
	ValueLocation* location;
	Value* value;
	status_t error = fValueNode->ResolvedLocationAndValue(&valueLoader,
		location, value);
	if (error != B_OK) {
		TRACE_LOCALS("ResolveValueNodeValueJob::_ResolveNodeValue(): value "
			"node: %p (\"%s\"): fValueNode->ResolvedLocationAndValue() "
			"failed\n", fValueNode, fValueNode->Name().String());
		return error;
	}
	BReference<ValueLocation> locationReference(location, true);
	BReference<Value> valueReference(value, true);

	// set location and value on the node
	containerLocker.Lock();
	status_t nodeResolutionState
		= fValueNode->LocationAndValueResolutionState();
	if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
		return nodeResolutionState;
	fValueNode->SetLocationAndValue(location, value, B_OK);
	containerLocker.Unlock();

	return B_OK;
}


status_t
ResolveValueNodeValueJob::_ResolveNodeChildLocation(ValueNodeChild* nodeChild)
{
	// resolve the location
	ValueLoader valueLoader(fArchitecture, fDebuggerInterface,
		fTypeInformation, fCpuState);
	ValueLocation* location = NULL;
	status_t error = nodeChild->ResolveLocation(&valueLoader, location);
	BReference<ValueLocation> locationReference(location, true);

	// set the location on the node child
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);
	status_t nodeChildResolutionState = nodeChild->LocationResolutionState();
	if (nodeChildResolutionState == VALUE_NODE_UNRESOLVED)
		nodeChild->SetLocation(location, error);
	else
		error = nodeChildResolutionState;

	return error;
}


status_t
ResolveValueNodeValueJob::_ResolveParentNodeValue(ValueNode* parentNode)
{
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);

	if (parentNode->Container() != fContainer)
		return B_BAD_VALUE;

	// if the parent node already has a value, we're done
	status_t nodeResolutionState
		= parentNode->LocationAndValueResolutionState();
	if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
		return nodeResolutionState;

	// check whether a job is already in progress
	AutoLocker<Worker> workerLocker(GetWorker());
	SimpleJobKey jobKey(parentNode, JOB_TYPE_RESOLVE_VALUE_NODE_VALUE);
	if (GetWorker()->GetJob(jobKey) == NULL) {
		workerLocker.Unlock();

		// schedule the job
		status_t error = GetWorker()->ScheduleJob(
			new(std::nothrow) ResolveValueNodeValueJob(fDebuggerInterface,
				fArchitecture, fCpuState, fTypeInformation, fContainer,
				parentNode));
		if (error != B_OK) {
			// scheduling failed -- set the value to invalid
			parentNode->SetLocationAndValue(NULL, NULL, error);
			return error;
		}
	}

	// wait for the job to finish
	workerLocker.Unlock();
	containerLocker.Unlock();

	switch (WaitFor(jobKey)) {
		case JOB_DEPENDENCY_SUCCEEDED:
		case JOB_DEPENDENCY_NOT_FOUND:
			// "Not found" can happen due to a race condition between
			// unlocking the worker and starting to wait.
			break;
		case JOB_DEPENDENCY_ACTIVE:
			return B_OK;
		case JOB_DEPENDENCY_FAILED:
		case JOB_DEPENDENCY_ABORTED:
		default:
			return B_ERROR;
	}

	containerLocker.Lock();

	// now there should be a value for the node
	nodeResolutionState = parentNode->LocationAndValueResolutionState();
	return nodeResolutionState != VALUE_NODE_UNRESOLVED
		? nodeResolutionState : B_ERROR;
}
