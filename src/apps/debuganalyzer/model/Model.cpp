/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Model.h"

#include <new>

#include <stdio.h>
#include <stdlib.h>

#include <AutoDeleter.h>


// #pragma mark - WaitObject


Model::WaitObject::WaitObject(const system_profiler_wait_object_info* event)
	:
	fEvent(event),
	fWaits(0),
	fTotalWaitTime(0)
{
}


Model::WaitObject::~WaitObject()
{
}


void
Model::WaitObject::AddWait(nanotime_t waitTime)
{
	fWaits++;
	fTotalWaitTime += waitTime;
}


// #pragma mark - WaitObjectGroup


Model::WaitObjectGroup::WaitObjectGroup(WaitObject* waitObject)
	:
	fWaits(-1),
	fTotalWaitTime(-1)
{
	fWaitObjects.AddItem(waitObject);
}


Model::WaitObjectGroup::~WaitObjectGroup()
{
}


int64
Model::WaitObjectGroup::Waits()
{
	if (fWaits < 0)
		_ComputeWaits();

	return fWaits;
}


nanotime_t
Model::WaitObjectGroup::TotalWaitTime()
{
	if (fTotalWaitTime < 0)
		_ComputeWaits();

	return fTotalWaitTime;
}


void
Model::WaitObjectGroup::_ComputeWaits()
{
	fWaits = 0;
	fTotalWaitTime = 0;

	for (int32 i = fWaitObjects.CountItems(); i-- > 0;) {
		WaitObject* waitObject = fWaitObjects.ItemAt(i);

		fWaits += waitObject->Waits();
		fTotalWaitTime += waitObject->TotalWaitTime();
	}
}


// #pragma mark - ThreadWaitObject


Model::ThreadWaitObject::ThreadWaitObject(WaitObject* waitObject)
	:
	fWaitObject(waitObject),
	fWaits(0),
	fTotalWaitTime(0)
{
}


Model::ThreadWaitObject::~ThreadWaitObject()
{
}


void
Model::ThreadWaitObject::AddWait(nanotime_t waitTime)
{
	fWaits++;
	fTotalWaitTime += waitTime;

	fWaitObject->AddWait(waitTime);
}


// #pragma mark - ThreadWaitObjectGroup


Model::ThreadWaitObjectGroup::ThreadWaitObjectGroup(
	ThreadWaitObject* threadWaitObject)
{
	fWaitObjects.Add(threadWaitObject);
}


Model::ThreadWaitObjectGroup::~ThreadWaitObjectGroup()
{
}


bool
Model::ThreadWaitObjectGroup::GetThreadWaitObjects(
	BObjectList<ThreadWaitObject>& objects)
{
	ThreadWaitObjectList::Iterator it = fWaitObjects.GetIterator();
	while (ThreadWaitObject* object = it.Next()) {
		if (!objects.AddItem(object))
			return false;
	}

	return true;
}


// #pragma mark - Team


Model::Team::Team(const system_profiler_team_added* event, nanotime_t time)
	:
	fCreationEvent(event),
	fCreationTime(time),
	fDeletionTime(-1),
	fThreads(10)
{
}


Model::Team::~Team()
{
}


bool
Model::Team::AddThread(Thread* thread)
{
	return fThreads.BinaryInsert(thread, &Thread::CompareByCreationTimeID);
}


// #pragma mark - Thread


Model::Thread::Thread(Team* team, const system_profiler_thread_added* event,
	nanotime_t time)
	:
	fTeam(team),
	fCreationEvent(event),
	fCreationTime(time),
	fDeletionTime(-1),
	fRuns(0),
	fTotalRunTime(0),
	fMinRunTime(-1),
	fMaxRunTime(-1),
	fLatencies(0),
	fTotalLatency(0),
	fMinLatency(-1),
	fMaxLatency(-1),
	fReruns(0),
	fTotalRerunTime(0),
	fMinRerunTime(-1),
	fMaxRerunTime(-1),
	fWaits(0),
	fTotalWaitTime(0),
	fUnspecifiedWaitTime(0),
	fPreemptions(0),
	fIndex(-1),
	fWaitObjectGroups(20, true)
{
}


Model::Thread::~Thread()
{
}


