/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Model.h"

#include <new>

#include <stdio.h>
#include <stdlib.h>

#include <AutoDeleter.h>

#include <thread_defs.h>



static const char* const kThreadStateNames[] = {
	"running",
	"still running",
	"preempted",
	"ready",
	"waiting",
	"unknown"
};


const char*
thread_state_name(ThreadState state)
{
	return kThreadStateNames[state];
}


const char*
wait_object_type_name(uint32 type)
{
	switch (type) {
		case THREAD_BLOCK_TYPE_SEMAPHORE:
			return "semaphore";
		case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
			return "condition";
		case THREAD_BLOCK_TYPE_MUTEX:
			return "mutex";
		case THREAD_BLOCK_TYPE_RW_LOCK:
			return "rw lock";
		case THREAD_BLOCK_TYPE_OTHER:
			return "other";
		case THREAD_BLOCK_TYPE_SNOOZE:
			return "snooze";
		case THREAD_BLOCK_TYPE_SIGNAL:
			return "signal";
		default:
			return "unknown";
	}
}


// #pragma mark - CPU


Model::CPU::CPU()
	:
	fIdleTime(0)
{
}


void
Model::CPU::SetIdleTime(nanotime_t time)
{
	fIdleTime = time;
}


// #pragma mark - IORequest


Model::IORequest::IORequest(
	system_profiler_io_request_scheduled* scheduledEvent,
	system_profiler_io_request_finished* finishedEvent, size_t operationCount)
	:
	scheduledEvent(scheduledEvent),
	finishedEvent(finishedEvent),
	operationCount(operationCount)
{
}


Model::IORequest::~IORequest()
{
}


/*static*/ Model::IORequest*
Model::IORequest::Create(system_profiler_io_request_scheduled* scheduledEvent,
	system_profiler_io_request_finished* finishedEvent, size_t operationCount)
{
	void* memory = malloc(
		sizeof(IORequest) + operationCount * sizeof(IOOperation));
	if (memory == NULL)
		return NULL;

	return new(memory) IORequest(scheduledEvent, finishedEvent, operationCount);
}


void
Model::IORequest::Delete()
{
	free(this);
}


// #pragma mark - IOScheduler


Model::IOScheduler::IOScheduler(system_profiler_io_scheduler_added* event,
	int32 index)
	:
	fAddedEvent(event),
	fIndex(index)
{
}


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
	fEvents(NULL),
	fEventCount(0),
	fIORequests(NULL),
	fIORequestCount(0),
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
	fIOCount(0),
	fIOTime(0),
	fPreemptions(0),
	fIndex(-1),
	fWaitObjectGroups(20, true)
{
}


Model::Thread::~Thread()
{
	if (fIORequests != NULL) {
		for (size_t i = 0; i < fIORequestCount; i++)
			fIORequests[i]->Delete();

		delete[] fIORequests;
	}

	delete[] fEvents;
}


void
Model::Thread::SetEvents(system_profiler_event_header** events,
	size_t eventCount)
{
	fEvents = events;
	fEventCount = eventCount;
}


void
Model::Thread::SetIORequests(IORequest** requests, size_t requestCount)
{
	fIORequests = requests;
	fIORequestCount = requestCount;
}


size_t
Model::Thread::ClosestRequestStartIndex(nanotime_t minRequestStartTime) const
{
	size_t lower = 0;
	size_t upper = fIORequestCount;
	while (lower < upper) {
		size_t mid = (lower + upper) / 2;
		IORequest* request = fIORequests[mid];

		if (request->ScheduledTime() < minRequestStartTime)
			lower = mid + 1;
		else
			upper = mid;
	}

	return lower;
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


void
Model::Thread::SetIOs(int64 count, nanotime_t time)
{
	fIOCount = count;
	fIOTime = time;
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
		DeleteThread(state);
		state = next;
	}

	fLastEventTime = -1;
}

