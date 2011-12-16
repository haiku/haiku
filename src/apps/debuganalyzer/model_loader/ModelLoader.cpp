/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ModelLoader.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <DebugEventStream.h>

#include <system_profiler_defs.h>
#include <thread_defs.h>

#include "DataSource.h"
#include "MessageCodes.h"
#include "Model.h"


// add a scheduling state snapshot every x events
static const uint32 kSchedulingSnapshotInterval = 1024;

static const uint32 kMaxCPUCount = 1024;


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


// #pragma mark - CPUInfo


struct ModelLoader::CPUInfo {
	nanotime_t	idleTime;

	CPUInfo()
		:
		idleTime(0)
	{
	}
};


// #pragma mark - IOOperation


struct ModelLoader::IOOperation : DoublyLinkedListLinkImpl<IOOperation> {
	io_operation_started*	startedEvent;
	io_operation_finished*	finishedEvent;

	IOOperation(io_operation_started* startedEvent)
		:
		startedEvent(startedEvent),
		finishedEvent(NULL)
	{
	}
};


// #pragma mark - IORequest


struct ModelLoader::IORequest : DoublyLinkedListLinkImpl<IORequest> {
	io_request_scheduled*	scheduledEvent;
	io_request_finished*	finishedEvent;
	IOOperationList			operations;
	size_t					operationCount;
	IORequest*				hashNext;

	IORequest(io_request_scheduled* scheduledEvent)
		:
		scheduledEvent(scheduledEvent),
		finishedEvent(NULL),
		operationCount(0)
	{
	}

	~IORequest()
	{
		while (IOOperation* operation = operations.RemoveHead())
			delete operation;
	}

	void AddOperation(IOOperation* operation)
	{
		operations.Add(operation);
		operationCount++;
	}

	IOOperation* FindOperation(void* address) const
	{
		for (IOOperationList::ConstReverseIterator it
				= operations.GetReverseIterator();
			IOOperation* operation = it.Next();) {
			if (operation->startedEvent->operation == address)
				return operation;
		}

		return NULL;
	}

	Model::IORequest* CreateModelRequest() const
	{
		size_t operationCount = operations.Count();

		Model::IORequest* modelRequest = Model::IORequest::Create(
			scheduledEvent, finishedEvent, operationCount);
		if (modelRequest == NULL)
			return NULL;

		size_t index = 0;
		for (IOOperationList::ConstIterator it = operations.GetIterator();
				IOOperation* operation = it.Next();) {
			Model::IOOperation& modelOperation
				= modelRequest->operations[index++];
			modelOperation.startedEvent = operation->startedEvent;
			modelOperation.finishedEvent = operation->finishedEvent;
		}

		return modelRequest;
	}
};


// #pragma mark - IORequestHashDefinition


struct ModelLoader::IORequestHashDefinition {
	typedef void*		KeyType;
	typedef	IORequest	ValueType;

	size_t HashKey(KeyType key) const
		{ return (size_t)key; }

	size_t Hash(const IORequest* value) const
		{ return HashKey(value->scheduledEvent->request); }

	bool Compare(KeyType key, const IORequest* value) const
		{ return key == value->scheduledEvent->request; }

	IORequest*& GetLink(IORequest* value) const
		{ return value->hashNext; }
};


// #pragma mark - ExtendedThreadSchedulingState


