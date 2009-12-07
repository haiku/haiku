/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ThreadModel.h"

#include <new>


// #pragma mark - WaitObjectGroup


ThreadModel::WaitObjectGroup::WaitObjectGroup(
	Model::ThreadWaitObject** waitObjects, int32 count)
	:
	fWaitObjects(waitObjects),
	fCount(count),
	fWaits(0),
	fTotalWaitTime(0)
{
	for (int32 i = 0; i < fCount; i++) {
		fWaits += fWaitObjects[i]->Waits();
		fTotalWaitTime += fWaitObjects[i]->TotalWaitTime();
	}
}


ThreadModel::WaitObjectGroup::~WaitObjectGroup()
{
	delete[] fWaitObjects;
}


// #pragma mark - ThreadModel


ThreadModel::ThreadModel(Model* model, Model::Thread* thread)
	:
	fModel(model),
	fThread(thread),
	fWaitObjectGroups(10, true)
{
}


ThreadModel::~ThreadModel()
{
}


ThreadModel::WaitObjectGroup*
ThreadModel::AddWaitObjectGroup(
	const BObjectList<Model::ThreadWaitObject>& waitObjects, int32 start,
	int32 end)
{
	// check params
	int32 count = end - start;
	if (start < 0 || count <= 0 || waitObjects.CountItems() < end)
		return NULL;

	// create an array of the wait object
	Model::ThreadWaitObject** objects
		= new(std::nothrow) Model::ThreadWaitObject*[count];
	if (objects == NULL)
		return NULL;

	for (int32 i = 0; i < count; i++)
		objects[i] = waitObjects.ItemAt(start + i);

	// create and add the group
	WaitObjectGroup* group = new(std::nothrow) WaitObjectGroup(objects, count);
	if (group == NULL) {
		delete[] objects;
		return NULL;
	}

	if (!fWaitObjectGroups.BinaryInsert(group,
			&WaitObjectGroup::CompareByTypeName)) {
		delete group;
		return NULL;
	}

	return group;
}


bool
ThreadModel::AddSchedulingEvent(const system_profiler_event_header* eventHeader)
{
	return fSchedulingEvents.AddItem(eventHeader);
}


int32
ThreadModel::FindSchedulingEvent(nanotime_t time)
{
	if (time < 0)
		return 0;

	time += fModel->BaseTime();

	int32 lower = 0;
	int32 upper = CountSchedulingEvents() - 1;

	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		const system_profiler_event_header* header = SchedulingEventAt(mid);
		system_profiler_thread_scheduling_event* event
			=  (system_profiler_thread_scheduling_event*)(header + 1);
		if (event->time < time)
			lower = mid + 1;
		else
			upper = mid;
	}

	// We've found the first event that has a time >= the given time. If its
	// time is >, we rather return the previous event.
	if (lower > 0) {
		const system_profiler_event_header* header = SchedulingEventAt(lower);
		system_profiler_thread_scheduling_event* event
			=  (system_profiler_thread_scheduling_event*)(header + 1);
		if (event->time > time)
			lower--;
	}

	return lower;
}