void
Model::SchedulingState::DeleteThread(ThreadSchedulingState* thread)
{
	delete thread;
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


Model::Model(const char* dataSourceName, void* eventData, size_t eventDataSize,
	system_profiler_event_header** events, size_t eventCount)
	:
	fDataSourceName(dataSourceName),
	fEventData(eventData),
	fEvents(events),
	fEventDataSize(eventDataSize),
	fEventCount(eventCount),
	fCPUCount(1),
	fBaseTime(0),
	fLastEventTime(0),
	fIdleTime(0),
	fCPUs(20, true),
	fTeams(20, true),
	fThreads(20, true),
	fWaitObjectGroups(20, true),
	fIOSchedulers(10, true),
	fSchedulingStates(100)
{
}


Model::~Model()
{
	for (int32 i = 0; CompactSchedulingState* state
		= fSchedulingStates.ItemAt(i); i++) {
		state->Delete();
	}

	delete[] fEvents;

	free(fEventData);

	for (int32 i = 0; void* data = fAssociatedData.ItemAt(i); i++)
		free(data);
}


size_t
Model::ClosestEventIndex(nanotime_t eventTime) const
{
	// The events themselves are unmodified and use an absolute time.
	eventTime += fBaseTime;

	// Binary search the event. Since not all events have a timestamp, we have
	// to do a bit of iteration, too.
	size_t lower = 0;
	size_t upper = CountEvents();
	while (lower < upper) {
		size_t mid = (lower + upper) / 2;
		while (mid < upper) {
			system_profiler_event_header* header = fEvents[mid];
			switch (header->event) {
				case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
				case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
				case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
					break;
				default:
					mid++;
					continue;
			}

			break;
		}

		if (mid == upper) {
			lower = mid;
			break;
		}

		system_profiler_thread_scheduling_event* event
			= (system_profiler_thread_scheduling_event*)(fEvents[mid] + 1);
		if (event->time < eventTime)
			lower = mid + 1;
		else
			upper = mid;
	}

	return lower;
}


bool
Model::AddAssociatedData(void* data)
{
	return fAssociatedData.AddItem(data);
}


void
Model::RemoveAssociatedData(void* data)
{
	fAssociatedData.RemoveItem(data);
}


void
Model::LoadingFinished()
{
	// set the thread indices
	for (int32 i = 0; Thread* thread = fThreads.ItemAt(i); i++)
		thread->SetIndex(i);

	// compute the total idle time
	fIdleTime = 0;
	for (int32 i = 0; CPU* cpu = CPUAt(i); i++)
		fIdleTime += cpu->IdleTime();
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


bool
Model::SetCPUCount(int32 count)
{
	fCPUCount = count;

	fCPUs.MakeEmpty();

	for (int32 i = 0; i < fCPUCount; i++) {
		CPU* cpu = new(std::nothrow) CPU;
		if (cpu == NULL || !fCPUs.AddItem(cpu)) {
			delete cpu;
			return false;
		}
	}

	return true;
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


int32
Model::CountIOSchedulers() const
{
	return fIOSchedulers.CountItems();
}


Model::IOScheduler*
Model::IOSchedulerAt(int32 index) const
{
	return fIOSchedulers.ItemAt(index);
}


Model::IOScheduler*
Model::IOSchedulerByID(int32 id) const
{
	for (int32 i = 0; IOScheduler* scheduler = fIOSchedulers.ItemAt(i); i++) {
		if (scheduler->ID() == id)
			return scheduler;
	}

	return NULL;
}


Model::IOScheduler*
Model::AddIOScheduler(system_profiler_io_scheduler_added* event)
{
	IOScheduler* scheduler = new(std::nothrow) IOScheduler(event,
		fIOSchedulers.CountItems());
	if (scheduler == NULL || !fIOSchedulers.AddItem(scheduler)) {
		delete scheduler;
		return NULL;
	}

	return scheduler;
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