struct ModelLoader::ExtendedThreadSchedulingState
		: Model::ThreadSchedulingState {

	ExtendedThreadSchedulingState(Model::Thread* thread)
		:
		Model::ThreadSchedulingState(thread),
		fEvents(NULL),
		fEventIndex(0),
		fEventCount(0)
	{
	}

	~ExtendedThreadSchedulingState()
	{
		delete[] fEvents;

		while (IORequest* request = fIORequests.RemoveHead())
			delete request;
		while (IORequest* request = fPendingIORequests.RemoveHead())
			delete request;
	}

	system_profiler_event_header** Events() const
	{
		return fEvents;
	}

	size_t CountEvents() const
	{
		return fEventCount;
	}

	system_profiler_event_header** DetachEvents()
	{
		system_profiler_event_header** events = fEvents;
		fEvents = NULL;
		return events;
	}

	void IncrementEventCount()
	{
		fEventCount++;
	}

	void AddEvent(system_profiler_event_header* event)
	{
		fEvents[fEventIndex++] = event;
	}

	bool AllocateEventArray()
	{
		if (fEventCount == 0)
			return true;

		fEvents = new(std::nothrow) system_profiler_event_header*[fEventCount];
		if (fEvents == NULL)
			return false;

		return true;
	}

	void AddIORequest(IORequest* request)
	{
		fPendingIORequests.Add(request);
	}

	void IORequestFinished(IORequest* request)
	{
		fPendingIORequests.Remove(request);
		fIORequests.Add(request);
	}

	bool PrepareThreadIORequests(Model::IORequest**& _requests,
		size_t& _requestCount)
	{
		fIORequests.MoveFrom(&fPendingIORequests);
		size_t requestCount = fIORequests.Count();

		if (requestCount == 0) {
			_requests = NULL;
			_requestCount = 0;
			return true;
		}

		Model::IORequest** requests
			= new(std::nothrow) Model::IORequest*[requestCount];
		if (requests == NULL)
			return false;

		size_t index = 0;
		while (IORequest* request = fIORequests.RemoveHead()) {
			ObjectDeleter<IORequest> requestDeleter(request);

			Model::IORequest* modelRequest = request->CreateModelRequest();
			if (modelRequest == NULL) {
				for (size_t i = 0; i < index; i++)
					requests[i]->Delete();
				return false;
			}

			requests[index++] = modelRequest;
		}

		_requests = requests;
		_requestCount = requestCount;
		return true;
	}

private:
	system_profiler_event_header**	fEvents;
	size_t							fEventIndex;
	size_t							fEventCount;
	IORequestList					fIORequests;
	IORequestList					fPendingIORequests;
};


// #pragma mark - ExtendedSchedulingState


struct ModelLoader::ExtendedSchedulingState : Model::SchedulingState {
	inline ExtendedThreadSchedulingState* LookupThread(thread_id threadID) const
	{
		Model::ThreadSchedulingState* thread
			= Model::SchedulingState::LookupThread(threadID);
		return thread != NULL
			? static_cast<ExtendedThreadSchedulingState*>(thread) : NULL;
	}


protected:
	virtual void DeleteThread(Model::ThreadSchedulingState* thread)
	{
		delete static_cast<ExtendedThreadSchedulingState*>(thread);
	}
};


// #pragma mark - ModelLoader


inline void
ModelLoader::_UpdateLastEventTime(nanotime_t time)
{
	if (fBaseTime < 0) {
		fBaseTime = time;
		fModel->SetBaseTime(time);
	}

	fState->SetLastEventTime(time - fBaseTime);
}


ModelLoader::ModelLoader(DataSource* dataSource,
	const BMessenger& target, void* targetCookie)
	:
	AbstractModelLoader(target, targetCookie),
	fModel(NULL),
	fDataSource(dataSource),
	fCPUInfos(NULL),
	fState(NULL),
	fIORequests(NULL)
{
}