Model::ThreadWaitObjectGroup*
Model::Thread::ThreadWaitObjectGroupFor(uint32 type, addr_t object) const
{
	type_and_object key;
	key.type = type;
	key.object = object;

	return fWaitObjectGroups.BinarySearchByKey(key,
		&ThreadWaitObjectGroup::CompareWithTypeObject);
}


void
Model::Thread::AddRun(nanotime_t runTime)
{
	fRuns++;
	fTotalRunTime += runTime;

	if (fMinRunTime < 0 || runTime < fMinRunTime)
		fMinRunTime = runTime;
	if (runTime > fMaxRunTime)
		fMaxRunTime = runTime;
}


void
Model::Thread::AddRerun(nanotime_t runTime)
{
	fReruns++;
	fTotalRerunTime += runTime;

	if (fMinRerunTime < 0 || runTime < fMinRerunTime)
		fMinRerunTime = runTime;
	if (runTime > fMaxRerunTime)
		fMaxRerunTime = runTime;
}


void
Model::Thread::AddLatency(nanotime_t latency)
{
	fLatencies++;
	fTotalLatency += latency;

	if (fMinLatency < 0 || latency < fMinLatency)
		fMinLatency = latency;
	if (latency > fMaxLatency)
		fMaxLatency = latency;
}


void
Model::Thread::AddPreemption(nanotime_t runTime)
{
	fPreemptions++;
}


void
Model::Thread::AddWait(nanotime_t waitTime)
{
	fWaits++;
	fTotalWaitTime += waitTime;
}


void
Model::Thread::AddUnspecifiedWait(nanotime_t waitTime)
{
	fUnspecifiedWaitTime += waitTime;
}


Model::ThreadWaitObject*
Model::Thread::AddThreadWaitObject(WaitObject* waitObject,
	ThreadWaitObjectGroup** _threadWaitObjectGroup)
{
	// create a thread wait object
	ThreadWaitObject* threadWaitObject
		= new(std::nothrow) ThreadWaitObject(waitObject);
	if (threadWaitObject == NULL)
		return NULL;

	// find the thread wait object group
	ThreadWaitObjectGroup* threadWaitObjectGroup
		= ThreadWaitObjectGroupFor(waitObject->Type(), waitObject->Object());
	if (threadWaitObjectGroup == NULL) {
		// doesn't exist yet -- create
		threadWaitObjectGroup = new(std::nothrow) ThreadWaitObjectGroup(
			threadWaitObject);
		if (threadWaitObjectGroup == NULL) {
			delete threadWaitObject;
			return NULL;
		}

		// add to the list
		if (!fWaitObjectGroups.BinaryInsert(threadWaitObjectGroup,
				&ThreadWaitObjectGroup::CompareByTypeObject)) {
			delete threadWaitObjectGroup;
			return NULL;
		}
	} else {
		// exists -- just add the object
		threadWaitObjectGroup->AddWaitObject(threadWaitObject);
	}

	if (_threadWaitObjectGroup != NULL)
		*_threadWaitObjectGroup = threadWaitObjectGroup;

	return threadWaitObject;
}


// #pragma mark - SchedulingState


Model::SchedulingState::~SchedulingState()
{
	Clear();
}


