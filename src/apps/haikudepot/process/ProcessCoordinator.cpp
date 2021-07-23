/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ProcessCoordinator.h"

#include <AutoLocker.h>
#include <Catalog.h>
#include <StringFormat.h>

#include "Logger.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessCoordinator"


// #pragma mark - ProcessCoordinatorState implementation


ProcessCoordinatorState::ProcessCoordinatorState(
	const ProcessCoordinator* processCoordinator, float progress,
	const BString& message, bool isRunning, status_t errorStatus)
	:
	fProcessCoordinator(processCoordinator),
	fProgress(progress),
	fMessage(message),
	fIsRunning(isRunning),
	fErrorStatus(errorStatus)
{
}


ProcessCoordinatorState::~ProcessCoordinatorState()
{
}


const ProcessCoordinator*
ProcessCoordinatorState::Coordinator() const
{
	return fProcessCoordinator;
}


float
ProcessCoordinatorState::Progress() const
{
	return fProgress;
}


BString
ProcessCoordinatorState::Message() const
{
	return fMessage;
}


bool
ProcessCoordinatorState::IsRunning() const
{
	return fIsRunning;
}


status_t
ProcessCoordinatorState::ErrorStatus() const
{
	return fErrorStatus;
}


// #pragma mark - ProcessCoordinator implementation


ProcessCoordinator::ProcessCoordinator(const char* name, BMessage* message)
	:
	fName(name),
	fListener(NULL),
	fMessage(message),
	fWasStopped(false)
{
}


ProcessCoordinator::~ProcessCoordinator()
{
	AutoLocker<BLocker> locker(&fLock);
	for (int32 i = 0; i < fNodes.CountItems(); i++) {
		AbstractProcessNode* node = fNodes.ItemAt(i);
		node->Process()->SetListener(NULL);
		delete node;
	}
	delete fMessage;
}


void
ProcessCoordinator::SetListener(ProcessCoordinatorListener *listener)
{
	fListener = listener;
}


void
ProcessCoordinator::AddNode(AbstractProcessNode* node)
{
	AutoLocker<BLocker> locker(&fLock);
	fNodes.AddItem(node);
	node->Process()->SetListener(this);
}


void
ProcessCoordinator::ProcessChanged()
{
	_CoordinateAndCallListener();
}


bool
ProcessCoordinator::IsRunning()
{
	AutoLocker<BLocker> locker(&fLock);
	for (int32 i = 0; i < fNodes.CountItems(); i++) {
		if (_IsRunning(fNodes.ItemAt(i)))
			return true;
	}

	return false;
}


void
ProcessCoordinator::Start()
{
	_CoordinateAndCallListener();
}


void
ProcessCoordinator::RequestStop()
{
	AutoLocker<BLocker> locker(&fLock);
	if (!fWasStopped) {
		fWasStopped = true;
		HDINFO("[Coordinator] will stop process coordinator");
		for (int32 i = 0; i < fNodes.CountItems(); i++) {
			AbstractProcessNode* node = fNodes.ItemAt(i);
			HDINFO("[Coordinator] stopping process [%s]",
				node->Process()->Name());
			node->RequestStop();
		}
	}
}


status_t
ProcessCoordinator::ErrorStatus()
{
	AutoLocker<BLocker> locker(&fLock);
	for (int32 i = 0; i < fNodes.CountItems(); i++) {
		status_t result = fNodes.ItemAt(i)->Process()->ErrorStatus();

		if (result != B_OK)
			return result;
	}

	return B_OK;
}


float
ProcessCoordinator::Progress()
{
	AutoLocker<BLocker> locker(&fLock);
	float result = 0.0f;

	if (!fWasStopped) {
		int32 count = fNodes.CountItems();

		// if there is only one then return it's value directly because this
		// allows for the indeterminate state of -1.

		if (count == 1)
			result = fNodes.ItemAt(0)->Process()->Progress();
		else {
			float progressPerNode = 1.0f / ((float) count);

			for (int32 i = count - 1; i >= 0; i--) {
				AbstractProcess* process = fNodes.ItemAt(i)->Process();

				switch(process->ProcessState()) {
					case PROCESS_INITIAL:
						break;
					case PROCESS_RUNNING:
						result += (progressPerNode * fmaxf(
							0.0f, fminf(1.0, process->Progress())));
						break;
					case PROCESS_COMPLETE:
						result += progressPerNode;
						break;
				}
			}
		}
	}
	return result;
}


