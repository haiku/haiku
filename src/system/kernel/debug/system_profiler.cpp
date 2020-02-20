/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <system_profiler.h>

#include <AutoDeleter.h>
#include <Referenceable.h>

#include <util/AutoLock.h>

#include <system_profiler_defs.h>

#include <cpu.h>
#include <kernel.h>
#include <kimage.h>
#include <kscheduler.h>
#include <listeners.h>
#include <Notifications.h>
#include <sem.h>
#include <team.h>
#include <thread.h>
#include <user_debugger.h>
#include <vm/vm.h>

#include <arch/debug.h>

#include "IOSchedulerRoster.h"


// This is the kernel-side implementation of the system profiling support.
// A userland team can register as system profiler, providing an area as buffer
// for events. Those events are team, thread, and image changes (added/removed),
// periodic sampling of the return address stack for each CPU, as well as
// scheduling and I/O scheduling events.


class SystemProfiler;


// minimum/maximum size of the table used for wait object caching
#define MIN_WAIT_OBJECT_COUNT	128
#define MAX_WAIT_OBJECT_COUNT	1024


static spinlock sProfilerLock = B_SPINLOCK_INITIALIZER;
static SystemProfiler* sProfiler = NULL;
static struct system_profiler_parameters* sRecordedParameters = NULL;


class SystemProfiler : public BReferenceable, private NotificationListener,
	private SchedulerListener, private WaitObjectListener {
public:
								SystemProfiler(team_id team,
									const area_info& userAreaInfo,
									const system_profiler_parameters&
										parameters);
								~SystemProfiler();

			team_id				TeamID() const	{ return fTeam; }

			status_t			Init();
			status_t			NextBuffer(size_t bytesRead,
									uint64* _droppedEvents);

private:
	virtual	void				EventOccurred(NotificationService& service,
									const KMessage* event);

	virtual	void				ThreadEnqueuedInRunQueue(Thread* thread);
	virtual	void				ThreadRemovedFromRunQueue(Thread* thread);
	virtual	void				ThreadScheduled(Thread* oldThread,
									Thread* newThread);

	virtual	void				SemaphoreCreated(sem_id id,
									const char* name);
	virtual	void				ConditionVariableInitialized(
									ConditionVariable* variable);
	virtual	void				MutexInitialized(mutex* lock);
	virtual	void				RWLockInitialized(rw_lock* lock);

			bool				_TeamAdded(Team* team);
			bool				_TeamRemoved(Team* team);
			bool				_TeamExec(Team* team);

			bool				_ThreadAdded(Thread* thread);
			bool				_ThreadRemoved(Thread* thread);

			bool				_ImageAdded(struct image* image);
			bool				_ImageRemoved(struct image* image);

			bool				_IOSchedulerAdded(IOScheduler* scheduler);
			bool				_IOSchedulerRemoved(IOScheduler* scheduler);
			bool				_IORequestScheduled(IOScheduler* scheduler,
									IORequest* request);
			bool				_IORequestFinished(IOScheduler* scheduler,
									IORequest* request);
			bool				_IOOperationStarted(IOScheduler* scheduler,
									IORequest* request, IOOperation* operation);
			bool				_IOOperationFinished(IOScheduler* scheduler,
									IORequest* request, IOOperation* operation);

			void				_WaitObjectCreated(addr_t object, uint32 type);
			void				_WaitObjectUsed(addr_t object, uint32 type);

	inline	void				_MaybeNotifyProfilerThreadLocked();
	inline	void				_MaybeNotifyProfilerThread();

	static	bool				_InitialImageIterator(struct image* image,
									void* cookie);

			void*				_AllocateBuffer(size_t size, int event, int cpu,
									int count);

	static	void				_InitTimers(void* cookie, int cpu);
	static	void				_UninitTimers(void* cookie, int cpu);
			void				_ScheduleTimer(int cpu);

			void				_DoSample();

	static	int32				_ProfilingEvent(struct timer* timer);

private:
			struct CPUProfileData {
				struct timer	timer;
				bigtime_t		timerEnd;
				bool			timerScheduled;
				addr_t			buffer[B_DEBUG_STACK_TRACE_DEPTH];
			};

			struct WaitObjectKey {
				addr_t	object;
				uint32	type;
			};

			struct WaitObject : DoublyLinkedListLinkImpl<WaitObject>,
					WaitObjectKey {
				struct WaitObject* hash_link;
			};

			struct WaitObjectTableDefinition {
				typedef WaitObjectKey	KeyType;
				typedef	WaitObject		ValueType;

				size_t HashKey(const WaitObjectKey& key) const
				{
					return (size_t)key.object ^ (size_t)key.type;
				}

				size_t Hash(const WaitObject* value) const
				{
					return HashKey(*value);
				}

				bool Compare(const WaitObjectKey& key,
					const WaitObject* value) const
				{
					return value->type == key.type
						&& value->object == key.object;
				}

				WaitObject*& GetLink(WaitObject* value) const
				{
					return value->hash_link;
				}
			};

			typedef DoublyLinkedList<WaitObject> WaitObjectList;
			typedef BOpenHashTable<WaitObjectTableDefinition> WaitObjectTable;

private:
			spinlock			fLock;
			team_id				fTeam;
			area_id				fUserArea;
			area_id				fKernelArea;
			size_t				fAreaSize;
			uint32				fFlags;
			uint32				fStackDepth;
			bigtime_t			fInterval;
			system_profiler_buffer_header* fHeader;
			uint8*				fBufferBase;
			size_t				fBufferCapacity;
			size_t				fBufferStart;
			size_t				fBufferSize;
			uint64				fDroppedEvents;
			int64				fLastTeamAddedSerialNumber;
			int64				fLastThreadAddedSerialNumber;
			bool				fTeamNotificationsRequested;
			bool				fTeamNotificationsEnabled;
			bool				fThreadNotificationsRequested;
			bool				fThreadNotificationsEnabled;
			bool				fImageNotificationsRequested;
			bool				fImageNotificationsEnabled;
			bool				fIONotificationsRequested;
			bool				fIONotificationsEnabled;
			bool				fSchedulerNotificationsRequested;
			bool				fWaitObjectNotificationsRequested;
			Thread* volatile	fWaitingProfilerThread;
			bool				fProfilingActive;
			bool				fReentered[SMP_MAX_CPUS];
			CPUProfileData		fCPUData[SMP_MAX_CPUS];
			WaitObject*			fWaitObjectBuffer;
			int32				fWaitObjectCount;
			WaitObjectList		fUsedWaitObjects;
			WaitObjectList		fFreeWaitObjects;
			WaitObjectTable		fWaitObjectTable;
};