status_t
Model::SchedulingState::Init()
{
	status_t error = fThreadStates.Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
Model::SchedulingState::Init(const CompactSchedulingState* state)
{
	status_t error = Init();
	if (error != B_OK)
		return error;

	if (state == NULL)
		return B_OK;

	fLastEventTime = state->LastEventTime();
	for (int32 i = 0; const CompactThreadSchedulingState* compactThreadState
			= state->ThreadStateAt(i); i++) {
		ThreadSchedulingState* threadState
			= new(std::nothrow) ThreadSchedulingState(*compactThreadState);
		if (threadState == NULL)
			return B_NO_MEMORY;

		fThreadStates.Insert(threadState);
	}

	return B_OK;
}


void
Model::SchedulingState::Clear()
{
	ThreadSchedulingState* state = fThreadStates.Clear(true);
	while (state != NULL) {
		ThreadSchedulingState* next = state->next;
		delete state;
		state = next;
	}

	fLastEventTime = -1;
}


// #pragma mark - CompactSchedulingState


/*static*/ Model::CompactSchedulingState*
Model::CompactSchedulingState::Create(const SchedulingState& state,
	off_t eventOffset)
{
	nanotime_t lastEventTime = state.LastEventTime();

	// count the active threads
	int32 threadCount = 0;
	for (ThreadSchedulingStateTable::Iterator it
				= state.ThreadStates().GetIterator();
			ThreadSchedulingState* threadState = it.Next();) {
		Thread* thread = threadState->thread;
		if (thread->CreationTime() <= lastEventTime
			&& (thread->DeletionTime() == -1
				|| thread->DeletionTime() >= lastEventTime)) {
			threadCount++;
		}
	}

	CompactSchedulingState* compactState = (CompactSchedulingState*)malloc(
		sizeof(CompactSchedulingState)
			+ threadCount * sizeof(CompactThreadSchedulingState));
	if (compactState == NULL)
		return NULL;

	// copy the state info
	compactState->fEventOffset = eventOffset;
	compactState->fThreadCount = threadCount;
	compactState->fLastEventTime = lastEventTime;

	int32 threadIndex = 0;
	for (ThreadSchedulingStateTable::Iterator it
				= state.ThreadStates().GetIterator();
			ThreadSchedulingState* threadState = it.Next();) {
		Thread* thread = threadState->thread;
		if (thread->CreationTime() <= lastEventTime
			&& (thread->DeletionTime() == -1
				|| thread->DeletionTime() >= lastEventTime)) {
			compactState->fThreadStates[threadIndex++] = *threadState;
		}
	}

	return compactState;
}


void
Model::CompactSchedulingState::Delete()
{
	free(this);
}


// #pragma mark - Model


Model::Model(const char* dataSourceName, void* eventData, size_t eventDataSize)
	:
	fDataSourceName(dataSourceName),
	fEventData(eventData),
	fEventDataSize(eventDataSize),
	fBaseTime(0),
	fLastEventTime(0),
	fTeams(20, true),
	fThreads(20, true),
	fWaitObjectGroups(20, true),
	fSchedulingStates(100)
{
}


Model::~Model()
{
	for (int32 i = 0; CompactSchedulingState* state
		= fSchedulingStates.ItemAt(i); i++) {
		state->Delete();
	}

	free(fEventData);
}


void
Model::LoadingFinished()
{
	for (int32 i = 0; Thread* thread = fThreads.ItemAt(i); i++)
		thread->SetIndex(i);
}


void
Model::SetBaseTime(nanotime_t time)
{
	fBaseTime = time;
}


void
Model::SetLastEventTime(nanotime_t time)
{
	fLastEventTime = time;
}


int32
Model::CountTeams() const
{
		return fTeams.CountItems();
}


Model::Team*
Model::TeamAt(int32 index) const
{
	return fTeams.ItemAt(index);
}


Model::Team*
Model::TeamByID(team_id id) const
{
	return fTeams.BinarySearchByKey(id, &Team::CompareWithID);
}


Model::Team*
Model::AddTeam(const system_profiler_team_added* event, nanotime_t time)
{
	Team* team = TeamByID(event->team);
	if (team != NULL) {
		fprintf(stderr, "Duplicate team: %ld\n", event->team);
		// TODO: User feedback!
		return team;
	}

	team = new(std::nothrow) Team(event, time);
	if (team == NULL)
		return NULL;

	if (!fTeams.BinaryInsert(team, &Team::CompareByID)) {
		delete team;
		return NULL;
	}

	return team;
}


int32
Model::CountThreads() const
{
	return fThreads.CountItems();
}


Model::Thread*
Model::ThreadAt(int32 index) const
{
	return fThreads.ItemAt(index);
}


Model::Thread*
Model::ThreadByID(thread_id id) const
{
	return fThreads.BinarySearchByKey(id, &Thread::CompareWithID);
}


Model::Thread*
Model::AddThread(const system_profiler_thread_added* event, nanotime_t time)
{
	// check whether we do already know the thread
	Thread* thread = ThreadByID(event->thread);
	if (thread != NULL) {
		fprintf(stderr, "Duplicate thread: %ld\n", event->thread);
		// TODO: User feedback!
		return thread;
	}

	// get its team
	Team* team = TeamByID(event->team);
	if (team == NULL) {
		fprintf(stderr, "No team for thread: %ld\n", event->thread);
		return NULL;
	}

	// create the thread and add it
	thread = new(std::nothrow) Thread(team, event, time);
	if (thread == NULL)
		return NULL;
	ObjectDeleter<Thread> threadDeleter(thread);

	if (!fThreads.BinaryInsert(thread, &Thread::CompareByID))
		return NULL;

	if (!team->AddThread(thread)) {
		fThreads.RemoveItem(thread);
		return NULL;
	}

	threadDeleter.Detach();
	return thread;
}


Model::WaitObject*
Model::AddWaitObject(const system_profiler_wait_object_info* event,
	WaitObjectGroup** _waitObjectGroup)
{
	// create a wait object
	WaitObject* waitObject = new(std::nothrow) WaitObject(event);
	if (waitObject == NULL)
		return NULL;

	// find the wait object group
	WaitObjectGroup* waitObjectGroup
		= WaitObjectGroupFor(waitObject->Type(), waitObject->Object());
	if (waitObjectGroup == NULL) {
		// doesn't exist yet -- create
		waitObjectGroup = new(std::nothrow) WaitObjectGroup(waitObject);
		if (waitObjectGroup == NULL) {
			delete waitObject;
			return NULL;
		}

		// add to the list
		if (!fWaitObjectGroups.BinaryInsert(waitObjectGroup,
				&WaitObjectGroup::CompareByTypeObject)) {
			delete waitObjectGroup;
			return NULL;
		}
	} else {
		// exists -- just add the object
		waitObjectGroup->AddWaitObject(waitObject);
	}

	if (_waitObjectGroup != NULL)
		*_waitObjectGroup = waitObjectGroup;

	return waitObject;
}


int32
Model::CountWaitObjectGroups() const
{
	return fWaitObjectGroups.CountItems();
}


Model::WaitObjectGroup*
Model::WaitObjectGroupAt(int32 index) const
{
	return fWaitObjectGroups.ItemAt(index);
}


Model::WaitObjectGroup*
Model::WaitObjectGroupFor(uint32 type, addr_t object) const
{
	type_and_object key;
	key.type = type;
	key.object = object;

	return fWaitObjectGroups.BinarySearchByKey(key,
		&WaitObjectGroup::CompareWithTypeObject);
}


Model::ThreadWaitObject*
Model::AddThreadWaitObject(thread_id threadID, WaitObject* waitObject,
	ThreadWaitObjectGroup** _threadWaitObjectGroup)
{
	Thread* thread = ThreadByID(threadID);
	if (thread == NULL)
		return NULL;

	return thread->AddThreadWaitObject(waitObject, _threadWaitObjectGroup);
}


Model::ThreadWaitObjectGroup*
Model::ThreadWaitObjectGroupFor(thread_id threadID, uint32 type, addr_t object) const
{
	Thread* thread = ThreadByID(threadID);
	if (thread == NULL)
		return NULL;

	return thread->ThreadWaitObjectGroupFor(type, object);
}


bool
Model::AddSchedulingStateSnapshot(const SchedulingState& state,
	off_t eventOffset)
{
	CompactSchedulingState* compactState = CompactSchedulingState::Create(state,
		eventOffset);
	if (compactState == NULL)
		return false;

	if (!fSchedulingStates.AddItem(compactState)) {
		compactState->Delete();
		return false;
	}

	return true;
}


const Model::CompactSchedulingState*
Model::ClosestSchedulingState(nanotime_t eventTime) const
{
	int32 index = fSchedulingStates.BinarySearchIndexByKey(eventTime,
		&_CompareEventTimeSchedulingState);
	if (index >= 0)
		return fSchedulingStates.ItemAt(index);

	// no exact match
	index = -index - 1;
	return index > 0 ? fSchedulingStates.ItemAt(index - 1) : NULL;
}


/*static*/ int
Model::_CompareEventTimeSchedulingState(const nanotime_t* time,
	const CompactSchedulingState* state)
{
	if (*time < state->LastEventTime())
		return -1;
	return *time == state->LastEventTime() ? 0 : 1;
}