const BString&
ProcessCoordinator::Name() const
{
	return fName;
}


BMessage*
ProcessCoordinator::Message() const
{
	return fMessage;
}


BString
ProcessCoordinator::_CreateStatusMessage()
{
	// work through the nodes and take a description from the first one.  If
	// there are others present then use a 'plus X others' suffix.  Go backwards
	// through the processes so that the most recent activity is shown first.

	BString firstProcessDescription;
	uint32 additionalRunningProcesses = 0;

	for (int32 i = fNodes.CountItems() - 1; i >= 0; i--) {
		AbstractProcess* process = fNodes.ItemAt(i)->Process();

		if (process->ProcessState() == PROCESS_RUNNING) {
			if (firstProcessDescription.IsEmpty()) {
				firstProcessDescription = process->Description();
			} else {
				additionalRunningProcesses++;
			}
		}
	}

	if (firstProcessDescription.IsEmpty())
		return "???";

	if (additionalRunningProcesses == 0)
		return firstProcessDescription;

	static BStringFormat format(B_TRANSLATE(
		"%FIRST_PROCESS_DESCRIPTION% +"
		"{0, plural, one{# process} other{# processes}}"));
	BString result;
	format.Format(result, additionalRunningProcesses);
	result.ReplaceAll("%FIRST_PROCESS_DESCRIPTION%", firstProcessDescription);

	return result;
}


/*! This method assumes that a lock is held on the coordinator. */

ProcessCoordinatorState
ProcessCoordinator::_CreateStatus()
{
	return ProcessCoordinatorState(
		this, Progress(), _CreateStatusMessage(), IsRunning(), ErrorStatus());
}


void
ProcessCoordinator::_CoordinateAndCallListener()
{
	ProcessCoordinatorState state = _Coordinate();

	if (fListener != NULL)
		fListener->CoordinatorChanged(state);
}


ProcessCoordinatorState
ProcessCoordinator::_Coordinate()
{
	HDTRACE("[Coordinator] will coordinate nodes");
	AutoLocker<BLocker> locker(&fLock);
	_StopSuccessorNodesToErroredOrStoppedNodes();

	// go through the nodes and find those that are still to be run and
	// for which the preconditions are met to start.
	for (int32 i = 0; i < fNodes.CountItems(); i++) {
		AbstractProcessNode* node = fNodes.ItemAt(i);

		if (node->Process()->ProcessState() == PROCESS_INITIAL) {
			if (node->AllPredecessorsComplete())
				node->Start();
			else {
				HDTRACE("[Coordinator] all predecessors not complete -> "
					"[%s] not started", node->Process()->Name());
			}
		} else {
			HDTRACE("[Coordinator] process [%s] running or complete",
				node->Process()->Name());
		}
	}

	return _CreateStatus();
}


/*! This method assumes that a lock is held on the coordinator. */

void
ProcessCoordinator::_StopSuccessorNodesToErroredOrStoppedNodes()
{
	for (int32 i = 0; i < fNodes.CountItems(); i++) {
		AbstractProcessNode* node = fNodes.ItemAt(i);
		AbstractProcess* process = node->Process();

		if (process->WasStopped() || process->ErrorStatus() != B_OK)
			_StopSuccessorNodes(node);
	}
}


/*! This method assumes that a lock is held on the coordinator. */

void
ProcessCoordinator::_StopSuccessorNodes(AbstractProcessNode* predecessorNode)
{
	for (int32 i = 0; i < predecessorNode->CountSuccessors(); i++) {
		AbstractProcessNode* node = predecessorNode->SuccessorAt(i);
		AbstractProcess* process = node->Process();

		if (process->ProcessState() == PROCESS_INITIAL) {
			HDDEBUG("[Coordinator] [%s] (failed) --> [%s] (stopping)",
				predecessorNode->Process()->Name(), process->Name());
			node->RequestStop();
			_StopSuccessorNodes(node);
		}
	}
}


bool
ProcessCoordinator::_IsRunning(AbstractProcessNode* node)
{
	return node->Process()->ProcessState() != PROCESS_COMPLETE;
}


int32
ProcessCoordinator::_CountNodesCompleted()
{
	int32 nodesCompleted = 0;
	for (int32 i = 0; i < fNodes.CountItems(); i++) {
		AbstractProcess *process = fNodes.ItemAt(i)->Process();
		if (process->ProcessState() == PROCESS_COMPLETE)
			nodesCompleted++;
	}
	return nodesCompleted;
}