/*!	Notifies the profiler thread when the profiling buffer is full enough.
	The caller must hold fLock.
*/
inline void
SystemProfiler::_MaybeNotifyProfilerThreadLocked()
{
	// If the buffer is full enough, notify the profiler.
	if (fWaitingProfilerThread != NULL && fBufferSize > fBufferCapacity / 2) {
		int cpu = smp_get_current_cpu();
		fReentered[cpu] = true;

		Thread* profilerThread = fWaitingProfilerThread;
		fWaitingProfilerThread = NULL;

		SpinLocker _(profilerThread->scheduler_lock);
		thread_unblock_locked(profilerThread, B_OK);

		fReentered[cpu] = false;
	}
}


inline void
SystemProfiler::_MaybeNotifyProfilerThread()
{
	if (fWaitingProfilerThread == NULL)
		return;

	InterruptsSpinLocker locker(fLock);

	_MaybeNotifyProfilerThreadLocked();
}


// #pragma mark - SystemProfiler public


SystemProfiler::SystemProfiler(team_id team, const area_info& userAreaInfo,
	const system_profiler_parameters& parameters)
	:
	fTeam(team),
	fUserArea(userAreaInfo.area),
	fKernelArea(-1),
	fAreaSize(userAreaInfo.size),
	fFlags(parameters.flags),
	fStackDepth(parameters.stack_depth),
	fInterval(parameters.interval),
	fHeader(NULL),
	fBufferBase(NULL),
	fBufferCapacity(0),
	fBufferStart(0),
	fBufferSize(0),
	fDroppedEvents(0),
	fLastTeamAddedSerialNumber(0),
	fLastThreadAddedSerialNumber(0),
	fTeamNotificationsRequested(false),
	fTeamNotificationsEnabled(false),
	fThreadNotificationsRequested(false),
	fThreadNotificationsEnabled(false),
	fImageNotificationsRequested(false),
	fImageNotificationsEnabled(false),
	fIONotificationsRequested(false),
	fIONotificationsEnabled(false),
	fSchedulerNotificationsRequested(false),
	fWaitObjectNotificationsRequested(false),
	fWaitingProfilerThread(NULL),
	fWaitObjectBuffer(NULL),
	fWaitObjectCount(0),
	fUsedWaitObjects(),
	fFreeWaitObjects(),
	fWaitObjectTable()
{
	B_INITIALIZE_SPINLOCK(&fLock);

	memset(fReentered, 0, sizeof(fReentered));

	// compute the number wait objects we want to cache
	if ((fFlags & B_SYSTEM_PROFILER_SCHEDULING_EVENTS) != 0) {
		fWaitObjectCount = parameters.locking_lookup_size
			/ (sizeof(WaitObject) + (sizeof(void*) * 3 / 2));
		if (fWaitObjectCount < MIN_WAIT_OBJECT_COUNT)
			fWaitObjectCount = MIN_WAIT_OBJECT_COUNT;
		if (fWaitObjectCount > MAX_WAIT_OBJECT_COUNT)
			fWaitObjectCount = MAX_WAIT_OBJECT_COUNT;
	}
}


SystemProfiler::~SystemProfiler()
{
	// Wake up the user thread, if it is waiting, and mark profiling
	// inactive.
	InterruptsSpinLocker locker(fLock);
	if (fWaitingProfilerThread != NULL) {
		thread_unblock(fWaitingProfilerThread, B_OK);
		fWaitingProfilerThread = NULL;
	}
	fProfilingActive = false;
	locker.Unlock();

	// stop scheduler listening
	if (fSchedulerNotificationsRequested)
		scheduler_remove_listener(this);

	// stop wait object listening
	if (fWaitObjectNotificationsRequested) {
		InterruptsSpinLocker locker(gWaitObjectListenerLock);
		remove_wait_object_listener(this);
	}

	// deactivate the profiling timers on all CPUs
	if ((fFlags & B_SYSTEM_PROFILER_SAMPLING_EVENTS) != 0)
		call_all_cpus(_UninitTimers, this);

	// cancel notifications
	NotificationManager& notificationManager
		= NotificationManager::Manager();

	// images
	if (fImageNotificationsRequested) {
		fImageNotificationsRequested = false;
		notificationManager.RemoveListener("images", NULL, *this);
	}

	// threads
	if (fThreadNotificationsRequested) {
		fThreadNotificationsRequested = false;
		notificationManager.RemoveListener("threads", NULL, *this);
	}

	// teams
	if (fTeamNotificationsRequested) {
		fTeamNotificationsRequested = false;
		notificationManager.RemoveListener("teams", NULL, *this);
	}

	// I/O
	if (fIONotificationsRequested) {
		fIONotificationsRequested = false;
		notificationManager.RemoveListener("I/O", NULL, *this);
	}

	// delete wait object related allocations
	fWaitObjectTable.Clear();
	delete[] fWaitObjectBuffer;

	// unlock the memory and delete the area
	if (fKernelArea >= 0) {
		unlock_memory(fHeader, fAreaSize, B_READ_DEVICE);
		delete_area(fKernelArea);
		fKernelArea = -1;
	}
}


