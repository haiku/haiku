/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <system_profiler.h>

#include <AutoDeleter.h>
#include <Referenceable.h>

#include <util/AutoLock.h>

#include <system_profiler_defs.h>

#include <cpu.h>
#include <kimage.h>
#include <Notifications.h>
#include <team.h>
#include <thread.h>
#include <user_debugger.h>

#include <arch/debug.h>


// This is the kernel-side implementation of the system profiling support.
// A userland team can register as system profiler, providing an area as buffer
// for events. Those events are team, thread, and image changes (added/removed)
// and periodic sampling of the return address stack for each CPU.


class SystemProfiler;


static spinlock sProfilerLock = B_SPINLOCK_INITIALIZER;
static SystemProfiler* sProfiler = NULL;


class SystemProfiler : public Referenceable, private NotificationListener {
public:
								SystemProfiler(team_id team,
									const area_info& userAreaInfo,
									bigtime_t interval, int32 stackDepth);
								~SystemProfiler();

			team_id				Team() const	{ return fTeam; }

			status_t			Init();
			status_t			NextBuffer(size_t bytesRead);

private:
    virtual	void				EventOccured(NotificationService& service,
									const KMessage* event);

			bool				_TeamAdded(struct team* team);
			bool				_TeamRemoved(struct team* team);
			bool				_TeamExec(struct team* team);

			bool				_ThreadAdded(struct thread* thread);
			bool				_ThreadRemoved(struct thread* thread);

			bool				_ImageAdded(struct image* image);
			bool				_ImageRemoved(struct image* image);

	static	bool				_InitialTeamIterator(struct team* team,
									void* cookie);
	static	bool				_InitialThreadIterator(struct thread* thread,
									void* cookie);
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

private:
			spinlock			fLock;
			team_id				fTeam;
			area_id				fUserArea;
			area_id				fKernelArea;
			size_t				fAreaSize;
			int32				fStackDepth;
			bigtime_t			fInterval;
			system_profiler_buffer_header* fHeader;
			uint8*				fBufferBase;
			size_t				fBufferCapacity;
			size_t				fBufferStart;
			size_t				fBufferSize;
			bool				fTeamNotificationsRequested;
			bool				fTeamNotificationsEnabled;
			bool				fThreadNotificationsRequested;
			bool				fThreadNotificationsEnabled;
			bool				fImageNotificationsRequested;
			bool				fImageNotificationsEnabled;
			ConditionVariable	fProfilerWaitCondition;
			bool				fProfilerWaiting;
			bool				fProfilingActive;
			CPUProfileData		fCPUData[B_MAX_CPU_COUNT];
};


SystemProfiler::SystemProfiler(team_id team, const area_info& userAreaInfo,
	bigtime_t interval, int32 stackDepth)
	:
	fTeam(team),
	fUserArea(userAreaInfo.area),
	fKernelArea(-1),
	fAreaSize(userAreaInfo.size),
	fStackDepth(stackDepth),
	fInterval(interval),
	fHeader(NULL),
	fBufferBase(NULL),
	fBufferCapacity(0),
	fBufferStart(0),
	fBufferSize(0),
	fTeamNotificationsRequested(false),
	fTeamNotificationsEnabled(false),
	fThreadNotificationsRequested(false),
	fThreadNotificationsEnabled(false),
	fImageNotificationsRequested(false),
	fImageNotificationsEnabled(false),
	fProfilerWaiting(false)
{
	B_INITIALIZE_SPINLOCK(&fLock);
	fProfilerWaitCondition.Init(this, "system profiler");
}


