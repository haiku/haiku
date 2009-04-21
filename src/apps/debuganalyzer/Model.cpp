/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Model.h"

#include <new>

#include <stdio.h>
#include <stdlib.h>

#include <AutoDeleter.h>


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
	fDeletionTime(-1)
{
}


Model::Thread::~Thread()
{
}


// #pragma mark - Model


Model::Model(void* eventData, size_t eventDataSize)
	:
	fEventData(eventData),
	fEventDataSize(eventDataSize),
	fTeams(20, true),
	fThreads(20, true)
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