status_t
SystemProfiler::Init()
{
	// clone the user area
	void* areaBase;
	fKernelArea = clone_area("profiling samples", &areaBase,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		fUserArea);
	if (fKernelArea < 0)
		return fKernelArea;

	// we need the memory locked
	status_t error = lock_memory(areaBase, fAreaSize, B_READ_DEVICE);
	if (error != B_OK) {
		delete_area(fKernelArea);
		fKernelArea = -1;
		return error;
	}

	// the buffer is ready for use
	fHeader = (system_profiler_buffer_header*)areaBase;
	fBufferBase = (uint8*)(fHeader + 1);
	fBufferCapacity = fAreaSize - (fBufferBase - (uint8*)areaBase);
	fHeader->start = 0;
	fHeader->size = 0;

	// allocate the wait object buffer and init the hash table
	if (fWaitObjectCount > 0) {
		fWaitObjectBuffer = new(std::nothrow) WaitObject[fWaitObjectCount];
		if (fWaitObjectBuffer == NULL)
			return B_NO_MEMORY;

		for (int32 i = 0; i < fWaitObjectCount; i++)
			fFreeWaitObjects.Add(fWaitObjectBuffer + i);

		error = fWaitObjectTable.Init(fWaitObjectCount * 3 / 2);
		if (error != B_OK)
			return error;
	}

	// start listening for notifications

	// teams
	NotificationManager& notificationManager
		= NotificationManager::Manager();
	if ((fFlags & B_SYSTEM_PROFILER_TEAM_EVENTS) != 0) {
		error = notificationManager.AddListener("teams",
			TEAM_ADDED | TEAM_REMOVED | TEAM_EXEC, *this);
		if (error != B_OK)
			return error;
		fTeamNotificationsRequested = true;
	}

	// threads
	if ((fFlags & B_SYSTEM_PROFILER_THREAD_EVENTS) != 0) {
		error = notificationManager.AddListener("threads",
			THREAD_ADDED | THREAD_REMOVED, *this);
		if (error != B_OK)
			return error;
		fThreadNotificationsRequested = true;
	}

	// images
	if ((fFlags & B_SYSTEM_PROFILER_IMAGE_EVENTS) != 0) {
		error = notificationManager.AddListener("images",
			IMAGE_ADDED | IMAGE_REMOVED, *this);
		if (error != B_OK)
			return error;
		fImageNotificationsRequested = true;
	}

	// I/O events
	if ((fFlags & B_SYSTEM_PROFILER_IO_SCHEDULING_EVENTS) != 0) {
		error = notificationManager.AddListener("I/O",
			IO_SCHEDULER_ADDED | IO_SCHEDULER_REMOVED
				| IO_SCHEDULER_REQUEST_SCHEDULED | IO_SCHEDULER_REQUEST_FINISHED
				| IO_SCHEDULER_OPERATION_STARTED
				| IO_SCHEDULER_OPERATION_FINISHED,
			*this);
		if (error != B_OK)
			return error;
		fIONotificationsRequested = true;
	}

	// We need to fill the buffer with the initial state of teams, threads,
	// and images.

	// teams
	if ((fFlags & B_SYSTEM_PROFILER_TEAM_EVENTS) != 0) {
		InterruptsSpinLocker locker(fLock);

		TeamListIterator iterator;
		while (Team* team = iterator.Next()) {
			locker.Unlock();

			bool added = _TeamAdded(team);

			// release the reference returned by the iterator
			team->ReleaseReference();

			if (!added)
				return B_BUFFER_OVERFLOW;

			locker.Lock();
		}

		fTeamNotificationsEnabled = true;
	}

	// images
	if ((fFlags & B_SYSTEM_PROFILER_IMAGE_EVENTS) != 0) {
		if (image_iterate_through_images(&_InitialImageIterator, this) != NULL)
			return B_BUFFER_OVERFLOW;
	}

	// threads
	if ((fFlags & B_SYSTEM_PROFILER_THREAD_EVENTS) != 0) {
		InterruptsSpinLocker locker(fLock);

		ThreadListIterator iterator;
		while (Thread* thread = iterator.Next()) {
			locker.Unlock();

			bool added = _ThreadAdded(thread);

			// release the reference returned by the iterator
			thread->ReleaseReference();

			if (!added)
				return B_BUFFER_OVERFLOW;

			locker.Lock();
		}

		fThreadNotificationsEnabled = true;
	}

	fProfilingActive = true;

	// start scheduler and wait object listening
	if ((fFlags & B_SYSTEM_PROFILER_SCHEDULING_EVENTS) != 0) {
		scheduler_add_listener(this);
		fSchedulerNotificationsRequested = true;

		InterruptsSpinLocker waitObjectLocker(gWaitObjectListenerLock);
		add_wait_object_listener(this);
		fWaitObjectNotificationsRequested = true;
		waitObjectLocker.Unlock();

		// fake schedule events for the initially running threads
		int32 cpuCount = smp_get_num_cpus();
		for (int32 i = 0; i < cpuCount; i++) {
			Thread* thread = gCPU[i].running_thread;
			if (thread != NULL)
				ThreadScheduled(thread, thread);
		}
	}

	// I/O scheduling
	if ((fFlags & B_SYSTEM_PROFILER_IO_SCHEDULING_EVENTS) != 0) {
		IOSchedulerRoster* roster = IOSchedulerRoster::Default();
		AutoLocker<IOSchedulerRoster> rosterLocker(roster);

		for (IOSchedulerList::ConstIterator it
				= roster->SchedulerList().GetIterator();
			IOScheduler* scheduler = it.Next();) {
			_IOSchedulerAdded(scheduler);
		}

		fIONotificationsEnabled = true;
	}

	// activate the profiling timers on all CPUs
	if ((fFlags & B_SYSTEM_PROFILER_SAMPLING_EVENTS) != 0)
		call_all_cpus(_InitTimers, this);

	return B_OK;
}