SystemProfiler::~SystemProfiler()
{
	// Wake up the user thread, if it is waiting, and mark profiling
	// inactive.
	InterruptsSpinLocker locker(fLock);
	if (fProfilerWaiting) {
		fProfilerWaiting = false;
		fProfilerWaitCondition.NotifyAll();
	}
	fProfilingActive = false;
	locker.Unlock();

	// deactivate the profiling timers on all CPUs
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
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
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

	// start listening for notifications

	// teams
	NotificationManager& notificationManager
		= NotificationManager::Manager();
	error = notificationManager.AddListener("teams",
		TEAM_ADDED | TEAM_REMOVED | TEAM_EXEC, *this);
	if (error != B_OK)
		return error;
	fTeamNotificationsRequested = true;

	// threads
	error = notificationManager.AddListener("threads",
		THREAD_ADDED | THREAD_REMOVED, *this);
	if (error != B_OK)
		return error;
	fThreadNotificationsRequested = true;

	// images
	error = notificationManager.AddListener("images",
		IMAGE_ADDED | IMAGE_REMOVED, *this);
	if (error != B_OK)
		return error;
	fImageNotificationsRequested = true;

	// We need to fill the buffer with the initial state of teams, threads,
	// and images.

	// teams
	InterruptsSpinLocker teamsLocker(gTeamSpinlock);
	if (team_iterate_through_teams(&_InitialTeamIterator, this) != NULL)
		return B_BUFFER_OVERFLOW;
	fTeamNotificationsEnabled = true;
	teamsLocker.Unlock();

	// threads
	InterruptsSpinLocker threadsLocker(gThreadSpinlock);
	if (thread_iterate_through_threads(&_InitialThreadIterator, this)
			!= NULL) {
		return B_BUFFER_OVERFLOW;
	}
	fThreadNotificationsEnabled = true;
	threadsLocker.Unlock();

	// images
	if (image_iterate_through_images(&_InitialImageIterator, this) != NULL)
		return B_BUFFER_OVERFLOW;

	fProfilingActive = true;

	// activate the profiling timers on all CPUs
	call_all_cpus(_InitTimers, this);

	return B_OK;
}


status_t
SystemProfiler::NextBuffer(size_t bytesRead)
{
	InterruptsSpinLocker locker(fLock);

	if (fProfilerWaiting || !fProfilingActive || bytesRead > fBufferSize)
		return B_BAD_VALUE;

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
		ConditionVariableEntry waitEntry;
		fProfilerWaitCondition.Add(&waitEntry);

		fProfilerWaiting = true;

		locker.Unlock();

		status_t error = waitEntry.Wait(
			B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 1000000);
		if (error == B_OK) {
			// the caller has unset fProfilerWaiting for us
			return B_OK;
		}

		locker.Lock();
		fProfilerWaiting = false;

		if (error != B_TIMED_OUT)
			return error;

		// just the timeout -- return, if the buffer is not empty
		if (fBufferSize > 0)
			return B_OK;
	}
}


void
SystemProfiler::EventOccured(NotificationService& service,
	const KMessage* event)
{
	int32 eventCode;
	if (event->FindInt32("event", &eventCode) != B_OK)
		return;

	if (strcmp(service.Name(), "teams") == 0) {
		if (!fTeamNotificationsEnabled)
			return;

		struct team* team = (struct team*)event->GetPointer("teamStruct",
			NULL);
		if (team == NULL)
			return;

		switch (eventCode) {
			case TEAM_ADDED:
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

					RemoveReference();
				} else
					_TeamRemoved(team);

				break;

			case TEAM_EXEC:
				_TeamExec(team);
				break;
		}
	} else if (strcmp(service.Name(), "threads") == 0) {
		if (!fThreadNotificationsEnabled)
			return;

		struct thread* thread = (struct thread*)event->GetPointer(
			"threadStruct", NULL);
		if (thread == NULL)
			return;

		switch (eventCode) {
			case THREAD_ADDED:
				_ThreadAdded(thread);
				break;

			case THREAD_REMOVED:
				_ThreadRemoved(thread);
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
	}
}


