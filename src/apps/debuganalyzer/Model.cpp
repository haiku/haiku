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
	fEvent(event)
{
}


Model::WaitObject::~WaitObject()
{
}


// #pragma mark - WaitObjectGroup


Model::WaitObjectGroup::WaitObjectGroup(WaitObject* waitObject)
{
	fWaitObjects.Add(waitObject);
}


Model::WaitObjectGroup::~WaitObjectGroup()
{
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
Model::ThreadWaitObject::AddWait(bigtime_t waitTime)
{
	fWaits++;
	fTotalWaitTime += waitTime;
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


// #pragma mark - Team


Model::Team::Team(const system_profiler_team_added* event, bigtime_t time)
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
	bigtime_t time)
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
Model::Thread::AddRun(bigtime_t runTime)
{
	fRuns++;
	fTotalRunTime += runTime;

	if (fMinRunTime < 0 || runTime < fMinRunTime)
		fMinRunTime = runTime;
	if (runTime > fMaxRunTime)
		fMaxRunTime = runTime;
}


void
Model::Thread::AddRerun(bigtime_t runTime)
{
	fReruns++;
	fTotalRerunTime += runTime;

	if (fMinRerunTime < 0 || runTime < fMinRerunTime)
		fMinRerunTime = runTime;
	if (runTime > fMaxRerunTime)
		fMaxRerunTime = runTime;
}


void
Model::Thread::AddLatency(bigtime_t latency)
{
	fLatencies++;
	fTotalLatency += latency;

	if (fMinLatency < 0 || latency < fMinLatency)
		fMinLatency = latency;
	if (latency > fMaxLatency)
		fMaxLatency = latency;
}


void
Model::Thread::AddPreemption(bigtime_t runTime)
{
	fPreemptions++;
}


void
Model::Thread::AddWait(bigtime_t waitTime)
{
	fWaits++;
	fTotalWaitTime += waitTime;
}


void
Model::Thread::AddUnspecifiedWait(bigtime_t waitTime)
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


// #pragma mark - Model


Model::Model(void* eventData, size_t eventDataSize)
	:
	fEventData(eventData),
	fEventDataSize(eventDataSize),
	fTeams(20, true),
	fThreads(20, true),
	fWaitObjectGroups(20, true)
{
}


Model::~Model()
{
	free(fEventData);
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
Model::AddTeam(const system_profiler_team_added* event, bigtime_t time)
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
Model::AddThread(const system_profiler_thread_added* event, bigtime_t time)
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