status_t
SystemProfiler::NextBuffer(size_t bytesRead, uint64* _droppedEvents)
{
	InterruptsSpinLocker locker(fLock);

	if (fWaitingProfilerThread != NULL || !fProfilingActive
		|| bytesRead > fBufferSize) {
		return B_BAD_VALUE;
	}

	fBufferSize -= bytesRead;
	fBufferStart += bytesRead;
	if (fBufferStart > fBufferCapacity)
		fBufferStart -= fBufferCapacity;
	fHeader->size = fBufferSize;
	fHeader->start = fBufferStart;

	// already enough data in the buffer to return?
	if (fBufferSize > fBufferCapacity / 2)
		return B_OK;

	// Wait until the buffer gets too full or an error or a timeout occurs.
	while (true) {
		Thread* thread = thread_get_current_thread();
		fWaitingProfilerThread = thread;

		thread_prepare_to_block(thread, B_CAN_INTERRUPT,
			THREAD_BLOCK_TYPE_OTHER, "system profiler buffer");

		locker.Unlock();

		status_t error = thread_block_with_timeout(B_RELATIVE_TIMEOUT, 1000000);

		locker.Lock();

		if (error == B_OK) {
			// the caller has unset fWaitingProfilerThread for us
			break;
		}

		fWaitingProfilerThread = NULL;

		if (error != B_TIMED_OUT)
			return error;

		// just the timeout -- return, if the buffer is not empty
		if (fBufferSize > 0)
			break;
	}

	if (_droppedEvents != NULL) {
		*_droppedEvents = fDroppedEvents;
		fDroppedEvents = 0;
	}

	return B_OK;
}


// #pragma mark - NotificationListener interface


void
SystemProfiler::EventOccurred(NotificationService& service,
	const KMessage* event)
{
	int32 eventCode;
	if (event->FindInt32("event", &eventCode) != B_OK)
		return;

	if (strcmp(service.Name(), "teams") == 0) {
		Team* team = (Team*)event->GetPointer("teamStruct", NULL);
		if (team == NULL)
			return;

		switch (eventCode) {
			case TEAM_ADDED:
				if (fTeamNotificationsEnabled)
					_TeamAdded(team);
				break;

			case TEAM_REMOVED:
				if (team->id == fTeam) {
					// The profiling team is gone -- uninstall the profiler!
					InterruptsSpinLocker locker(sProfilerLock);
					if (sProfiler != this)
						return;

					sProfiler = NULL;
					locker.Unlock();

					ReleaseReference();
					return;
				}

				// When we're still doing the initial team list scan, we are
				// also interested in removals that happened to teams we have
				// already seen.
				if (fTeamNotificationsEnabled
					|| team->serial_number <= fLastTeamAddedSerialNumber) {
					_TeamRemoved(team);
				}
				break;

			case TEAM_EXEC:
				if (fTeamNotificationsEnabled)
					_TeamExec(team);
				break;
		}
	} else if (strcmp(service.Name(), "threads") == 0) {
		Thread* thread = (Thread*)event->GetPointer("threadStruct", NULL);
		if (thread == NULL)
			return;

		switch (eventCode) {
			case THREAD_ADDED:
				if (fThreadNotificationsEnabled)
					_ThreadAdded(thread);
				break;

			case THREAD_REMOVED:
				// When we're still doing the initial thread list scan, we are
				// also interested in removals that happened to threads we have
				// already seen.
				if (fThreadNotificationsEnabled
					|| thread->serial_number <= fLastThreadAddedSerialNumber) {
					_ThreadRemoved(thread);
				}
				break;
		}
	} else if (strcmp(service.Name(), "images") == 0) {
		if (!fImageNotificationsEnabled)
			return;

		struct image* image = (struct image*)event->GetPointer(
			"imageStruct", NULL);
		if (image == NULL)
			return;

		switch (eventCode) {
			case IMAGE_ADDED:
				_ImageAdded(image);
				break;

			case IMAGE_REMOVED:
				_ImageRemoved(image);
				break;
		}
	} else if (strcmp(service.Name(), "I/O") == 0) {
		if (!fIONotificationsEnabled)
			return;

		IOScheduler* scheduler = (IOScheduler*)event->GetPointer("scheduler",
			NULL);
		if (scheduler == NULL)
			return;

		IORequest* request = (IORequest*)event->GetPointer("request", NULL);
		IOOperation* operation = (IOOperation*)event->GetPointer("operation",
			NULL);

		switch (eventCode) {
			case IO_SCHEDULER_ADDED:
				_IOSchedulerAdded(scheduler);
				break;

			case IO_SCHEDULER_REMOVED:
				_IOSchedulerRemoved(scheduler);
				break;

			case IO_SCHEDULER_REQUEST_SCHEDULED:
				_IORequestScheduled(scheduler, request);
				break;

			case IO_SCHEDULER_REQUEST_FINISHED:
				_IORequestFinished(scheduler, request);
				break;

			case IO_SCHEDULER_OPERATION_STARTED:
				_IOOperationStarted(scheduler, request, operation);
				break;

			case IO_SCHEDULER_OPERATION_FINISHED:
				_IOOperationFinished(scheduler, request, operation);
				break;
		}
	}

	_MaybeNotifyProfilerThread();
}


// #pragma mark - SchedulerListener interface


void
SystemProfiler::ThreadEnqueuedInRunQueue(Thread* thread)
{
	int cpu = smp_get_current_cpu();

	InterruptsSpinLocker locker(fLock, false, !fReentered[cpu]);
		// When re-entering, we already hold the lock.

	system_profiler_thread_enqueued_in_run_queue* event
		= (system_profiler_thread_enqueued_in_run_queue*)
			_AllocateBuffer(
				sizeof(system_profiler_thread_enqueued_in_run_queue),
				B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE, cpu, 0);
	if (event == NULL)
		return;

	event->time = system_time_nsecs();
	event->thread = thread->id;
	event->priority = thread->priority;

	fHeader->size = fBufferSize;

	// Unblock the profiler thread, if necessary, but don't unblock the thread,
	// if it had been waiting on a condition variable, since then we'd likely
	// deadlock in ConditionVariable::NotifyOne(), as it acquires a static
	// spinlock.
	if (thread->wait.type != THREAD_BLOCK_TYPE_CONDITION_VARIABLE)
		_MaybeNotifyProfilerThreadLocked();
}


void
SystemProfiler::ThreadRemovedFromRunQueue(Thread* thread)
{
	int cpu = smp_get_current_cpu();

	InterruptsSpinLocker locker(fLock, false, !fReentered[cpu]);
		// When re-entering, we already hold the lock.

	system_profiler_thread_removed_from_run_queue* event
		= (system_profiler_thread_removed_from_run_queue*)
			_AllocateBuffer(
				sizeof(system_profiler_thread_removed_from_run_queue),
				B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE, cpu, 0);
	if (event == NULL)
		return;

	event->time = system_time_nsecs();
	event->thread = thread->id;

	fHeader->size = fBufferSize;

	// unblock the profiler thread, if necessary
	_MaybeNotifyProfilerThreadLocked();
}