bool
SystemProfiler::_TeamAdded(struct team* team)
{
	InterruptsSpinLocker locker(fLock);

	size_t argsLen = strlen(team->args);

	system_profiler_team_added* event = (system_profiler_team_added*)
		_AllocateBuffer(sizeof(system_profiler_team_added) + argsLen,
			B_SYSTEM_PROFILER_TEAM_ADDED, 0, 0);
	if (event == NULL)
		return false;

	event->team = team->id;
	strcpy(event->args, team->args);

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_TeamRemoved(struct team* team)
{
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
SystemProfiler::_TeamExec(struct team* team)
{
	InterruptsSpinLocker locker(fLock);

	size_t argsLen = strlen(team->args);

	system_profiler_team_exec* event = (system_profiler_team_exec*)
		_AllocateBuffer(sizeof(system_profiler_team_exec) + argsLen,
			B_SYSTEM_PROFILER_TEAM_EXEC, 0, 0);
	if (event == NULL)
		return false;

	event->team = team->id;
	strlcpy(event->thread_name, team->main_thread->name,
		sizeof(event->thread_name));
	strcpy(event->args, team->args);

	fHeader->size = fBufferSize;

	return true;
}


bool
SystemProfiler::_ThreadAdded(struct thread* thread)
{
	InterruptsSpinLocker locker(fLock);

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
SystemProfiler::_ThreadRemoved(struct thread* thread)
{
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
	event->info = image->info;

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
	event->image = image->info.id;

	fHeader->size = fBufferSize;

	return true;
}


/*static*/ bool
SystemProfiler::_InitialTeamIterator(struct team* team, void* cookie)
{
	SystemProfiler* self = (SystemProfiler*)cookie;
	return !self->_TeamAdded(team);
}


/*static*/ bool
SystemProfiler::_InitialThreadIterator(struct thread* thread, void* cookie)
{
	SystemProfiler* self = (SystemProfiler*)cookie;
	return !self->_ThreadAdded(thread);
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
	size += sizeof(system_profiler_event_header);

	size_t end = fBufferStart + fBufferSize;
	if (end + size > fBufferCapacity) {
		// Buffer is wrapped or needs wrapping.
		if (end < fBufferCapacity) {
			// not wrapped yet, but needed
			system_profiler_event_header* header
				= (system_profiler_event_header*)(fBufferBase + end);
			header->event = B_SYSTEM_PROFILER_SAMPLES_END;
			fBufferSize = fBufferCapacity - fBufferStart;
			end = 0;
		} else
			end -= fBufferCapacity;

		if (end + size > fBufferStart)
			return NULL;
	}

	system_profiler_event_header* header
		= (system_profiler_event_header*)(fBufferBase + end);
	header->event = event;
	header->cpu = cpu;
	header->size = size - sizeof(system_profiler_event_header);

	fBufferSize += size;

	// If the buffer is full enough notify the profiler.
	if (fProfilerWaiting && fBufferSize > fBufferCapacity / 2) {
		fProfilerWaiting = false;
		fProfilerWaitCondition.NotifyOne();
	}

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
	struct thread* thread = thread_get_current_thread();
	int cpu = thread->cpu->cpu_num;
	CPUProfileData& cpuData = fCPUData[cpu];

	// get the samples
	int32 count = arch_debug_get_stack_trace(cpuData.buffer, fStackDepth, 1,
		0, false);

	InterruptsSpinLocker locker(fLock);

	system_profiler_samples* event = (system_profiler_samples*)
		_AllocateBuffer(sizeof(system_profiler_samples)
				+ count * sizeof(addr_t),
			B_SYSTEM_PROFILER_SAMPLES, cpu, count);
	if (event == NULL) {
		// TODO: Count drops!
		return;
	}

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


// #pragma mark - syscalls


status_t
_user_system_profiler_start(area_id bufferArea, bigtime_t interval,
	int32 stackDepth)
{
	// check the parameters
	team_id team = thread_get_current_thread()->team->id;

	area_info areaInfo;
	status_t error = get_area_info(bufferArea, &areaInfo);
	if (error != B_OK)
		return error;

	if (areaInfo.team != team || stackDepth < 1)
		return B_BAD_VALUE;

	if (interval < B_DEBUG_MIN_PROFILE_INTERVAL)
		interval = B_DEBUG_MIN_PROFILE_INTERVAL;

	if (stackDepth > B_DEBUG_STACK_TRACE_DEPTH)
		stackDepth = B_DEBUG_STACK_TRACE_DEPTH;

	// quick check to see whether we do already have a profiler installed
	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler != NULL)
		return B_BUSY;
	locker.Unlock();

	// initialize the profiler
	SystemProfiler* profiler = new(std::nothrow) SystemProfiler(team, areaInfo,
		interval, stackDepth);
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
_user_system_profiler_next_buffer(size_t bytesRead)
{
	team_id team = thread_get_current_thread()->team->id;

	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler == NULL || sProfiler->Team() != team)
		return B_BAD_VALUE;

	// get a reference to the profiler
	SystemProfiler* profiler = sProfiler;
	Reference<SystemProfiler> reference(profiler);
	locker.Unlock();

	return profiler->NextBuffer(bytesRead);
}


status_t
_user_system_profiler_stop()
{
	team_id team = thread_get_current_thread()->team->id;

	InterruptsSpinLocker locker(sProfilerLock);
	if (sProfiler == NULL || sProfiler->Team() != team)
		return B_BAD_VALUE;

	SystemProfiler* profiler = sProfiler;
	sProfiler = NULL;
	locker.Unlock();

	profiler->RemoveReference();

	return B_OK;
}
