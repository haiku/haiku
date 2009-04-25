/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ModelLoader.h"

#include <stdio.h>
#include <string.h>

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <DebugEventStream.h>

#include <system_profiler_defs.h>
#include <thread_defs.h>

#include "DataSource.h"
#include "MessageCodes.h"
#include "Model.h"


struct SimpleWaitObjectInfo : system_profiler_wait_object_info {
	SimpleWaitObjectInfo(uint32 type)
	{
		this->type = type;
		object = 0;
		referenced_object = 0;
		name[0] = '\0';
	}
};


static const SimpleWaitObjectInfo kSnoozeWaitObjectInfo(
	THREAD_BLOCK_TYPE_SNOOZE);
static const SimpleWaitObjectInfo kSignalWaitObjectInfo(
	THREAD_BLOCK_TYPE_SIGNAL);


// #pragma mark - ThreadInfo


ModelLoader::ThreadInfo::ThreadInfo(Model::Thread* thread)
	:
	thread(thread),
	state(UNKNOWN),
	lastTime(0),
	waitObject(NULL)
{
}


// #pragma mark -


inline void
ModelLoader::_UpdateLastEventTime(bigtime_t time)
{
	if (fBaseTime < 0)
		fBaseTime = time;

	fLastEventTime = time - fBaseTime;
}


ModelLoader::ModelLoader(DataSource* dataSource,
	const BMessenger& target, void* targetCookie)
	:
	fLock("main model loader"),
	fModel(NULL),
	fDataSource(dataSource),
	fTarget(target),
	fTargetCookie(targetCookie),
	fLoaderThread(-1),
	fLoading(false),
	fAborted(false)
{
}


ModelLoader::~ModelLoader()
{
	if (fLoaderThread >= 0) {
		Abort();
		wait_for_thread(fLoaderThread, NULL);
	}

	delete fDataSource;
	delete fModel;
}


status_t
ModelLoader::StartLoading()
{
	// check initialization
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	AutoLocker<BLocker> locker(fLock);

	if (fModel != NULL || fLoading || fDataSource == NULL)
		return B_BAD_VALUE;

	// init the hash tables
	error = fThreads.Init();
	if (error != B_OK)
		return error;

	// spawn the loader thread
	fLoaderThread = spawn_thread(&_LoaderEntry, "main model loader",
		B_NORMAL_PRIORITY, this);
	if (fLoaderThread < 0)
		return fLoaderThread;

	fLoading = true;
	fAborted = false;

	resume_thread(fLoaderThread);

	return B_OK;
}


void
ModelLoader::Abort()
{
	AutoLocker<BLocker> locker(fLock);

	if (!fLoading || fAborted)
		return;

	fAborted = true;
}


Model*
ModelLoader::DetachModel()
{
	AutoLocker<BLocker> locker(fLock);

	if (fModel == NULL || fLoading)
		return NULL;

	Model* model = fModel;
	fModel = NULL;

	return model;
}


/*static*/ status_t
ModelLoader::_LoaderEntry(void* data)
{
	return ((ModelLoader*)data)->_Loader();
}


status_t
ModelLoader::_Loader()
{
	status_t error;
	try {
		error = _Load();
	} catch(...) {
		error = B_ERROR;
	}
	
	// clean up and notify the target
	AutoLocker<BLocker> locker(fLock);

	ThreadInfo* threadInfo = fThreads.Clear(true);
	while (threadInfo != NULL) {
		ThreadInfo* nextInfo = threadInfo->fNext;
		delete threadInfo;
		threadInfo = nextInfo;
	}

	BMessage message;
	if (error == B_OK) {
		message.what = MSG_MODEL_LOADED_SUCCESSFULLY;
	} else {
		delete fModel;
		fModel = NULL;

		message.what = fAborted
			? MSG_MODEL_LOADED_ABORTED : MSG_MODEL_LOADED_FAILED;
	}

	message.AddPointer("loader", this);
	message.AddPointer("targetCookie", fTargetCookie);
	fTarget.SendMessage(&message);

	fLoading = false;

	return B_OK;
}