void
SystemProfiler::ThreadScheduled(Thread* oldThread, Thread* newThread)
{
	int cpu = smp_get_current_cpu();

	InterruptsSpinLocker locker(fLock, false, !fReentered[cpu]);
		// When re-entering, we already hold the lock.

	// If the old thread starts waiting, handle the wait object.
	if (oldThread->state == B_THREAD_WAITING)
		_WaitObjectUsed((addr_t)oldThread->wait.object, oldThread->wait.type);

	system_profiler_thread_scheduled* event
		= (system_profiler_thread_scheduled*)
			_AllocateBuffer(sizeof(system_profiler_thread_scheduled),
				B_SYSTEM_PROFILER_THREAD_SCHEDULED, cpu, 0);
	if (event == NULL)
		return;

	event->time = system_time_nsecs();
	event->thread = newThread->id;
	event->previous_thread = oldThread->id;
	event->previous_thread_state = oldThread->state;
	event->previous_thread_wait_object_type = oldThread->wait.type;
	event->previous_thread_wait_object = (addr_t)oldThread->wait.object;

	fHeader->size = fBufferSize;

	// unblock the profiler thread, if necessary
	_MaybeNotifyProfilerThreadLocked();
}


// #pragma mark - WaitObjectListener interface


void
SystemProfiler::SemaphoreCreated(sem_id id, const char* name)
{
	_WaitObjectCreated((addr_t)id, THREAD_BLOCK_TYPE_SEMAPHORE);
}


void
SystemProfiler::ConditionVariableInitialized(ConditionVariable* variable)
{
	_WaitObjectCreated((addr_t)variable, THREAD_BLOCK_TYPE_CONDITION_VARIABLE);
}


void
SystemProfiler::MutexInitialized(mutex* lock)
{
	_WaitObjectCreated((addr_t)lock, THREAD_BLOCK_TYPE_MUTEX);
}


void
SystemProfiler::RWLockInitialized(rw_lock* lock)
{
	_WaitObjectCreated((addr_t)lock, THREAD_BLOCK_TYPE_RW_LOCK);
}


// #pragma mark - SystemProfiler private