ModelLoader::~ModelLoader()
{
	delete[] fCPUInfos;
	delete fDataSource;
	delete fModel;
	delete fState;
	delete fIORequests;
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


status_t
ModelLoader::PrepareForLoading()
{
	if (fModel != NULL || fDataSource == NULL)
		return B_BAD_VALUE;

	// create and init the state
	fState = new(std::nothrow) ExtendedSchedulingState;
	if (fState == NULL)
		return B_NO_MEMORY;

	status_t error = fState->Init();
	if (error != B_OK)
		return error;

	// create CPU info array
	fCPUInfos = new(std::nothrow) CPUInfo[kMaxCPUCount];
	if (fCPUInfos == NULL)
		return B_NO_MEMORY;

	// create IORequest hash table
	fIORequests = new(std::nothrow) IORequestTable;
	if (fIORequests == NULL || fIORequests->Init() != B_OK)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
ModelLoader::Load()
{
	try {
		return _Load();
	} catch(...) {
		return B_ERROR;
	}
}


void
ModelLoader::FinishLoading(bool success)
{
	delete fState;
	fState = NULL;

	if (!success) {
		delete fModel;
		fModel = NULL;
	}

	delete[] fCPUInfos;
	fCPUInfos = NULL;

	delete fIORequests;
	fIORequests = NULL;
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
	MemoryDeleter eventDataDeleter(eventData);

	// create a debug event array
	system_profiler_event_header** events;
	size_t eventCount;
	error = _CreateDebugEventArray(eventData, eventDataSize, events,
		eventCount);
	if (error != B_OK)
		return error;
	ArrayDeleter<system_profiler_event_header*> eventsDeleter(events);

	// get the data source name
	BString dataSourceName;
	fDataSource->GetName(dataSourceName);

	// create a model
	fModel = new(std::nothrow) Model(dataSourceName.String(), eventData,
		eventDataSize, events, eventCount);
	if (fModel == NULL)
		return B_NO_MEMORY;
	eventDataDeleter.Detach();
	eventsDeleter.Detach();

	// create a debug input stream
	BDebugEventInputStream input;
	error = input.SetTo(eventData, eventDataSize, false);
	if (error != B_OK)
		return error;

	// add the snooze and signal wait objects to the model
	if (fModel->AddWaitObject(&kSnoozeWaitObjectInfo, NULL) == NULL
		|| fModel->AddWaitObject(&kSignalWaitObjectInfo, NULL) == NULL) {
		return B_NO_MEMORY;
	}

	// process the events
	fMaxCPUIndex = 0;
	fState->Clear();
	fBaseTime = -1;
	uint64 count = 0;

	while (true) {
		// get next event
		uint32 event;
		uint32 cpu;
		const void* buffer;
		off_t offset;
		ssize_t bufferSize = input.ReadNextEvent(&event, &cpu, &buffer,
			&offset);
		if (bufferSize < 0)
			return bufferSize;
		if (buffer == NULL)
			break;

		// process the event
		status_t error = _ProcessEvent(event, cpu, buffer, bufferSize);
		if (error != B_OK)
			return error;

		if (cpu > fMaxCPUIndex) {
			if (cpu + 1 > kMaxCPUCount)
				return B_BAD_DATA;
			fMaxCPUIndex = cpu;
		}

		// periodically check whether we're supposed to abort
		if (++count % 32 == 0) {
			AutoLocker<BLocker> locker(fLock);
			if (fAborted)
				return B_ERROR;
		}

		// periodically add scheduling snapshots
		if (count % kSchedulingSnapshotInterval == 0)
			fModel->AddSchedulingStateSnapshot(*fState, offset);
	}

	if (!fModel->SetCPUCount(fMaxCPUIndex + 1))
		return B_NO_MEMORY;

	for (uint32 i = 0; i <= fMaxCPUIndex; i++)
		fModel->CPUAt(i)->SetIdleTime(fCPUInfos[i].idleTime);

	fModel->SetLastEventTime(fState->LastEventTime());

	if (!_SetThreadEvents() || !_SetThreadIORequests())
		return B_NO_MEMORY;

	fModel->LoadingFinished();

	return B_OK;
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
ModelLoader::_CreateDebugEventArray(void* eventData, size_t eventDataSize,
	system_profiler_event_header**& _events, size_t& _eventCount)
{
	// count the events
	BDebugEventInputStream input;
	status_t error = input.SetTo(eventData, eventDataSize, false);
	if (error != B_OK)
		return error;

	size_t eventCount = 0;
	while (true) {
		// get next event
		uint32 event;
		uint32 cpu;
		const void* buffer;
		ssize_t bufferSize = input.ReadNextEvent(&event, &cpu, &buffer, NULL);
		if (bufferSize < 0)
			return bufferSize;
		if (buffer == NULL)
			break;

		eventCount++;
	}

	// create the array
	system_profiler_event_header** events = new(std::nothrow)
		system_profiler_event_header*[eventCount];
	if (events == NULL)
		return B_NO_MEMORY;

	// populate the array
	error = input.SetTo(eventData, eventDataSize, false);
	if (error != B_OK) {
		delete[] events;
		return error;
	}

	size_t eventIndex = 0;
	while (true) {
		// get next event
		uint32 event;
		uint32 cpu;
		const void* buffer;
		off_t offset;
		input.ReadNextEvent(&event, &cpu, &buffer, &offset);
		if (buffer == NULL)
			break;

		events[eventIndex++]
			= (system_profiler_event_header*)((uint8*)eventData + offset);
	}

	_events = events;
	_eventCount = eventCount;
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
			_HandleThreadScheduled(cpu,
				(system_profiler_thread_scheduled*)buffer);
			break;

		case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
			_HandleThreadEnqueuedInRunQueue(
				(thread_enqueued_in_run_queue*)buffer);
			break;

		case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
			_HandleThreadRemovedFromRunQueue(cpu,
				(thread_removed_from_run_queue*)buffer);
			break;

		case B_SYSTEM_PROFILER_WAIT_OBJECT_INFO:
			_HandleWaitObjectInfo((system_profiler_wait_object_info*)buffer);
			break;

		case B_SYSTEM_PROFILER_IO_SCHEDULER_ADDED:
			_HandleIOSchedulerAdded(
				(system_profiler_io_scheduler_added*)buffer);
			break;

		case B_SYSTEM_PROFILER_IO_SCHEDULER_REMOVED:
			// not so interesting
			break;

		case B_SYSTEM_PROFILER_IO_REQUEST_SCHEDULED:
			_HandleIORequestScheduled((io_request_scheduled*)buffer);
			break;
		case B_SYSTEM_PROFILER_IO_REQUEST_FINISHED:
			_HandleIORequestFinished((io_request_finished*)buffer);
			break;
		case B_SYSTEM_PROFILER_IO_OPERATION_STARTED:
			_HandleIOOperationStarted((io_operation_started*)buffer);
			break;
		case B_SYSTEM_PROFILER_IO_OPERATION_FINISHED:
			_HandleIOOperationFinished((io_operation_finished*)buffer);
			break;

		default:
printf("unsupported event type %lu, size: %lu\n", event, size);
return B_BAD_DATA;
			break;
	}

	return B_OK;
}


bool
ModelLoader::_SetThreadEvents()
{
	// allocate the threads' events arrays
	for (int32 i = 0; Model::Thread* thread = fModel->ThreadAt(i); i++) {
		ExtendedThreadSchedulingState* state
			= fState->LookupThread(thread->ID());
		if (!state->AllocateEventArray())
			return false;
	}

	// fill the threads' event arrays
	system_profiler_event_header** events = fModel->Events();
	size_t eventCount = fModel->CountEvents();
	for (size_t i = 0; i < eventCount; i++) {
		system_profiler_event_header* header = events[i];
		void* buffer = header + 1;

		switch (header->event) {
			case B_SYSTEM_PROFILER_THREAD_ADDED:
			{
				system_profiler_thread_added* event
					= (system_profiler_thread_added*)buffer;
				fState->LookupThread(event->thread)->AddEvent(header);
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_REMOVED:
			{
				system_profiler_thread_removed* event
					= (system_profiler_thread_removed*)buffer;
				fState->LookupThread(event->thread)->AddEvent(header);
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
			{
				system_profiler_thread_scheduled* event
					= (system_profiler_thread_scheduled*)buffer;
				fState->LookupThread(event->thread)->AddEvent(header);

				if (event->thread != event->previous_thread) {
					fState->LookupThread(event->previous_thread)
						->AddEvent(header);
				}
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
			{
				thread_enqueued_in_run_queue* event
					= (thread_enqueued_in_run_queue*)buffer;
				fState->LookupThread(event->thread)->AddEvent(header);
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
			{
				thread_removed_from_run_queue* event
					= (thread_removed_from_run_queue*)buffer;
				fState->LookupThread(event->thread)->AddEvent(header);
				break;
			}

			default:
				break;
		}
	}

	// transfer the events arrays from the scheduling states to the thread
	// objects
	for (int32 i = 0; Model::Thread* thread = fModel->ThreadAt(i); i++) {
		ExtendedThreadSchedulingState* state
			= fState->LookupThread(thread->ID());
		thread->SetEvents(state->Events(), state->CountEvents());
		state->DetachEvents();
	}

	return true;
}


bool
ModelLoader::_SetThreadIORequests()
{
	for (int32 i = 0; Model::Thread* thread = fModel->ThreadAt(i); i++) {
		ExtendedThreadSchedulingState* state
			= fState->LookupThread(thread->ID());
		Model::IORequest** requests;
		size_t requestCount;
		if (!state->PrepareThreadIORequests(requests, requestCount))
			return false;
		if (requestCount > 0)
			_SetThreadIORequests(thread, requests, requestCount);
	}

	return true;
}


void
ModelLoader::_SetThreadIORequests(Model::Thread* thread,
	Model::IORequest** requests, size_t requestCount)
{
	// compute some totals
	int64 ioCount = 0;
	nanotime_t ioTime = 0;

	// sort requests by scheduler and start time
	std::sort(requests, requests + requestCount,
		Model::IORequest::SchedulerTimeLess);

	nanotime_t endTime = fBaseTime + fModel->LastEventTime();

	// compute the summed up I/O times
	nanotime_t ioStart = requests[0]->scheduledEvent->time;
	nanotime_t previousEnd = requests[0]->finishedEvent != NULL
		? requests[0]->finishedEvent->time : endTime;
	int32 scheduler = requests[0]->scheduledEvent->scheduler;

	for (size_t i = 1; i < requestCount; i++) {
		system_profiler_io_request_scheduled* scheduledEvent
			= requests[i]->scheduledEvent;
		if (scheduledEvent->scheduler != scheduler
			|| scheduledEvent->time >= previousEnd) {
			ioCount++;
			ioTime += previousEnd - ioStart;
			ioStart = scheduledEvent->time;
		}

		previousEnd = requests[i]->finishedEvent != NULL
			? requests[i]->finishedEvent->time : endTime;
	}

	ioCount++;
	ioTime += previousEnd - ioStart;

	// sort requests by start time
	std::sort(requests, requests + requestCount, Model::IORequest::TimeLess);

	// set the computed values
	thread->SetIORequests(requests, requestCount);
	thread->SetIOs(ioCount, ioTime);
}


void
ModelLoader::_HandleTeamAdded(system_profiler_team_added* event)
{
	if (fModel->AddTeam(event, fState->LastEventTime()) == NULL)
		throw std::bad_alloc();
}


void
ModelLoader::_HandleTeamRemoved(system_profiler_team_removed* event)
{
	if (Model::Team* team = fModel->TeamByID(event->team))
		team->SetDeletionTime(fState->LastEventTime());
	else
		printf("Warning: Removed event for unknown team: %ld\n", event->team);
}


void
ModelLoader::_HandleTeamExec(system_profiler_team_exec* event)
{
	// TODO:...
}


void
ModelLoader::_HandleThreadAdded(system_profiler_thread_added* event)
{
	_AddThread(event)->IncrementEventCount();
}


void
ModelLoader::_HandleThreadRemoved(system_profiler_thread_removed* event)
{
	ExtendedThreadSchedulingState* thread = fState->LookupThread(event->thread);
	if (thread == NULL) {
		printf("Warning: Removed event for unknown thread: %ld\n",
			event->thread);
		thread = _AddUnknownThread(event->thread);
	}

	thread->thread->SetDeletionTime(fState->LastEventTime());
	thread->IncrementEventCount();
}


void
ModelLoader::_HandleThreadScheduled(uint32 cpu,
	system_profiler_thread_scheduled* event)
{
	_UpdateLastEventTime(event->time);

	ExtendedThreadSchedulingState* thread = fState->LookupThread(event->thread);
	if (thread == NULL) {
		printf("Warning: Schedule event for unknown thread: %ld\n",
			event->thread);
		thread = _AddUnknownThread(event->thread);
		return;
	}

	nanotime_t diffTime = fState->LastEventTime() - thread->lastTime;

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
		thread->lastTime = fState->LastEventTime();
		thread->state = RUNNING;
	}

	thread->IncrementEventCount();

	// unscheduled thread

	if (event->thread == event->previous_thread)
		return;

	thread = fState->LookupThread(event->previous_thread);
	if (thread == NULL) {
		printf("Warning: Schedule event for unknown previous thread: %ld\n",
			event->previous_thread);
		thread = _AddUnknownThread(event->previous_thread);
	}

	diffTime = fState->LastEventTime() - thread->lastTime;

	if (thread->state == STILL_RUNNING) {
		// thread preempted
		thread->thread->AddPreemption(diffTime);
		thread->thread->AddRun(diffTime);
		if (thread->priority == 0)
			_AddIdleTime(cpu, diffTime);

		thread->lastTime = fState->LastEventTime();
		thread->state = PREEMPTED;
	} else if (thread->state == RUNNING) {
		// thread starts waiting (it hadn't been added to the run
		// queue before being unscheduled)
		thread->thread->AddRun(diffTime);
		if (thread->priority == 0)
			_AddIdleTime(cpu, diffTime);

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

		thread->lastTime = fState->LastEventTime();
		thread->state = WAITING;
	} else if (thread->state == UNKNOWN) {
		uint32 threadState = event->previous_thread_state;
		if (threadState == B_THREAD_WAITING
			|| threadState == B_THREAD_SUSPENDED) {
			thread->lastTime = fState->LastEventTime();
			thread->state = WAITING;
		} else if (threadState == B_THREAD_READY) {
			thread->lastTime = fState->LastEventTime();
			thread->state = PREEMPTED;
		}
	}

	thread->IncrementEventCount();
}


void
ModelLoader::_HandleThreadEnqueuedInRunQueue(
	thread_enqueued_in_run_queue* event)
{
	_UpdateLastEventTime(event->time);

	ExtendedThreadSchedulingState* thread = fState->LookupThread(event->thread);
	if (thread == NULL) {
		printf("Warning: Enqueued in run queue event for unknown thread: %ld\n",
			event->thread);
		thread = _AddUnknownThread(event->thread);
	}

	if (thread->state == RUNNING || thread->state == STILL_RUNNING) {
		// Thread was running and is reentered into the run queue. This
		// is done by the scheduler, if the thread remains ready.
		thread->state = STILL_RUNNING;
	} else {
		// Thread was waiting and is ready now.
		nanotime_t diffTime = fState->LastEventTime() - thread->lastTime;
		if (thread->waitObject != NULL) {
			thread->waitObject->AddWait(diffTime);
			thread->waitObject = NULL;
			thread->thread->AddWait(diffTime);
		} else if (thread->state != UNKNOWN)
			thread->thread->AddUnspecifiedWait(diffTime);

		thread->lastTime = fState->LastEventTime();
		thread->state = READY;
	}

	thread->priority = event->priority;

	thread->IncrementEventCount();
}


void
ModelLoader::_HandleThreadRemovedFromRunQueue(uint32 cpu,
	thread_removed_from_run_queue* event)
{
	_UpdateLastEventTime(event->time);

	ExtendedThreadSchedulingState* thread = fState->LookupThread(event->thread);
	if (thread == NULL) {
		printf("Warning: Removed from run queue event for unknown thread: "
			"%ld\n", event->thread);
		thread = _AddUnknownThread(event->thread);
	}

	// This really only happens when the thread priority is changed
	// while the thread is ready.

	nanotime_t diffTime = fState->LastEventTime() - thread->lastTime;
	if (thread->state == RUNNING) {
		// This should never happen.
		thread->thread->AddRun(diffTime);
		if (thread->priority == 0)
			_AddIdleTime(cpu, diffTime);
	} else if (thread->state == READY || thread->state == PREEMPTED) {
		// Not really correct, but the case is rare and we keep it
		// simple.
		thread->thread->AddUnspecifiedWait(diffTime);
	}

	thread->lastTime = fState->LastEventTime();
	thread->state = WAITING;

	thread->IncrementEventCount();
}


void
ModelLoader::_HandleWaitObjectInfo(system_profiler_wait_object_info* event)
{
	if (fModel->AddWaitObject(event, NULL) == NULL)
		throw std::bad_alloc();
}


void
ModelLoader::_HandleIOSchedulerAdded(system_profiler_io_scheduler_added* event)
{
	Model::IOScheduler* scheduler = fModel->IOSchedulerByID(event->scheduler);
	if (scheduler != NULL) {
		printf("Warning: Duplicate added event for I/O scheduler %ld\n",
			event->scheduler);
		return;
	}

	if (fModel->AddIOScheduler(event) == NULL)
		throw std::bad_alloc();
}


void
ModelLoader::_HandleIORequestScheduled(io_request_scheduled* event)
{
	IORequest* request = fIORequests->Lookup(event->request);
	if (request != NULL) {
		printf("Warning: Duplicate schedule event for I/O request %p\n",
			event->request);
		return;
	}

	ExtendedThreadSchedulingState* thread = fState->LookupThread(event->thread);
	if (thread == NULL) {
		printf("Warning: I/O request for unknown thread %ld\n", event->thread);
		thread = _AddUnknownThread(event->thread);
	}

	if (fModel->IOSchedulerByID(event->scheduler) == NULL) {
		printf("Warning: I/O requests for unknown scheduler %ld\n",
			event->scheduler);
		// TODO: Add state for unknown scheduler, as we do for threads.
		return;
	}

	request = new(std::nothrow) IORequest(event);
	if (request == NULL)
		throw std::bad_alloc();

	fIORequests->Insert(request);
	thread->AddIORequest(request);
}


void
ModelLoader::_HandleIORequestFinished(io_request_finished* event)
{
	IORequest* request = fIORequests->Lookup(event->request);
	if (request == NULL)
		return;

	request->finishedEvent = event;

	fIORequests->Remove(request);
	fState->LookupThread(request->scheduledEvent->thread)
		->IORequestFinished(request);
}


void
ModelLoader::_HandleIOOperationStarted(io_operation_started* event)
{
	IORequest* request = fIORequests->Lookup(event->request);
	if (request == NULL) {
		printf("Warning: I/O request for operation %p not found\n",
			event->operation);
		return;
	}

	IOOperation* operation = new(std::nothrow) IOOperation(event);
	if (operation == NULL)
		throw std::bad_alloc();

	request->AddOperation(operation);
}


void
ModelLoader::_HandleIOOperationFinished(io_operation_finished* event)
{
	IORequest* request = fIORequests->Lookup(event->request);
	if (request == NULL) {
		printf("Warning: I/O request for operation %p not found\n",
			event->operation);
		return;
	}

	IOOperation* operation = request->FindOperation(event->operation);
	if (operation == NULL) {
		printf("Warning: operation %p not found\n", event->operation);
		return;
	}

	operation->finishedEvent = event;
}


ModelLoader::ExtendedThreadSchedulingState*
ModelLoader::_AddThread(system_profiler_thread_added* event)
{
	// do we know the thread already?
	ExtendedThreadSchedulingState* info = fState->LookupThread(event->thread);
	if (info != NULL) {
		printf("Warning: Duplicate thread added event for thread %" B_PRId32
			"\n", event->thread);
		return info;
	}

	// add the thread to the model
	Model::Thread* thread = fModel->AddThread(event, fState->LastEventTime());
	if (thread == NULL)
		throw std::bad_alloc();

	// create and add a ThreadSchedulingState
	info = new(std::nothrow) ExtendedThreadSchedulingState(thread);
	if (info == NULL)
		throw std::bad_alloc();

	// TODO: The priority is missing from the system_profiler_thread_added
	// struct. For now guess at least whether this is an idle thread.
	if (strncmp(event->name, "idle thread", strlen("idle thread")) == 0)
		info->priority = 0;
	else
		info->priority = B_NORMAL_PRIORITY;

	fState->InsertThread(info);

	return info;
}


ModelLoader::ExtendedThreadSchedulingState*
ModelLoader::_AddUnknownThread(thread_id threadID)
{
	// create a dummy "add thread" event
	system_profiler_thread_added* event = (system_profiler_thread_added*)
		malloc(sizeof(system_profiler_thread_added));
	if (event == NULL)
		throw std::bad_alloc();

	if (!fModel->AddAssociatedData(event)) {
		free(event);
		throw std::bad_alloc();
	}

	try {
		event->team = _AddUnknownTeam()->ID();
		event->thread = threadID;
		snprintf(event->name, sizeof(event->name), "unknown thread %" B_PRId32,
			threadID);

		// add the thread to the model
		ExtendedThreadSchedulingState* state = _AddThread(event);
		return state;
	} catch (...) {
		throw;
	}
}

Model::Team*
ModelLoader::_AddUnknownTeam()
{
	team_id teamID = 0;
	Model::Team* team = fModel->TeamByID(teamID);
	if (team != NULL)
		return team;

	// create a dummy "add team" event
	static const char* const kUnknownThreadsTeamName = "unknown threads";
	size_t nameLength = strlen(kUnknownThreadsTeamName);

	system_profiler_team_added* event = (system_profiler_team_added*)
		malloc(sizeof(system_profiler_team_added) + nameLength);
	if (event == NULL)
		throw std::bad_alloc();

	event->team = teamID;
	event->args_offset = nameLength;
	strlcpy(event->name, kUnknownThreadsTeamName, nameLength + 1);

	// add the team to the model
	team = fModel->AddTeam(event, fState->LastEventTime());
	if (team == NULL)
		throw std::bad_alloc();

	return team;
}


void
ModelLoader::_AddThreadWaitObject(ExtendedThreadSchedulingState* thread,
	uint32 type, addr_t object)
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


void
ModelLoader::_AddIdleTime(uint32 cpu, nanotime_t time)
{
	fCPUInfos[cpu].idleTime += time;
}