status_t
ModelLoader::_Load()
{
	// read the complete data into memory
	void* eventData;
	size_t eventDataSize;
	status_t error = _ReadDebugEvents(&eventData, &eventDataSize);
	if (error != B_OK)
		return error;

	// create a model
	fModel = new(std::nothrow) Model(eventData, eventDataSize);
	if (fModel == NULL) {
		free(eventData);
		return B_NO_MEMORY;
	}

	// create a debug input stream
	BDebugEventInputStream* input = new(std::nothrow) BDebugEventInputStream;
	if (input == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BDebugEventInputStream> inputDeleter(input);

	error = input->SetTo(eventData, eventDataSize, false);
	if (error != B_OK)
		return error;

	// add the snooze and signal wait objects to the model
	if (fModel->AddWaitObject(&kSnoozeWaitObjectInfo, NULL) == NULL
		|| fModel->AddWaitObject(&kSignalWaitObjectInfo, NULL) == NULL) {
		return B_NO_MEMORY;
	}

	// process the events
	fBaseTime = -1;
	fLastEventTime = -1;
	uint32 count = 0;

	while (true) {
		// get next event
		uint32 event;
		uint32 cpu;
		const void* buffer;
		ssize_t bufferSize = input->ReadNextEvent(&event, &cpu, &buffer);
		if (bufferSize < 0)
			return bufferSize;
		if (buffer == NULL)
			return B_OK;

		// process the event
		status_t error = _ProcessEvent(event, cpu, buffer, bufferSize);
		if (error != B_OK)
			return error;

		// periodically check whether we're supposed to abort
		if (++count % 32 == 0) {
			AutoLocker<BLocker> locker(fLock);
			if (fAborted)
				return B_ERROR;
		}
	}
}


status_t
ModelLoader::_ReadDebugEvents(void** _eventData, size_t* _size)
{
	// get a BDataIO from the data source
	BDataIO* io;
	status_t error = fDataSource->CreateDataIO(&io);
	if (error != B_OK)
		return error;
	ObjectDeleter<BDataIO> dataIOtDeleter(io);

	// First we need to find out how large a buffer to allocate.
	size_t size;

	if (BPositionIO* positionIO = dynamic_cast<BPositionIO*>(io)) {
		// it's a BPositionIO -- this makes things easier, since we know how
		// many bytes to read
		off_t currentPos = positionIO->Position();
		if (currentPos < 0)
			return currentPos;

		off_t fileSize;
		error = positionIO->GetSize(&fileSize);
		if (error != B_OK)
			return error;

		size = fileSize - currentPos;
	} else {
		// no BPositionIO -- we need to determine the total size by iteratively
		// reading the whole data one time

		// allocate a dummy buffer for reading
		const size_t kBufferSize = 1024 * 1024;
		void* buffer = malloc(kBufferSize);
		if (buffer == NULL)
			return B_NO_MEMORY;
		MemoryDeleter bufferDeleter(buffer);

		size = 0;
		while (true) {
			ssize_t bytesRead = io->Read(buffer, kBufferSize);
			if (bytesRead < 0)
				return bytesRead;
			if (bytesRead == 0)
				break;

			size += bytesRead;
		}

		// we've got the size -- recreate the BDataIO
		dataIOtDeleter.Delete();
		error = fDataSource->CreateDataIO(&io);
		if (error != B_OK)
			return error;
		dataIOtDeleter.SetTo(io);
	}

	// allocate the data buffer
	void* data = malloc(size);
	if (data == NULL)
		return B_NO_MEMORY;
	MemoryDeleter dataDeleter(data);

	// read the data
	ssize_t bytesRead = io->Read(data, size);
	if (bytesRead < 0)
		return bytesRead;
	if ((size_t)bytesRead != size)
		return B_FILE_ERROR;

	dataDeleter.Detach();
	*_eventData = data;
	*_size = size;
	return B_OK;
}


status_t
ModelLoader::_ProcessEvent(uint32 event, uint32 cpu, const void* buffer,
	size_t size)
{
	switch (event) {
		case B_SYSTEM_PROFILER_TEAM_ADDED:
			_HandleTeamAdded((system_profiler_team_added*)buffer);
			break;

		case B_SYSTEM_PROFILER_TEAM_REMOVED:
			_HandleTeamRemoved((system_profiler_team_removed*)buffer);
			break;

		case B_SYSTEM_PROFILER_TEAM_EXEC:
			_HandleTeamExec((system_profiler_team_exec*)buffer);
			break;

		case B_SYSTEM_PROFILER_THREAD_ADDED:
			_HandleThreadAdded((system_profiler_thread_added*)buffer);
			break;

		case B_SYSTEM_PROFILER_THREAD_REMOVED:
			_HandleThreadRemoved((system_profiler_thread_removed*)buffer);
			break;

		case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
			_HandleThreadScheduled((system_profiler_thread_scheduled*)buffer);
			break;

		case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
			_HandleThreadEnqueuedInRunQueue(
				(thread_enqueued_in_run_queue*)buffer);
			break;

		case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
			_HandleThreadRemovedFromRunQueue(
				(thread_removed_from_run_queue*)buffer);
			break;

		case B_SYSTEM_PROFILER_WAIT_OBJECT_INFO:
			_HandleWaitObjectInfo((system_profiler_wait_object_info*)buffer);
			break;

		default:
printf("unsupported event type %lu, size: %lu\n", event, size);
return B_BAD_DATA;
			break;
	}

	return B_OK;
}


void
ModelLoader::_HandleTeamAdded(system_profiler_team_added* event)
{
	if (fModel->AddTeam(event, fLastEventTime) == NULL)
		throw std::bad_alloc();
}


void
ModelLoader::_HandleTeamRemoved(system_profiler_team_removed* event)
{
	if (Model::Team* team = fModel->TeamByID(event->team))
		team->SetDeletionTime(fLastEventTime);
	else
		printf("Removed event for unknown team: %ld\n", event->team);
}


void
ModelLoader::_HandleTeamExec(system_profiler_team_exec* event)
{
	// TODO:...
}


void
ModelLoader::_HandleThreadAdded(system_profiler_thread_added* event)
{
	if (_AddThread(event) == NULL)
		throw std::bad_alloc();
}


void
ModelLoader::_HandleThreadRemoved(system_profiler_thread_removed* event)
{
	if (Model::Thread* thread = fModel->ThreadByID(event->thread))
		thread->SetDeletionTime(fLastEventTime);
	else
		printf("Removed event for unknown team: %ld\n", event->thread);
}


void
ModelLoader::_HandleThreadScheduled(system_profiler_thread_scheduled* event)
{
	_UpdateLastEventTime(event->time);

	ThreadInfo* thread = fThreads.Lookup(event->thread);
	if (thread == NULL) {
		printf("Schedule event for unknown thread: %ld\n", event->thread);
		return;
	}

	bigtime_t diffTime = fLastEventTime - thread->lastTime;

	if (thread->state == READY) {
		// thread scheduled after having been woken up
		thread->thread->AddLatency(diffTime);
	} else if (thread->state == PREEMPTED) {
		// thread scheduled after having been preempted before
		thread->thread->AddRerun(diffTime);
	}

	if (thread->state == STILL_RUNNING) {
		// Thread was running and continues to run.
		thread->state = RUNNING;
	}

	if (thread->state != RUNNING) {
		thread->lastTime = fLastEventTime;
		thread->state = RUNNING;
	}

	// unscheduled thread

	if (event->thread == event->previous_thread)
		return;

	thread = fThreads.Lookup(event->previous_thread);
	if (thread == NULL) {
		printf("Schedule event for unknown previous thread: %ld\n",
			event->previous_thread);
		return;
	}

	diffTime = fLastEventTime - thread->lastTime;

	if (thread->state == STILL_RUNNING) {
		// thread preempted
		thread->thread->AddPreemption(diffTime);
		thread->thread->AddRun(diffTime);

		thread->lastTime = fLastEventTime;
		thread->state = PREEMPTED;
	} else if (thread->state == RUNNING) {
		// thread starts waiting (it hadn't been added to the run
		// queue before being unscheduled)
		thread->thread->AddRun(diffTime);

		if (event->previous_thread_state == B_THREAD_WAITING) {
			addr_t waitObject = event->previous_thread_wait_object;
			switch (event->previous_thread_wait_object_type) {
				case THREAD_BLOCK_TYPE_SNOOZE:
				case THREAD_BLOCK_TYPE_SIGNAL:
					waitObject = 0;
					break;
				case THREAD_BLOCK_TYPE_SEMAPHORE:
				case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
				case THREAD_BLOCK_TYPE_MUTEX:
				case THREAD_BLOCK_TYPE_RW_LOCK:
				case THREAD_BLOCK_TYPE_OTHER:
				default:
					break;
			}

			_AddThreadWaitObject(thread,
				event->previous_thread_wait_object_type, waitObject);
		}

		thread->lastTime = fLastEventTime;
		thread->state = WAITING;
	} else if (thread->state == UNKNOWN) {
		uint32 threadState = event->previous_thread_state;
		if (threadState == B_THREAD_WAITING
			|| threadState == B_THREAD_SUSPENDED) {
			thread->lastTime = fLastEventTime;
			thread->state = WAITING;
		} else if (threadState == B_THREAD_READY) {
			thread->lastTime = fLastEventTime;
			thread->state = PREEMPTED;
		}
	}
}


void
ModelLoader::_HandleThreadEnqueuedInRunQueue(
	thread_enqueued_in_run_queue* event)
{
	_UpdateLastEventTime(event->time);

	ThreadInfo* thread = fThreads.Lookup(event->thread);
	if (thread == NULL) {
		printf("Enqueued in run queue event for unknown thread: %ld\n",
			event->thread);
		return;
	}

	if (thread->state == RUNNING || thread->state == STILL_RUNNING) {
		// Thread was running and is reentered into the run queue. This
		// is done by the scheduler, if the thread remains ready.
		thread->state = STILL_RUNNING;
	} else {
		// Thread was waiting and is ready now.
		bigtime_t diffTime = fLastEventTime - thread->lastTime;
		if (thread->waitObject != NULL) {
			thread->waitObject->AddWait(diffTime);
			thread->waitObject = NULL;
			thread->thread->AddWait(diffTime);
		} else if (thread->state != UNKNOWN)
			thread->thread->AddUnspecifiedWait(diffTime);

		thread->lastTime = fLastEventTime;
		thread->state = READY;
	}
}


void
ModelLoader::_HandleThreadRemovedFromRunQueue(
	thread_removed_from_run_queue* event)
{
	_UpdateLastEventTime(event->time);

	ThreadInfo* thread = fThreads.Lookup(event->thread);
	if (thread == NULL) {
		printf("Removed from run queue event for unknown thread: %ld\n",
			event->thread);
		return;
	}

	// This really only happens when the thread priority is changed
	// while the thread is ready.

	bigtime_t diffTime = fLastEventTime - thread->lastTime;
	if (thread->state == RUNNING) {
		// This should never happen.
		thread->thread->AddRun(diffTime);
	} else if (thread->state == READY || thread->state == PREEMPTED) {
		// Not really correct, but the case is rare and we keep it
		// simple.
		thread->thread->AddUnspecifiedWait(diffTime);
	}

	thread->lastTime = fLastEventTime;
	thread->state = WAITING;
}


void
ModelLoader::_HandleWaitObjectInfo(system_profiler_wait_object_info* event)
{
	if (fModel->AddWaitObject(event, NULL) == NULL)
		throw std::bad_alloc();
}


ModelLoader::ThreadInfo*
ModelLoader::_AddThread(system_profiler_thread_added* event)
{
	// do we know the thread already?
	ThreadInfo* info = fThreads.Lookup(event->thread);
	if (info != NULL) {
		// TODO: ?
		return info;
	}

	// add the thread to the model
	Model::Thread* thread = fModel->AddThread(event, fLastEventTime);
	if (thread == NULL)
		return NULL;

	// create and add a ThreadInfo
	info = new(std::nothrow) ThreadInfo(thread);
	if (info == NULL)
		return NULL;

	fThreads.Insert(info);

	return info;
}


void
ModelLoader::_AddThreadWaitObject(ThreadInfo* thread, uint32 type,
	addr_t object)
{
	Model::WaitObjectGroup* waitObjectGroup
		= fModel->WaitObjectGroupFor(type, object);
	if (waitObjectGroup == NULL) {
		// The algorithm should prevent this case.
printf("ModelLoader::_AddThreadWaitObject(): Unknown wait object: type: %lu, "
"object: %#lx\n", type, object);
		return;
	}

	Model::WaitObject* waitObject = waitObjectGroup->MostRecentWaitObject();

	Model::ThreadWaitObjectGroup* threadWaitObjectGroup
		= fModel->ThreadWaitObjectGroupFor(thread->ID(), type, object);

	if (threadWaitObjectGroup == NULL
		|| threadWaitObjectGroup->MostRecentWaitObject() != waitObject) {
		Model::ThreadWaitObject* threadWaitObject
			= fModel->AddThreadWaitObject(thread->ID(), waitObject,
				&threadWaitObjectGroup);
		if (threadWaitObject == NULL)
			throw std::bad_alloc();
	}

	thread->waitObject = threadWaitObjectGroup->MostRecentThreadWaitObject();
}