bool
SystemProfiler::_TeamAdded(Team* team)
{
	TeamLocker teamLocker(team);

	size_t nameLen = strlen(team->Name());
	size_t argsLen = strlen(team->Args());

	InterruptsSpinLocker locker(fLock);

	// During the initial scan check whether the team is already gone again.
	// Later this cannot happen, since the team creator notifies us before
	// actually starting the team.
	if (!fTeamNotificationsEnabled && team->state >= TEAM_STATE_DEATH)
		return true;

	if (team->serial_number > fLastTeamAddedSerialNumber)
		fLastTeamAddedSerialNumber = team->serial_number;

	system_profiler_team_added* event = (system_profiler_team_added*)
		_AllocateBuffer(
			sizeof(system_profiler_team_added) + nameLen + 1 + argsLen,
			B_SYSTEM_PROFILER_TEAM_ADDED, 0, 0);
	if (event == NULL)
		return false;

	event->team = team->id;
	strcpy(event->name, team->Name());
	event->args_offset = nameLen + 1;
	strcpy(event->name + nameLen + 1, team->Args());

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_TeamRemoved(Team* team)
{
	// TODO: It is possible that we get remove notifications for teams that
	// had already been removed from the global team list when we did the
	// initial scan, but were still in the process of dying. ATM it is not
	// really possible to identify such a case.

	TeamLocker teamLocker(team);
	InterruptsSpinLocker locker(fLock);

	system_profiler_team_removed* event = (system_profiler_team_removed*)
		_AllocateBuffer(sizeof(system_profiler_team_removed),
			B_SYSTEM_PROFILER_TEAM_REMOVED, 0, 0);
	if (event == NULL)
		return false;

	event->team = team->id;

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_TeamExec(Team* team)
{
	TeamLocker teamLocker(team);

	size_t argsLen = strlen(team->Args());

	InterruptsSpinLocker locker(fLock);

	system_profiler_team_exec* event = (system_profiler_team_exec*)
		_AllocateBuffer(sizeof(system_profiler_team_exec) + argsLen,
			B_SYSTEM_PROFILER_TEAM_EXEC, 0, 0);
	if (event == NULL)
		return false;

	event->team = team->id;
	strlcpy(event->thread_name, team->main_thread->name,
		sizeof(event->thread_name));
	strcpy(event->args, team->Args());

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_ThreadAdded(Thread* thread)
{
	ThreadLocker threadLocker(thread);
	InterruptsSpinLocker locker(fLock);

	// During the initial scan check whether the team is already gone again.
	// Later this cannot happen, since the team creator notifies us before
	// actually starting the thread.
	if (!fThreadNotificationsEnabled && !thread->IsAlive())
		return true;

	if (thread->serial_number > fLastThreadAddedSerialNumber)
		fLastThreadAddedSerialNumber = thread->serial_number;

	system_profiler_thread_added* event = (system_profiler_thread_added*)
		_AllocateBuffer(sizeof(system_profiler_thread_added),
			B_SYSTEM_PROFILER_THREAD_ADDED, 0, 0);
	if (event == NULL)
		return false;

	event->team = thread->team->id;
	event->thread = thread->id;
	strlcpy(event->name, thread->name, sizeof(event->name));

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_ThreadRemoved(Thread* thread)
{
	// TODO: It is possible that we get remove notifications for threads that
	// had already been removed from the global thread list when we did the
	// initial scan, but were still in the process of dying. ATM it is not
	// really possible to identify such a case.

	ThreadLocker threadLocker(thread);
	InterruptsSpinLocker locker(fLock);

	system_profiler_thread_removed* event
		= (system_profiler_thread_removed*)
			_AllocateBuffer(sizeof(system_profiler_thread_removed),
				B_SYSTEM_PROFILER_THREAD_REMOVED, 0, 0);
	if (event == NULL)
		return false;

	event->team = thread->team->id;
	event->thread = thread->id;

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_ImageAdded(struct image* image)
{
	InterruptsSpinLocker locker(fLock);

	system_profiler_image_added* event = (system_profiler_image_added*)
		_AllocateBuffer(sizeof(system_profiler_image_added),
			B_SYSTEM_PROFILER_IMAGE_ADDED, 0, 0);
	if (event == NULL)
		return false;

	event->team = image->team;
	event->info = image->info.basic_info;

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_ImageRemoved(struct image* image)
{
	InterruptsSpinLocker locker(fLock);

	system_profiler_image_removed* event = (system_profiler_image_removed*)
		_AllocateBuffer(sizeof(system_profiler_image_removed),
			B_SYSTEM_PROFILER_IMAGE_REMOVED, 0, 0);
	if (event == NULL)
		return false;

	event->team = image->team;
	event->image = image->info.basic_info.id;

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_IOSchedulerAdded(IOScheduler* scheduler)
{
	size_t nameLen = strlen(scheduler->Name());

	InterruptsSpinLocker locker(fLock);

	system_profiler_io_scheduler_added* event
		= (system_profiler_io_scheduler_added*)_AllocateBuffer(
			sizeof(system_profiler_io_scheduler_added) + nameLen,
			B_SYSTEM_PROFILER_IO_SCHEDULER_ADDED, 0, 0);
	if (event == NULL)
		return false;

	event->scheduler = scheduler->ID();
	strcpy(event->name, scheduler->Name());

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_IOSchedulerRemoved(IOScheduler* scheduler)
{
	InterruptsSpinLocker locker(fLock);

	system_profiler_io_scheduler_removed* event
		= (system_profiler_io_scheduler_removed*)_AllocateBuffer(
			sizeof(system_profiler_io_scheduler_removed),
			B_SYSTEM_PROFILER_IO_SCHEDULER_REMOVED, 0, 0);
	if (event == NULL)
		return false;

	event->scheduler = scheduler->ID();

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_IORequestScheduled(IOScheduler* scheduler, IORequest* request)
{
	InterruptsSpinLocker locker(fLock);

	system_profiler_io_request_scheduled* event
		= (system_profiler_io_request_scheduled*)_AllocateBuffer(
			sizeof(system_profiler_io_request_scheduled),
			B_SYSTEM_PROFILER_IO_REQUEST_SCHEDULED, 0, 0);
	if (event == NULL)
		return false;

	IORequestOwner* owner = request->Owner();

	event->time = system_time_nsecs();
	event->scheduler = scheduler->ID();
	event->team = owner->team;
	event->thread = owner->thread;
	event->request = request;
	event->offset = request->Offset();
	event->length = request->Length();
	event->write = request->IsWrite();
	event->priority = owner->priority;

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_IORequestFinished(IOScheduler* scheduler, IORequest* request)
{
	InterruptsSpinLocker locker(fLock);

	system_profiler_io_request_finished* event
		= (system_profiler_io_request_finished*)_AllocateBuffer(
			sizeof(system_profiler_io_request_finished),
			B_SYSTEM_PROFILER_IO_REQUEST_FINISHED, 0, 0);
	if (event == NULL)
		return false;

	event->time = system_time_nsecs();
	event->scheduler = scheduler->ID();
	event->request = request;
	event->status = request->Status();
	event->transferred = request->TransferredBytes();

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_IOOperationStarted(IOScheduler* scheduler, IORequest* request,
	IOOperation* operation)
{
	InterruptsSpinLocker locker(fLock);

	system_profiler_io_operation_started* event
		= (system_profiler_io_operation_started*)_AllocateBuffer(
			sizeof(system_profiler_io_operation_started),
			B_SYSTEM_PROFILER_IO_OPERATION_STARTED, 0, 0);
	if (event == NULL)
		return false;

	event->time = system_time_nsecs();
	event->scheduler = scheduler->ID();
	event->request = request;
	event->operation = operation;
	event->offset = request->Offset();
	event->length = request->Length();
	event->write = request->IsWrite();

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_IOOperationFinished(IOScheduler* scheduler, IORequest* request,
	IOOperation* operation)
{
	InterruptsSpinLocker locker(fLock);

	system_profiler_io_operation_finished* event
		= (system_profiler_io_operation_finished*)_AllocateBuffer(
			sizeof(system_profiler_io_operation_finished),
			B_SYSTEM_PROFILER_IO_OPERATION_FINISHED, 0, 0);
	if (event == NULL)
		return false;

	event->time = system_time_nsecs();
	event->scheduler = scheduler->ID();
	event->request = request;
	event->operation = operation;
	event->status = request->Status();
	event->transferred = request->TransferredBytes();

	fHeader->size = fBufferSize;

	return true;
}


void
SystemProfiler::_WaitObjectCreated(addr_t object, uint32 type)
{
	SpinLocker locker(fLock);

	// look up the object
	WaitObjectKey key;
	key.object = object;
	key.type = type;
	WaitObject* waitObject = fWaitObjectTable.Lookup(key);

	// If found, remove it and add it to the free list. This might sound weird,
	// but it makes sense, since we lazily track *used* wait objects only.
	// I.e. the object in the table is now guaranteedly obsolete.
	if (waitObject) {
		fWaitObjectTable.RemoveUnchecked(waitObject);
		fUsedWaitObjects.Remove(waitObject);
		fFreeWaitObjects.Add(waitObject, false);
	}
}

void
SystemProfiler::_WaitObjectUsed(addr_t object, uint32 type)
{
	// look up the object
	WaitObjectKey key;
	key.object = object;
	key.type = type;
	WaitObject* waitObject = fWaitObjectTable.Lookup(key);

	// If already known, re-queue it as most recently used and be done.
	if (waitObject != NULL) {
		fUsedWaitObjects.Remove(waitObject);
		fUsedWaitObjects.Add(waitObject);
		return;
	}

	// not known yet -- get the info
	const char* name = NULL;
	const void* referencedObject = NULL;

	switch (type) {
		case THREAD_BLOCK_TYPE_SEMAPHORE:
		{
			name = sem_get_name_unsafe((sem_id)object);
			break;
		}

		case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
		{
			ConditionVariable* variable = (ConditionVariable*)object;
			name = variable->ObjectType();
			referencedObject = variable->Object();
			break;
		}

		case THREAD_BLOCK_TYPE_MUTEX:
		{
			mutex* lock = (mutex*)object;
			name = lock->name;
			break;
		}

		case THREAD_BLOCK_TYPE_RW_LOCK:
		{
			rw_lock* lock = (rw_lock*)object;
			name = lock->name;
			break;
		}

		case THREAD_BLOCK_TYPE_OTHER:
		{
			name = (const char*)(void*)object;
			break;
		}

		case THREAD_BLOCK_TYPE_SNOOZE:
		case THREAD_BLOCK_TYPE_SIGNAL:
		default:
			return;
	}

	// add the event
	size_t nameLen = name != NULL ? strlen(name) : 0;

	system_profiler_wait_object_info* event
		= (system_profiler_wait_object_info*)
			_AllocateBuffer(sizeof(system_profiler_wait_object_info) + nameLen,
				B_SYSTEM_PROFILER_WAIT_OBJECT_INFO, 0, 0);
	if (event == NULL)
		return;

	event->type = type;
	event->object = object;
	event->referenced_object = (addr_t)referencedObject;
	if (name != NULL)
		strcpy(event->name, name);
	else
		event->name[0] = '\0';

	fHeader->size = fBufferSize;

	// add the wait object

	// get a free one or steal the least recently used one
	waitObject = fFreeWaitObjects.RemoveHead();
	if (waitObject == NULL) {
		waitObject = fUsedWaitObjects.RemoveHead();
		fWaitObjectTable.RemoveUnchecked(waitObject);
	}

	waitObject->object = object;
	waitObject->type = type;
	fWaitObjectTable.InsertUnchecked(waitObject);
	fUsedWaitObjects.Add(waitObject);
}


/*static*/ bool
SystemProfiler::_InitialImageIterator(struct image* image, void* cookie)
{
	SystemProfiler* self = (SystemProfiler*)cookie;
	self->fImageNotificationsEnabled = true;
		// Set that here, since the image lock is being held now.
	return !self->_ImageAdded(image);
}


void*
SystemProfiler::_AllocateBuffer(size_t size, int event, int cpu, int count)
{
	size = (size + 3) / 4 * 4;
	size += sizeof(system_profiler_event_header);

	size_t end = fBufferStart + fBufferSize;
	if (end + size > fBufferCapacity) {
		// Buffer is wrapped or needs wrapping.
		if (end < fBufferCapacity) {
			// not wrapped yet, but needed
			system_profiler_event_header* header
				= (system_profiler_event_header*)(fBufferBase + end);
			header->event = B_SYSTEM_PROFILER_BUFFER_END;
			fBufferSize = fBufferCapacity - fBufferStart;
			end = 0;
		} else
			end -= fBufferCapacity;

		if (end + size > fBufferStart) {
			fDroppedEvents++;
			return NULL;
		}
	}

	system_profiler_event_header* header
		= (system_profiler_event_header*)(fBufferBase + end);
	header->event = event;
	header->cpu = cpu;
	header->size = size - sizeof(system_profiler_event_header);

	fBufferSize += size;

	return header + 1;
}


/*static*/ void
SystemProfiler::_InitTimers(void* cookie, int cpu)
{
	SystemProfiler* self = (SystemProfiler*)cookie;
	self->_ScheduleTimer(cpu);
}


/*static*/ void
SystemProfiler::_UninitTimers(void* cookie, int cpu)
{
	SystemProfiler* self = (SystemProfiler*)cookie;

	CPUProfileData& cpuData = self->fCPUData[cpu];
	cancel_timer(&cpuData.timer);
	cpuData.timerScheduled = false;
}


void
SystemProfiler::_ScheduleTimer(int cpu)
{
	CPUProfileData& cpuData = fCPUData[cpu];
	cpuData.timerEnd = system_time() + fInterval;
	cpuData.timer.user_data = this;
	add_timer(&cpuData.timer, &_ProfilingEvent, fInterval,
		B_ONE_SHOT_RELATIVE_TIMER);
	cpuData.timerScheduled = true;
}


void
SystemProfiler::_DoSample()
{
	Thread* thread = thread_get_current_thread();
	int cpu = thread->cpu->cpu_num;
	CPUProfileData& cpuData = fCPUData[cpu];

	// get the samples
	int32 count = arch_debug_get_stack_trace(cpuData.buffer, fStackDepth, 1,
		0, STACK_TRACE_KERNEL | STACK_TRACE_USER);

	InterruptsSpinLocker locker(fLock);

	system_profiler_samples* event = (system_profiler_samples*)
		_AllocateBuffer(sizeof(system_profiler_samples)
				+ count * sizeof(addr_t),
			B_SYSTEM_PROFILER_SAMPLES, cpu, count);
	if (event == NULL)
		return;

	event->thread = thread->id;
	memcpy(event->samples, cpuData.buffer, count * sizeof(addr_t));

	fHeader->size = fBufferSize;
}


/*static*/ int32
SystemProfiler::_ProfilingEvent(struct timer* timer)
{
	SystemProfiler* self = (SystemProfiler*)timer->user_data;

	self->_DoSample();
	self->_ScheduleTimer(timer->cpu);

	return B_HANDLED_INTERRUPT;
}


// #pragma mark - private kernel API


#if SYSTEM_PROFILER

status_t
start_system_profiler(size_t areaSize, uint32 stackDepth, bigtime_t interval)
{
	struct ParameterDeleter {
		ParameterDeleter(area_id area)
			:
			fArea(area),
			fDetached(false)
		{
		}

		~ParameterDeleter()
		{
			if (!fDetached) {
				delete_area(fArea);
				delete sRecordedParameters;
				sRecordedParameters = NULL;
			}
		}

		void Detach()
		{
			fDetached = true;
		}

	private:
		area_id	fArea;
		bool	fDetached;
	};

	void* address;
	area_id area = create_area("kernel profile data", &address,
		B_ANY_KERNEL_ADDRESS, areaSize, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < 0)
		return area;

	ParameterDeleter parameterDeleter(area);

	sRecordedParameters = new(std::nothrow) system_profiler_parameters;
	if (sRecordedParameters == NULL)
		return B_NO_MEMORY;

	sRecordedParameters->buffer_area = area;
	sRecordedParameters->flags = B_SYSTEM_PROFILER_TEAM_EVENTS
		| B_SYSTEM_PROFILER_THREAD_EVENTS | B_SYSTEM_PROFILER_IMAGE_EVENTS
		| B_SYSTEM_PROFILER_IO_SCHEDULING_EVENTS
		| B_SYSTEM_PROFILER_SAMPLING_EVENTS;
	sRecordedParameters->locking_lookup_size = 4096;
	sRecordedParameters->interval = interval;
	sRecordedParameters->stack_depth = stackDepth;

	area_info areaInfo;
	get_area_info(area, &areaInfo);

	// initialize the profiler
	SystemProfiler* profiler = new(std::nothrow) SystemProfiler(B_SYSTEM_TEAM,
		areaInfo, *sRecordedParameters);
	if (profiler == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<SystemProfiler> profilerDeleter(profiler);

	status_t error = profiler->Init();
	if (error != B_OK)
		return error;

	// set the new profiler
	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler != NULL)
		return B_BUSY;

	parameterDeleter.Detach();
	profilerDeleter.Detach();
	sProfiler = profiler;
	locker.Unlock();

	return B_OK;
}


void
stop_system_profiler()
{
	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler == NULL)
		return;

	SystemProfiler* profiler = sProfiler;
	sProfiler = NULL;
	locker.Unlock();

	profiler->ReleaseReference();
}

#endif	// SYSTEM_PROFILER


// #pragma mark - syscalls


status_t
_user_system_profiler_start(struct system_profiler_parameters* userParameters)
{
	if (geteuid() != 0)
		return B_PERMISSION_DENIED;

	// copy params to the kernel
	struct system_profiler_parameters parameters;
	if (userParameters == NULL || !IS_USER_ADDRESS(userParameters)
		|| user_memcpy(&parameters, userParameters, sizeof(parameters))
			!= B_OK) {
		return B_BAD_ADDRESS;
	}

	// check the parameters
	team_id team = thread_get_current_thread()->team->id;

	area_info areaInfo;
	status_t error = get_area_info(parameters.buffer_area, &areaInfo);
	if (error != B_OK)
		return error;

	if (areaInfo.team != team)
		return B_BAD_VALUE;

	if ((parameters.flags & B_SYSTEM_PROFILER_SAMPLING_EVENTS) != 0) {
		if (parameters.stack_depth < 1)
			return B_BAD_VALUE;

		if (parameters.interval < B_DEBUG_MIN_PROFILE_INTERVAL)
			parameters.interval = B_DEBUG_MIN_PROFILE_INTERVAL;

		if (parameters.stack_depth > B_DEBUG_STACK_TRACE_DEPTH)
			parameters.stack_depth = B_DEBUG_STACK_TRACE_DEPTH;
	}

	// quick check to see whether we do already have a profiler installed
	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler != NULL)
		return B_BUSY;
	locker.Unlock();

	// initialize the profiler
	SystemProfiler* profiler = new(std::nothrow) SystemProfiler(team, areaInfo,
		parameters);
	if (profiler == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<SystemProfiler> profilerDeleter(profiler);

	error = profiler->Init();
	if (error != B_OK)
		return error;

	// set the new profiler
	locker.Lock();
	if (sProfiler != NULL)
		return B_BUSY;

	profilerDeleter.Detach();
	sProfiler = profiler;
	locker.Unlock();

	return B_OK;
}


status_t
_user_system_profiler_next_buffer(size_t bytesRead, uint64* _droppedEvents)
{
	if (geteuid() != 0)
		return B_PERMISSION_DENIED;

	if (_droppedEvents != NULL && !IS_USER_ADDRESS(_droppedEvents))
		return B_BAD_ADDRESS;

	team_id team = thread_get_current_thread()->team->id;

	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler == NULL || sProfiler->TeamID() != team)
		return B_BAD_VALUE;

	// get a reference to the profiler
	SystemProfiler* profiler = sProfiler;
	BReference<SystemProfiler> reference(profiler);
	locker.Unlock();

	uint64 droppedEvents = 0;
	status_t error = profiler->NextBuffer(bytesRead,
		_droppedEvents != NULL ? &droppedEvents : NULL);
	if (error == B_OK && _droppedEvents != NULL)
		user_memcpy(_droppedEvents, &droppedEvents, sizeof(droppedEvents));

	return error;
}


status_t
_user_system_profiler_stop()
{
	if (geteuid() != 0)
		return B_PERMISSION_DENIED;

	team_id team = thread_get_current_thread()->team->id;

	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler == NULL || sProfiler->TeamID() != team)
		return B_BAD_VALUE;

	SystemProfiler* profiler = sProfiler;
	sProfiler = NULL;
	locker.Unlock();

	profiler->ReleaseReference();

	return B_OK;
}


status_t
_user_system_profiler_recorded(system_profiler_parameters* userParameters)
{
	if (geteuid() != 0)
		return B_PERMISSION_DENIED;

	if (userParameters == NULL || !IS_USER_ADDRESS(userParameters))
		return B_BAD_ADDRESS;
	if (sRecordedParameters == NULL)
		return B_ERROR;

#if SYSTEM_PROFILER
	stop_system_profiler();

	// Transfer the area to the userland process

	void* address;
	area_id newArea = transfer_area(sRecordedParameters->buffer_area, &address,
		B_ANY_ADDRESS, team_get_current_team_id(), true);
	if (newArea < 0)
		return newArea;

	status_t status = set_area_protection(newArea, B_READ_AREA);
	if (status == B_OK) {
		sRecordedParameters->buffer_area = newArea;

		status = user_memcpy(userParameters, sRecordedParameters,
			sizeof(system_profiler_parameters));
	}
	if (status != B_OK)
		delete_area(newArea);

	delete sRecordedParameters;
	sRecordedParameters = NULL;

	return status;
#else
	return B_NOT_SUPPORTED;
#endif // SYSTEM_PROFILER
}
