/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadModelLoader.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>
#include <DebugEventStream.h>

#include "ThreadModel.h"


static int
compare_by_type_and_name(const Model::ThreadWaitObject* a,
	const Model::ThreadWaitObject* b)
{
	if (a->Type() != b->Type())
		return a->Type() < b->Type() ? -1 : 1;

	return strcmp(a->Name(), b->Name());
}


// #pragma mark -


ThreadModelLoader::ThreadModelLoader(Model* model, Model::Thread* thread,
	const BMessenger& target, void* targetCookie)
	:
	AbstractModelLoader(target, targetCookie),
	fModel(model),
	fThread(thread),
	fThreadModel(NULL)
{
}


ThreadModelLoader::~ThreadModelLoader()
{
	delete fThreadModel;
}


ThreadModel*
ThreadModelLoader::DetachModel()
{
	AutoLocker<BLocker> locker(fLock);

	if (fThreadModel == NULL || fLoading)
		return NULL;

	ThreadModel* model = fThreadModel;
	fThreadModel = NULL;

	return model;
}


status_t
ThreadModelLoader::PrepareForLoading()
{
	return B_OK;
}


status_t
ThreadModelLoader::Load()
{
	try {
		return _Load();
	} catch(...) {
		return B_ERROR;
	}
}


void
ThreadModelLoader::FinishLoading(bool success)
{
	if (!success) {
		delete fThreadModel;
		fThreadModel = NULL;
	}
}


status_t
ThreadModelLoader::_Load()
{
	// create a model
	fThreadModel = new(std::nothrow) ThreadModel(fModel, fThread);
	if (fThreadModel == NULL)
		return B_NO_MEMORY;

	// collect all wait objects
	BObjectList<Model::ThreadWaitObject> waitObjects;

	int32 groupCount = fThread->CountThreadWaitObjectGroups();
	for (int32 i = 0; i < groupCount; i++) {
		Model::ThreadWaitObjectGroup* group
			= fThread->ThreadWaitObjectGroupAt(i);

		if (!group->GetThreadWaitObjects(waitObjects))
			return B_NO_MEMORY;
	}

	// sort them by type and name
	waitObjects.SortItems(&compare_by_type_and_name);

	// create the groups
	int32 waitObjectCount = waitObjects.CountItems();
printf("%ld wait objects\n", waitObjectCount);
	for (int32 i = 0; i < waitObjectCount;) {
printf("new wait object group at %ld\n", i);
		// collect the objects for this group
		Model::ThreadWaitObject* firstObject = waitObjects.ItemAt(i);
		int32 k = i + 1;
		for (; k < waitObjectCount; k++) {
			if (compare_by_type_and_name(firstObject, waitObjects.ItemAt(k))
					!= 0) {
				break;
			}
		}

		if (fThreadModel->AddWaitObjectGroup(waitObjects, i, k) == NULL)
			return B_NO_MEMORY;

		i = k;
	}

	// filter the events
	thread_id threadID = fThread->ID();
	bool done = false;
	uint32 count = 0;

	system_profiler_event_header** events = fModel->Events();
	size_t eventCount = fModel->CountEvents();
	for (size_t i = 0; i < eventCount; i++) {
		system_profiler_event_header* header = events[i];
		void* buffer = header + 1;

		// process the event
		bool keepEvent = false;

		switch (header->event) {
			case B_SYSTEM_PROFILER_THREAD_REMOVED:
			{
				system_profiler_thread_removed* event
					= (system_profiler_thread_removed*)buffer;
				if (event->thread == threadID)
					done = true;
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
			{
				system_profiler_thread_scheduled* event
					= (system_profiler_thread_scheduled*)buffer;
				keepEvent = event->thread == threadID
					|| event->previous_thread == threadID ;
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
			{
				thread_enqueued_in_run_queue* event
					= (thread_enqueued_in_run_queue*)buffer;
				keepEvent = event->thread == threadID;
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
			{
				thread_removed_from_run_queue* event
					= (thread_removed_from_run_queue*)buffer;
				keepEvent = event->thread == threadID;
				break;
			}

			default:
				break;
		}

		if (keepEvent)
			fThreadModel->AddSchedulingEvent(header);

		// periodically check whether we're supposed to abort
		if (++count % 32 == 0) {
			AutoLocker<BLocker> locker(fLock);
			if (fAborted)
				return B_ERROR;
		}
	}

	return B_OK;
}
