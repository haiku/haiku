/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <UserTimer.h>

#include <algorithm>

#include <AutoDeleter.h>

#include <debug.h>
#include <kernel.h>
#include <real_time_clock.h>
#include <team.h>
#include <thread_types.h>
#include <UserEvent.h>
#include <util/AutoLock.h>


// Minimum interval length in microseconds for a periodic timer. This is not a
// restriction on the user timer interval length itself, but the minimum time
// span by which we advance the start time for kernel timers. A shorted user
// timer interval will result in the overrun count to be increased every time
// the kernel timer is rescheduled.
static const bigtime_t kMinPeriodicTimerInterval = 100;

static RealTimeUserTimerList sAbsoluteRealTimeTimers;
static spinlock sAbsoluteRealTimeTimersLock = B_SPINLOCK_INITIALIZER;


// #pragma mark - TimerLocker


namespace {

struct TimerLocker {
	Team*	team;
	Thread*	thread;

	TimerLocker()
		:
		team(NULL),
		thread(NULL)
	{
	}

	~TimerLocker()
	{
		Unlock();
	}

	void Lock(Team* team, Thread* thread)
	{
		this->team = team;
		team->Lock();

		this->thread = thread;

		if (thread != NULL) {
			thread->AcquireReference();
			thread->Lock();
		}

		// We don't check thread->team != team here, since this method can be
		// called for new threads not added to the team yet.
	}

	status_t LockAndGetTimer(thread_id threadID, int32 timerID,
		UserTimer*& _timer)
	{
		team = thread_get_current_thread()->team;
		team->Lock();

		if (threadID >= 0) {
			thread = Thread::GetAndLock(threadID);
			if (thread == NULL)
				return B_BAD_THREAD_ID;
			if (thread->team != team)
				return B_NOT_ALLOWED;
		}

		UserTimer* timer = thread != NULL
			? thread->UserTimerFor(timerID) : team->UserTimerFor(timerID);
		if (timer == NULL)
			return B_BAD_VALUE;

		_timer = timer;
		return B_OK;
	}

	void Unlock()
	{
		if (thread != NULL) {
			thread->UnlockAndReleaseReference();
			thread = NULL;
		}
		if (team != NULL) {
			team->Unlock();
			team = NULL;
		}
	}
};

}	// unnamed namespace


// #pragma mark - UserTimer


UserTimer::UserTimer()
	:
	fID(-1),
	fEvent(NULL),
	fNextTime(0),
	fInterval(0),
	fOverrunCount(0),
	fScheduled(false)
{
	// mark the timer unused
	fTimer.user_data = this;
}


UserTimer::~UserTimer()
{
	delete fEvent;
}


/*!	\fn UserTimer::Schedule(bigtime_t nextTime, bigtime_t interval,
		bigtime_t& _oldRemainingTime, bigtime_t& _oldInterval)
	Cancels the timer, if it is already scheduled, and optionally schedules it
	with new parameters.

	The caller must not hold the scheduler lock.

	\param nextTime The time at which the timer should go off the next time. If
		\c B_INFINITE_TIMEOUT, the timer will not be scheduled. Whether the
		value is interpreted as absolute or relative time, depends on \c flags.
	\param interval If <tt> >0 </tt>, the timer will be scheduled to fire
		periodically every \a interval microseconds. Otherwise it will fire
		only once at \a nextTime. If \a nextTime is \c B_INFINITE_TIMEOUT, it
		will fire never in either case.
	\param flags Bitwise OR of flags. Currently \c B_ABSOLUTE_TIMEOUT and
		\c B_RELATIVE_TIMEOUT are supported, indicating whether \a nextTime is
		an absolute or relative time.
	\param _oldRemainingTime Return variable that will be set to the
		microseconds remaining to the time for which the timer was scheduled
		next before the call. If it wasn't scheduled, the variable is set to
		\c B_INFINITE_TIMEOUT.
	\param _oldInterval Return variable that will be set to the interval in
		microseconds the timer was to be scheduled periodically. If the timer
		wasn't periodic, the variable is set to \c 0.
*/


/*!	Cancels the timer, if it is scheduled.

	The caller must not hold the scheduler lock.
*/
void
UserTimer::Cancel()
{
	bigtime_t oldNextTime;
	bigtime_t oldInterval;
	return Schedule(B_INFINITE_TIMEOUT, 0, 0, oldNextTime, oldInterval);
}


/*!	\fn UserTimer::GetInfo(bigtime_t& _remainingTime, bigtime_t& _interval,
		uint32& _overrunCount)
	Return information on the current timer.

	The caller must not hold the scheduler lock.

	\param _remainingTime Return variable that will be set to the microseconds
		remaining to the time for which the timer was scheduled next before the
		call. If it wasn't scheduled, the variable is set to
		\c B_INFINITE_TIMEOUT.
	\param _interval Return variable that will be set to the interval in
		microseconds the timer is to be scheduled periodically. If the timer
		isn't periodic, the variable is set to \c 0.
	\param _overrunCount Return variable that will be set to the number of times
		the timer went off, but its event couldn't be delivered, since it's
		previous delivery hasn't been handled yet.
*/


/*static*/ int32
UserTimer::HandleTimerHook(struct timer* timer)
{
	((UserTimer*)timer->user_data)->HandleTimer();
	return B_HANDLED_INTERRUPT;
}


void
UserTimer::HandleTimer()
{
	if (fEvent != NULL) {
		// fire the event and update the overrun count, if necessary
		status_t error = fEvent->Fire();
		if (error == B_BUSY) {
			if (fOverrunCount < MAX_USER_TIMER_OVERRUN_COUNT)
				fOverrunCount++;
		}
	}

	// Since we don't use periodic kernel timers, it isn't scheduled anymore.
	// If the timer is periodic, the derived class' version will schedule it
	// again.
	fScheduled = false;
}


/*!	Updates the start time for a periodic timer after it expired, enforcing
	sanity limits and updating \c fOverrunCount, if necessary.

	The caller must not hold the scheduler lock.
*/
void
UserTimer::UpdatePeriodicStartTime()
{
	if (fInterval < kMinPeriodicTimerInterval) {
		bigtime_t skip = (kMinPeriodicTimerInterval + fInterval - 1) / fInterval;
		fNextTime += skip * fInterval;

		// One interval is the normal advance, so don't consider it skipped.
		skip--;

		if (skip + fOverrunCount > MAX_USER_TIMER_OVERRUN_COUNT)
			fOverrunCount = MAX_USER_TIMER_OVERRUN_COUNT;
		else
			fOverrunCount += skip;
	} else
		fNextTime += fInterval;
}


/*!	Checks whether the timer start time lies too much in the past and, if so,
	adjusts it and updates \c fOverrunCount.

	The caller must not hold the scheduler lock.

	\param now The current time.
*/
void
UserTimer::CheckPeriodicOverrun(bigtime_t now)
{
	if (fNextTime + fInterval > now)
		return;

	// The start time is a full interval or more in the past. Skip those
	// intervals.
	bigtime_t skip = (now - fNextTime) / fInterval;
	fNextTime += skip * fInterval;

	if (skip + fOverrunCount > MAX_USER_TIMER_OVERRUN_COUNT)
		fOverrunCount = MAX_USER_TIMER_OVERRUN_COUNT;
	else
		fOverrunCount += skip;
}


// #pragma mark - SystemTimeUserTimer


void
SystemTimeUserTimer::Schedule(bigtime_t nextTime, bigtime_t interval,
	uint32 flags, bigtime_t& _oldRemainingTime, bigtime_t& _oldInterval)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	// get the current time
	bigtime_t now = system_time();

	// Cancel the old timer, if still scheduled, and get the previous values.
	if (fScheduled) {
		cancel_timer(&fTimer);

		_oldRemainingTime = fNextTime - now;
		_oldInterval = fInterval;

		fScheduled = false;
	} else {
		_oldRemainingTime = B_INFINITE_TIMEOUT;
		_oldInterval = 0;
	}

	// schedule the new timer
	fNextTime = nextTime;
	fInterval = interval;
	fOverrunCount = 0;

	if (nextTime == B_INFINITE_TIMEOUT)
		return;

	if ((flags & B_RELATIVE_TIMEOUT) != 0)
		fNextTime += now;

	ScheduleKernelTimer(now, fInterval > 0);
}


void
SystemTimeUserTimer::GetInfo(bigtime_t& _remainingTime, bigtime_t& _interval,
	uint32& _overrunCount)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	if (fScheduled) {
		_remainingTime = fNextTime - system_time();
		_interval = fInterval;
	} else {
		_remainingTime = B_INFINITE_TIMEOUT;
		_interval = 0;
	}

	_overrunCount = fOverrunCount;
}


void
SystemTimeUserTimer::HandleTimer()
{
	UserTimer::HandleTimer();

	// if periodic, reschedule the kernel timer
	if (fInterval > 0) {
		UpdatePeriodicStartTime();
		ScheduleKernelTimer(system_time(), true);
	}
}


/*!	Schedules the kernel timer.

	The caller must hold the scheduler lock.

	\param now The current system time to be used.
	\param checkPeriodicOverrun If \c true, calls CheckPeriodicOverrun() first,
		i.e. the start time will be adjusted to not lie too much in the past.
*/
void
SystemTimeUserTimer::ScheduleKernelTimer(bigtime_t now,
	bool checkPeriodicOverrun)
{
	// If periodic, check whether the start time is too far in the past.
	if (checkPeriodicOverrun)
		CheckPeriodicOverrun(now);

	uint32 timerFlags = B_ONE_SHOT_ABSOLUTE_TIMER
			| B_TIMER_USE_TIMER_STRUCT_TIMES | B_TIMER_ACQUIRE_SCHEDULER_LOCK;
		// We use B_TIMER_ACQUIRE_SCHEDULER_LOCK to avoid race conditions
		// between setting/canceling the timer and the event handler.

	fTimer.schedule_time = std::max(fNextTime, (bigtime_t)0);
	fTimer.period = 0;

	add_timer(&fTimer, &HandleTimerHook, fTimer.schedule_time, timerFlags);

	fScheduled = true;
}


// #pragma mark - RealTimeUserTimer


void
RealTimeUserTimer::Schedule(bigtime_t nextTime, bigtime_t interval,
	uint32 flags, bigtime_t& _oldRemainingTime, bigtime_t& _oldInterval)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	// get the current time
	bigtime_t now = system_time();

	// Cancel the old timer, if still scheduled, and get the previous values.
	if (fScheduled) {
		cancel_timer(&fTimer);

		_oldRemainingTime = fNextTime - now;
		_oldInterval = fInterval;

		if (fAbsolute) {
			SpinLocker globalListLocker(sAbsoluteRealTimeTimersLock);
			sAbsoluteRealTimeTimers.Remove(this);
		}

		fScheduled = false;
	} else {
		_oldRemainingTime = B_INFINITE_TIMEOUT;
		_oldInterval = 0;
	}

	// schedule the new timer
	fNextTime = nextTime;
	fInterval = interval;
	fOverrunCount = 0;

	if (nextTime == B_INFINITE_TIMEOUT)
		return;

	fAbsolute = (flags & B_RELATIVE_TIMEOUT) == 0;

	if (fAbsolute) {
		fRealTimeOffset = rtc_boot_time();
		fNextTime -= fRealTimeOffset;

		// If periodic, check whether the start time is too far in the past.
		if (fInterval > 0)
			CheckPeriodicOverrun(now);

		// add the absolute timer to the global list
		SpinLocker globalListLocker(sAbsoluteRealTimeTimersLock);
		sAbsoluteRealTimeTimers.Insert(this);
	} else
		fNextTime += now;

	ScheduleKernelTimer(now, false);
}


/*!	Called when the real-time clock has been changed.

	The caller must hold the scheduler lock. Optionally the caller may also
	hold \c sAbsoluteRealTimeTimersLock.
*/
void
RealTimeUserTimer::TimeWarped()
{
	ASSERT(fScheduled && fAbsolute);

	// get the new real-time offset
	bigtime_t oldRealTimeOffset = fRealTimeOffset;
	fRealTimeOffset = rtc_boot_time();
	if (fRealTimeOffset == oldRealTimeOffset)
		return;

	// cancel the kernel timer and reschedule it
	cancel_timer(&fTimer);

	fNextTime += oldRealTimeOffset - fRealTimeOffset;

	ScheduleKernelTimer(system_time(), fInterval > 0);
}


void
RealTimeUserTimer::HandleTimer()
{
	SystemTimeUserTimer::HandleTimer();

	// remove from global list, if no longer scheduled
	if (!fScheduled && fAbsolute) {
		SpinLocker globalListLocker(sAbsoluteRealTimeTimersLock);
		sAbsoluteRealTimeTimers.Remove(this);
	}
}


// #pragma mark - TeamTimeUserTimer


TeamTimeUserTimer::TeamTimeUserTimer(team_id teamID)
	:
	fTeamID(teamID),
	fTeam(NULL)
{
}


TeamTimeUserTimer::~TeamTimeUserTimer()
{
	ASSERT(fTeam == NULL);
}


void
TeamTimeUserTimer::Schedule(bigtime_t nextTime, bigtime_t interval,
	uint32 flags, bigtime_t& _oldRemainingTime, bigtime_t& _oldInterval)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	// get the current time, but only if needed
	bool nowValid = fTeam != NULL;
	bigtime_t now = nowValid ? fTeam->CPUTime(false) : 0;

	// Cancel the old timer, if still scheduled, and get the previous values.
	if (fTeam != NULL) {
		if (fScheduled) {
			cancel_timer(&fTimer);
			fScheduled = false;
		}

		_oldRemainingTime = fNextTime - now;
		_oldInterval = fInterval;

		fTeam->UserTimerDeactivated(this);
		fTeam->ReleaseReference();
		fTeam = NULL;
	} else {
		_oldRemainingTime = B_INFINITE_TIMEOUT;
		_oldInterval = 0;
	}

	// schedule the new timer
	fNextTime = nextTime;
	fInterval = interval;
	fOverrunCount = 0;

	if (fNextTime == B_INFINITE_TIMEOUT)
		return;

	// Get the team. If it doesn't exist anymore, just don't schedule the
	// timer anymore.
	fTeam = Team::Get(fTeamID);
	if (fTeam == NULL)
		return;

	fAbsolute = (flags & B_RELATIVE_TIMEOUT) == 0;

	// convert relative to absolute timeouts
	if (!fAbsolute) {
		if (!nowValid)
			now = fTeam->CPUTime(false);
		fNextTime += now;
	}

	fTeam->UserTimerActivated(this);

	// schedule/udpate the kernel timer
	Update(NULL);
}


void
TeamTimeUserTimer::GetInfo(bigtime_t& _remainingTime, bigtime_t& _interval,
	uint32& _overrunCount)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	if (fTeam != NULL) {
		_remainingTime = fNextTime - fTeam->CPUTime(false);
		_interval = fInterval;
	} else {
		_remainingTime = B_INFINITE_TIMEOUT;
		_interval = 0;
	}

	_overrunCount = fOverrunCount;
}


/*!	Deactivates the timer, if it is activated.

	The caller must hold the scheduler lock.
*/
void
TeamTimeUserTimer::Deactivate()
{
	if (fTeam == NULL)
		return;

	// unschedule, if scheduled
	if (fScheduled) {
		cancel_timer(&fTimer);
		fScheduled = false;
	}

	// deactivate
	fTeam->UserTimerDeactivated(this);
	fTeam->ReleaseReference();
	fTeam = NULL;
}


/*!	Starts/stops the timer as necessary, if it is active.

	Called whenever threads of the team whose CPU time is referred to by the
	timer are scheduled or unscheduled (or leave the team), or when the timer
	was just set. Schedules a kernel timer for the remaining time, respectively
	cancels it.

	The caller must hold the scheduler lock.

	\param unscheduledThread If not \c NULL, this is the thread that is
		currently running and which is in the process of being unscheduled.
*/
void
TeamTimeUserTimer::Update(Thread* unscheduledThread)
{
	if (fTeam == NULL)
		return;

	// determine how many of the team's threads are currently running
	fRunningThreads = 0;
	int32 cpuCount = smp_get_num_cpus();
	for (int32 i = 0; i < cpuCount; i++) {
		Thread* thread = gCPU[i].running_thread;
		if (thread != unscheduledThread && thread->team == fTeam)
			fRunningThreads++;
	}

	_Update(unscheduledThread != NULL);
}


/*!	Called when the team's CPU time clock which this timer refers to has been
	set.

	The caller must hold the scheduler lock.

	\param changedBy The value by which the clock has changed.
*/
void
TeamTimeUserTimer::TimeWarped(bigtime_t changedBy)
{
	if (fTeam == NULL || changedBy == 0)
		return;

	// If this is a relative timer, adjust fNextTime by the value the clock has
	// changed.
	if (!fAbsolute)
		fNextTime += changedBy;

	// reschedule the kernel timer
	_Update(false);
}


void
TeamTimeUserTimer::HandleTimer()
{
	UserTimer::HandleTimer();

	// If the timer is not periodic, it is no longer active. Otherwise
	// reschedule the kernel timer.
	if (fTeam != NULL) {
		if (fInterval == 0) {
			fTeam->UserTimerDeactivated(this);
			fTeam->ReleaseReference();
			fTeam = NULL;
		} else {
			UpdatePeriodicStartTime();
			_Update(false);
		}
	}
}


/*!	Schedules/cancels the kernel timer as necessary.

	\c fRunningThreads must be up-to-date.
	The caller must hold the scheduler lock.

	\param unscheduling \c true, when the current thread is in the process of
		being unscheduled.
*/
void
TeamTimeUserTimer::_Update(bool unscheduling)
{
	// unschedule the kernel timer, if scheduled
	if (fScheduled)
		cancel_timer(&fTimer);

	// if no more threads are running, we're done
	if (fRunningThreads == 0) {
		fScheduled = false;
		return;
	}

	// There are still threads running. Reschedule the kernel timer.
	bigtime_t now = fTeam->CPUTime(unscheduling);

	// If periodic, check whether the start time is too far in the past.
	if (fInterval > 0)
		CheckPeriodicOverrun(now);

	if (fNextTime > now) {
		fTimer.schedule_time = system_time()
			+ (fNextTime - now + fRunningThreads - 1) / fRunningThreads;
		// check for overflow
		if (fTimer.schedule_time < 0)
			fTimer.schedule_time = B_INFINITE_TIMEOUT;
	} else
		fTimer.schedule_time = 0;
	fTimer.period = 0;
		// We reschedule periodic timers manually in HandleTimer() to avoid
		// rounding errors.

	add_timer(&fTimer, &HandleTimerHook, fTimer.schedule_time,
		B_ONE_SHOT_ABSOLUTE_TIMER | B_TIMER_USE_TIMER_STRUCT_TIMES
			| B_TIMER_ACQUIRE_SCHEDULER_LOCK);
		// We use B_TIMER_ACQUIRE_SCHEDULER_LOCK to avoid race conditions
		// between setting/canceling the timer and the event handler.
		// We use B_TIMER_USE_TIMER_STRUCT_TIMES, so period remains 0, which
		// our base class expects.

	fScheduled = true;
}


// #pragma mark - TeamUserTimeUserTimer


TeamUserTimeUserTimer::TeamUserTimeUserTimer(team_id teamID)
	:
	fTeamID(teamID),
	fTeam(NULL)
{
}


TeamUserTimeUserTimer::~TeamUserTimeUserTimer()
{
	ASSERT(fTeam == NULL);
}


void
TeamUserTimeUserTimer::Schedule(bigtime_t nextTime, bigtime_t interval,
	uint32 flags, bigtime_t& _oldRemainingTime, bigtime_t& _oldInterval)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	// get the current time, but only if needed
	bool nowValid = fTeam != NULL;
	bigtime_t now = nowValid ? fTeam->UserCPUTime() : 0;

	// Cancel the old timer, if still active, and get the previous values.
	if (fTeam != NULL) {
		_oldRemainingTime = fNextTime - now;
		_oldInterval = fInterval;

		fTeam->UserTimerDeactivated(this);
		fTeam->ReleaseReference();
		fTeam = NULL;
	} else {
		_oldRemainingTime = B_INFINITE_TIMEOUT;
		_oldInterval = 0;
	}

	// schedule the new timer
	fNextTime = nextTime;
	fInterval = interval;
	fOverrunCount = 0;

	if (fNextTime == B_INFINITE_TIMEOUT)
		return;

	// Get the team. If it doesn't exist anymore, just don't schedule the
	// timer anymore.
	fTeam = Team::Get(fTeamID);
	if (fTeam == NULL)
		return;

	// convert relative to absolute timeouts
	if ((flags & B_RELATIVE_TIMEOUT) != 0) {
		if (!nowValid)
			now = fTeam->CPUTime(false);
		fNextTime += now;
	}

	fTeam->UserTimerActivated(this);

	// fire the event, if already timed out
	Check();
}


void
TeamUserTimeUserTimer::GetInfo(bigtime_t& _remainingTime, bigtime_t& _interval,
	uint32& _overrunCount)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	if (fTeam != NULL) {
		_remainingTime = fNextTime - fTeam->UserCPUTime();
		_interval = fInterval;
	} else {
		_remainingTime = B_INFINITE_TIMEOUT;
		_interval = 0;
	}

	_overrunCount = fOverrunCount;
}


/*!	Deactivates the timer, if it is activated.

	The caller must hold the scheduler lock.
*/
void
TeamUserTimeUserTimer::Deactivate()
{
	if (fTeam == NULL)
		return;

	// deactivate
	fTeam->UserTimerDeactivated(this);
	fTeam->ReleaseReference();
	fTeam = NULL;
}


/*!	Checks whether the timer is up, firing an event, if so.

	The caller must hold the scheduler lock.
*/
void
TeamUserTimeUserTimer::Check()
{
	if (fTeam == NULL)
		return;

	// check whether we need to fire the event yet
	bigtime_t now = fTeam->UserCPUTime();
	if (now < fNextTime)
		return;

	HandleTimer();

	// If the timer is not periodic, it is no longer active. Otherwise compute
	// the event time.
	if (fInterval == 0) {
		fTeam->UserTimerDeactivated(this);
		fTeam->ReleaseReference();
		fTeam = NULL;
		return;
	}

	// First validate fNextTime, then increment it, so that fNextTime is > now
	// (CheckPeriodicOverrun() only makes it > now - fInterval).
	CheckPeriodicOverrun(now);
	fNextTime += fInterval;
	fScheduled = true;
}


// #pragma mark - ThreadTimeUserTimer


ThreadTimeUserTimer::ThreadTimeUserTimer(thread_id threadID)
	:
	fThreadID(threadID),
	fThread(NULL)
{
}


ThreadTimeUserTimer::~ThreadTimeUserTimer()
{
	ASSERT(fThread == NULL);
}


void
ThreadTimeUserTimer::Schedule(bigtime_t nextTime, bigtime_t interval,
	uint32 flags, bigtime_t& _oldRemainingTime, bigtime_t& _oldInterval)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	// get the current time, but only if needed
	bool nowValid = fThread != NULL;
	bigtime_t now = nowValid ? fThread->CPUTime(false) : 0;

	// Cancel the old timer, if still scheduled, and get the previous values.
	if (fThread != NULL) {
		if (fScheduled) {
			cancel_timer(&fTimer);
			fScheduled = false;
		}

		_oldRemainingTime = fNextTime - now;
		_oldInterval = fInterval;

		fThread->UserTimerDeactivated(this);
		fThread->ReleaseReference();
		fThread = NULL;
	} else {
		_oldRemainingTime = B_INFINITE_TIMEOUT;
		_oldInterval = 0;
	}

	// schedule the new timer
	fNextTime = nextTime;
	fInterval = interval;
	fOverrunCount = 0;

	if (fNextTime == B_INFINITE_TIMEOUT)
		return;

	// Get the thread. If it doesn't exist anymore, just don't schedule the
	// timer anymore.
	fThread = Thread::Get(fThreadID);
	if (fThread == NULL)
		return;

	fAbsolute = (flags & B_RELATIVE_TIMEOUT) == 0;

	// convert relative to absolute timeouts
	if (!fAbsolute) {
		if (!nowValid)
			now = fThread->CPUTime(false);
		fNextTime += now;
	}

	fThread->UserTimerActivated(this);

	// If the thread is currently running, also schedule a kernel timer.
	if (fThread->cpu != NULL)
		Start();
}


void
ThreadTimeUserTimer::GetInfo(bigtime_t& _remainingTime, bigtime_t& _interval,
	uint32& _overrunCount)
{
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	if (fThread != NULL) {
		_remainingTime = fNextTime - fThread->CPUTime(false);
		_interval = fInterval;
	} else {
		_remainingTime = B_INFINITE_TIMEOUT;
		_interval = 0;
	}

	_overrunCount = fOverrunCount;
}


/*!	Deactivates the timer, if it is activated.

	The caller must hold the scheduler lock.
*/
void
ThreadTimeUserTimer::Deactivate()
{
	if (fThread == NULL)
		return;

	// unschedule, if scheduled
	if (fScheduled) {
		cancel_timer(&fTimer);
		fScheduled = false;
	}

	// deactivate
	fThread->UserTimerDeactivated(this);
	fThread->ReleaseReference();
	fThread = NULL;
}


/*!	Starts the timer, if it is active.

	Called when the thread whose CPU time is referred to by the timer is
	scheduled, or, when the timer was just set and the thread is already
	running. Schedules a kernel timer for the remaining time.

	The caller must hold the scheduler lock.
*/
void
ThreadTimeUserTimer::Start()
{
	if (fThread == NULL)
		return;

	ASSERT(!fScheduled);

	// add the kernel timer
	bigtime_t now = fThread->CPUTime(false);

	// If periodic, check whether the start time is too far in the past.
	if (fInterval > 0)
		CheckPeriodicOverrun(now);

	if (fNextTime > now) {
		fTimer.schedule_time = system_time() + fNextTime - now;
		// check for overflow
		if (fTimer.schedule_time < 0)
			fTimer.schedule_time = B_INFINITE_TIMEOUT;
	} else
		fTimer.schedule_time = 0;
	fTimer.period = 0;

	uint32 flags = B_ONE_SHOT_ABSOLUTE_TIMER
		| B_TIMER_USE_TIMER_STRUCT_TIMES | B_TIMER_ACQUIRE_SCHEDULER_LOCK;
		// We use B_TIMER_ACQUIRE_SCHEDULER_LOCK to avoid race conditions
		// between setting/canceling the timer and the event handler.

	add_timer(&fTimer, &HandleTimerHook, fTimer.schedule_time, flags);

	fScheduled = true;
}


/*!	Stops the timer, if it is active.

	Called when the thread whose CPU time is referred to by the timer is
	unscheduled, or, when the timer is canceled.

	The caller must hold the scheduler lock.
*/
void
ThreadTimeUserTimer::Stop()
{
	if (fThread == NULL)
		return;

	ASSERT(fScheduled);

	// cancel the kernel timer
	cancel_timer(&fTimer);
	fScheduled = false;

	// TODO: To avoid odd race conditions, we should check the current time of
	// the thread (ignoring the time since last_time) and manually fire the
	// user event, if necessary.
}


/*!	Called when the team's CPU time clock which this timer refers to has been
	set.

	The caller must hold the scheduler lock.

	\param changedBy The value by which the clock has changed.
*/
void
ThreadTimeUserTimer::TimeWarped(bigtime_t changedBy)
{
	if (fThread == NULL || changedBy == 0)
		return;

	// If this is a relative timer, adjust fNextTime by the value the clock has
	// changed.
	if (!fAbsolute)
		fNextTime += changedBy;

	// reschedule the kernel timer
	if (fScheduled) {
		Stop();
		Start();
	}
}


void
ThreadTimeUserTimer::HandleTimer()
{
	UserTimer::HandleTimer();

	if (fThread != NULL) {
		// If the timer is periodic, reschedule the kernel timer. Otherwise it
		// is no longer active.
		if (fInterval > 0) {
			UpdatePeriodicStartTime();
			Start();
		} else {
			fThread->UserTimerDeactivated(this);
			fThread->ReleaseReference();
			fThread = NULL;
		}
	}
}


// #pragma mark - UserTimerList


UserTimerList::UserTimerList()
{
}


UserTimerList::~UserTimerList()
{
	ASSERT(fTimers.IsEmpty());
}


/*!	Returns the user timer with the given ID.

	\param id The timer's ID
	\return The user timer with the given ID or \c NULL, if there is no such
		timer.
*/
UserTimer*
UserTimerList::TimerFor(int32 id) const
{
	// TODO: Use a more efficient data structure. E.g. a sorted array.
	for (TimerList::ConstIterator it = fTimers.GetIterator();
			UserTimer* timer = it.Next();) {
		if (timer->ID() == id)
			return timer;
	}

	return NULL;
}


/*!	Adds the given user timer and assigns it an ID.

	\param timer The timer to be added.
*/
void
UserTimerList::AddTimer(UserTimer* timer)
{
	int32 id = timer->ID();
	if (id < 0) {
		// user-defined timer -- find an usused ID
		id = USER_TIMER_FIRST_USER_DEFINED_ID;
		UserTimer* insertAfter = NULL;
		for (TimerList::Iterator it = fTimers.GetIterator();
				UserTimer* other = it.Next();) {
			if (other->ID() > id)
				break;
			if (other->ID() == id)
				id++;
			insertAfter = other;
		}

		// insert the timer
		timer->SetID(id);
		fTimers.InsertAfter(insertAfter, timer);
	} else {
		// default timer -- find the insertion point
		UserTimer* insertAfter = NULL;
		for (TimerList::Iterator it = fTimers.GetIterator();
				UserTimer* other = it.Next();) {
			if (other->ID() > id)
				break;
			if (other->ID() == id) {
				panic("UserTimerList::AddTimer(): timer with ID %" B_PRId32
					" already exists!", id);
			}
			insertAfter = other;
		}

		// insert the timer
		fTimers.InsertAfter(insertAfter, timer);
	}
}


/*!	Deletes all (or all user-defined) user timers.

	\param userDefinedOnly If \c true, only the user-defined timers are deleted,
		otherwise all timers are deleted.
	\return The number of user-defined timers that were removed and deleted.
*/
int32
UserTimerList::DeleteTimers(bool userDefinedOnly)
{
	int32 userDefinedCount = 0;

	for (TimerList::Iterator it = fTimers.GetIterator();
			UserTimer* timer = it.Next();) {
		if (timer->ID() < USER_TIMER_FIRST_USER_DEFINED_ID) {
			if (userDefinedOnly)
				continue;
		} else
			userDefinedCount++;

		// remove, cancel, and delete the timer
		it.Remove();
		timer->Cancel();
		delete timer;
	}

	return userDefinedCount;
}


// #pragma mark - private


static int32
create_timer(clockid_t clockID, int32 timerID, Team* team, Thread* thread,
	uint32 flags, const struct sigevent& event,
	ThreadCreationAttributes* threadAttributes, bool isDefaultEvent)
{
	// create the timer object
	UserTimer* timer;
	switch (clockID) {
		case CLOCK_MONOTONIC:
			timer = new(std::nothrow) SystemTimeUserTimer;
			break;

		case CLOCK_REALTIME:
			timer = new(std::nothrow) RealTimeUserTimer;
			break;

		case CLOCK_THREAD_CPUTIME_ID:
			timer = new(std::nothrow) ThreadTimeUserTimer(
				thread_get_current_thread()->id);
			break;

		case CLOCK_PROCESS_CPUTIME_ID:
			if (team == NULL)
				return B_BAD_VALUE;
			timer = new(std::nothrow) TeamTimeUserTimer(team->id);
			break;

		case CLOCK_PROCESS_USER_CPUTIME_ID:
			if (team == NULL)
				return B_BAD_VALUE;
			timer = new(std::nothrow) TeamUserTimeUserTimer(team->id);
			break;

		default:
		{
			// The clock ID is a ID of the team whose CPU time the clock refers
			// to. Check whether the team exists and we have permission to
			// access its clock.
			if (clockID <= 0)
				return B_BAD_VALUE;
			if (clockID == team_get_kernel_team_id())
				return B_NOT_ALLOWED;

			Team* timedTeam = Team::GetAndLock(clockID);
			if (timedTeam == NULL)
				return B_BAD_VALUE;

			uid_t uid = geteuid();
			uid_t teamUID = timedTeam->effective_uid;

			timedTeam->UnlockAndReleaseReference();

			if (uid != 0 && uid != teamUID)
				return B_NOT_ALLOWED;

			timer = new(std::nothrow) TeamTimeUserTimer(clockID);
			break;
		}
	}

	if (timer == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<UserTimer> timerDeleter(timer);

	if (timerID >= 0)
		timer->SetID(timerID);

	SignalEvent* signalEvent = NULL;

	switch (event.sigev_notify) {
		case SIGEV_NONE:
			// the timer's event remains NULL
			break;

		case SIGEV_SIGNAL:
		{
			if (event.sigev_signo <= 0 || event.sigev_signo > MAX_SIGNAL_NUMBER)
				return B_BAD_VALUE;

			if (thread != NULL && (flags & USER_TIMER_SIGNAL_THREAD) != 0) {
				// The signal shall be sent to the thread.
				signalEvent = ThreadSignalEvent::Create(thread,
					event.sigev_signo, SI_TIMER, 0, team->id);
			} else {
				// The signal shall be sent to the team.
				signalEvent = TeamSignalEvent::Create(team, event.sigev_signo,
					SI_TIMER, 0);
			}

			if (signalEvent == NULL)
				return B_NO_MEMORY;

			timer->SetEvent(signalEvent);
			break;
		}

		case SIGEV_THREAD:
		{
			if (threadAttributes == NULL)
				return B_BAD_VALUE;

			CreateThreadEvent* event
				= CreateThreadEvent::Create(*threadAttributes);
			if (event == NULL)
				return B_NO_MEMORY;

			timer->SetEvent(event);
			break;
		}

		default:
			return B_BAD_VALUE;
	}

	// add it to the team/thread
	TimerLocker timerLocker;
	timerLocker.Lock(team, thread);

	status_t error = thread != NULL
		? thread->AddUserTimer(timer) : team->AddUserTimer(timer);
	if (error != B_OK)
		return error;

	// set a signal event's user value
	if (signalEvent != NULL) {
		// If no sigevent structure was given, use the timer ID.
		union sigval signalValue = event.sigev_value;
		if (isDefaultEvent)
			signalValue.sival_int = timer->ID();

		signalEvent->SetUserValue(signalValue);
	}

	return timerDeleter.Detach()->ID();
}


/*!	Called when the CPU time clock of the given thread has been set.

	The caller must hold the scheduler lock.

	\param thread The thread whose CPU time clock has been set.
	\param changedBy The value by which the CPU time clock has changed
		(new = old + changedBy).
*/
static void
thread_clock_changed(Thread* thread, bigtime_t changedBy)
{
	for (ThreadTimeUserTimerList::ConstIterator it
				= thread->CPUTimeUserTimerIterator();
			ThreadTimeUserTimer* timer = it.Next();) {
		timer->TimeWarped(changedBy);
	}
}


/*!	Called when the CPU time clock of the given team has been set.

	The caller must hold the scheduler lock.

	\param team The team whose CPU time clock has been set.
	\param changedBy The value by which the CPU time clock has changed
		(new = old + changedBy).
*/
static void
team_clock_changed(Team* team, bigtime_t changedBy)
{
	for (TeamTimeUserTimerList::ConstIterator it
				= team->CPUTimeUserTimerIterator();
			TeamTimeUserTimer* timer = it.Next();) {
		timer->TimeWarped(changedBy);
	}
}


// #pragma mark - kernel private


/*!	Creates the pre-defined user timers for the given thread.
	The thread may not have been added to its team yet, hence the team must be
	passed

	\param team The thread's (future) team.
	\param thread The thread whose pre-defined timers shall be created.
	\return \c B_OK, when everything when fine, another error code otherwise.
*/
status_t
user_timer_create_thread_timers(Team* team, Thread* thread)
{
	// create a real time user timer
	struct sigevent event;
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGALRM;

	int32 timerID = create_timer(CLOCK_MONOTONIC, USER_TIMER_REAL_TIME_ID,
		team, thread, USER_TIMER_SIGNAL_THREAD, event, NULL, true);
	if (timerID < 0)
		return timerID;

	return B_OK;
}


/*!	Creates the pre-defined user timers for the given team.

	\param team The team whose pre-defined timers shall be created.
	\return \c B_OK, when everything when fine, another error code otherwise.
*/
status_t
user_timer_create_team_timers(Team* team)
{
	// create a real time user timer
	struct sigevent event;
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGALRM;

	int32 timerID = create_timer(CLOCK_MONOTONIC, USER_TIMER_REAL_TIME_ID,
		team, NULL, 0, event, NULL, true);
	if (timerID < 0)
		return timerID;

	// create a total CPU time user timer
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGPROF;

	timerID = create_timer(CLOCK_PROCESS_CPUTIME_ID,
		USER_TIMER_TEAM_TOTAL_TIME_ID, team, NULL, 0, event, NULL, true);
	if (timerID < 0)
		return timerID;

	// create a user CPU time user timer
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGVTALRM;

	timerID = create_timer(CLOCK_PROCESS_USER_CPUTIME_ID,
		USER_TIMER_TEAM_USER_TIME_ID, team, NULL, 0, event, NULL, true);
	if (timerID < 0)
		return timerID;

	return B_OK;
}


status_t
user_timer_get_clock(clockid_t clockID, bigtime_t& _time)
{
	switch (clockID) {
		case CLOCK_MONOTONIC:
			_time = system_time();
			return B_OK;

		case CLOCK_REALTIME:
			_time = real_time_clock_usecs();
			return B_OK;

		case CLOCK_THREAD_CPUTIME_ID:
		{
			Thread* thread = thread_get_current_thread();
			InterruptsSpinLocker schedulerLocker(gSchedulerLock);
			_time = thread->CPUTime(false);
			return B_OK;
		}

		case CLOCK_PROCESS_USER_CPUTIME_ID:
		{
			Team* team = thread_get_current_thread()->team;
			InterruptsSpinLocker schedulerLocker(gSchedulerLock);
			_time = team->UserCPUTime();
			return B_OK;
		}

		case CLOCK_PROCESS_CPUTIME_ID:
		default:
		{
			// get the ID of the target team (or the respective placeholder)
			team_id teamID;
			if (clockID == CLOCK_PROCESS_CPUTIME_ID) {
				teamID = B_CURRENT_TEAM;
			} else {
				if (clockID < 0)
					return B_BAD_VALUE;
				if (clockID == team_get_kernel_team_id())
					return B_NOT_ALLOWED;

				teamID = clockID;
			}

			// get the team
			Team* team = Team::Get(teamID);
			if (team == NULL)
				return B_BAD_VALUE;
			BReference<Team> teamReference(team, true);

			// get the time
			InterruptsSpinLocker schedulerLocker(gSchedulerLock);
			_time = team->CPUTime(false);

			return B_OK;
		}
	}
}


void
user_timer_real_time_clock_changed()
{
	// we need to update all absolute real-time timers
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);
	SpinLocker globalListLocker(sAbsoluteRealTimeTimersLock);

	for (RealTimeUserTimerList::Iterator it
				= sAbsoluteRealTimeTimers.GetIterator();
			RealTimeUserTimer* timer = it.Next();) {
		timer->TimeWarped();
	}
}


void
user_timer_stop_cpu_timers(Thread* thread, Thread* nextThread)
{
	// stop thread timers
	for (ThreadTimeUserTimerList::ConstIterator it
				= thread->CPUTimeUserTimerIterator();
			ThreadTimeUserTimer* timer = it.Next();) {
		timer->Stop();
	}

	// update team timers
	if (nextThread == NULL || nextThread->team != thread->team) {
		for (TeamTimeUserTimerList::ConstIterator it
					= thread->team->CPUTimeUserTimerIterator();
				TeamTimeUserTimer* timer = it.Next();) {
			timer->Update(thread);
		}
	}
}


void
user_timer_continue_cpu_timers(Thread* thread, Thread* previousThread)
{
	// update team timers
	if (previousThread == NULL || previousThread->team != thread->team) {
		for (TeamTimeUserTimerList::ConstIterator it
					= thread->team->CPUTimeUserTimerIterator();
				TeamTimeUserTimer* timer = it.Next();) {
			timer->Update(NULL);
		}
	}

	// start thread timers
	for (ThreadTimeUserTimerList::ConstIterator it
				= thread->CPUTimeUserTimerIterator();
			ThreadTimeUserTimer* timer = it.Next();) {
		timer->Start();
	}
}


void
user_timer_check_team_user_timers(Team* team)
{
	for (TeamUserTimeUserTimerList::ConstIterator it
				= team->UserTimeUserTimerIterator();
			TeamUserTimeUserTimer* timer = it.Next();) {
		timer->Check();
	}
}


// #pragma mark - syscalls


status_t
_user_get_clock(clockid_t clockID, bigtime_t* userTime)
{
	// get the time
	bigtime_t time;
	status_t error = user_timer_get_clock(clockID, time);
	if (error != B_OK)
		return error;

	// copy the value back to userland
	if (userTime == NULL || !IS_USER_ADDRESS(userTime)
		|| user_memcpy(userTime, &time, sizeof(time)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
_user_set_clock(clockid_t clockID, bigtime_t time)
{
	switch (clockID) {
		case CLOCK_MONOTONIC:
			return B_BAD_VALUE;

		case CLOCK_REALTIME:
			// only root may set the time
			if (geteuid() != 0)
				return B_NOT_ALLOWED;

			set_real_time_clock_usecs(time);
			return B_OK;

		case CLOCK_THREAD_CPUTIME_ID:
		{
			Thread* thread = thread_get_current_thread();
			InterruptsSpinLocker schedulerLocker(gSchedulerLock);
			bigtime_t diff = time - thread->CPUTime(false);
			thread->cpu_clock_offset += diff;

			thread_clock_changed(thread, diff);
			return B_OK;
		}

		case CLOCK_PROCESS_USER_CPUTIME_ID:
			// not supported -- this clock is an Haiku-internal extension
			return B_BAD_VALUE;

		case CLOCK_PROCESS_CPUTIME_ID:
		default:
		{
			// get the ID of the target team (or the respective placeholder)
			team_id teamID;
			if (clockID == CLOCK_PROCESS_CPUTIME_ID) {
				teamID = B_CURRENT_TEAM;
			} else {
				if (clockID < 0)
					return B_BAD_VALUE;
				if (clockID == team_get_kernel_team_id())
					return B_NOT_ALLOWED;

				teamID = clockID;
			}

			// get the team
			Team* team = Team::Get(teamID);
			if (team == NULL)
				return B_BAD_VALUE;
			BReference<Team> teamReference(team, true);

			// set the time offset
			InterruptsSpinLocker schedulerLocker(gSchedulerLock);
			bigtime_t diff = time - team->CPUTime(false);
			team->cpu_clock_offset += diff;

			team_clock_changed(team, diff);
			return B_OK;
		}
	}

	return B_OK;
}


int32
_user_create_timer(clockid_t clockID, thread_id threadID, uint32 flags,
	const struct sigevent* userEvent,
	const thread_creation_attributes* userThreadAttributes)
{
	// copy the sigevent structure from userland
	struct sigevent event;
	if (userEvent != NULL) {
		if (!IS_USER_ADDRESS(userEvent)
			|| user_memcpy(&event, userEvent, sizeof(event)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	} else {
		// none given -- use defaults
		event.sigev_notify = SIGEV_SIGNAL;
		event.sigev_signo = SIGALRM;
	}

	// copy thread creation attributes from userland, if specified
	char nameBuffer[B_OS_NAME_LENGTH];
	ThreadCreationAttributes threadAttributes;
	if (event.sigev_notify == SIGEV_THREAD) {
		status_t error = threadAttributes.InitFromUserAttributes(
			userThreadAttributes, nameBuffer);
		if (error != B_OK)
			return error;
	}

	// get team and thread
	Team* team = thread_get_current_thread()->team;
	Thread* thread = NULL;
	if (threadID >= 0) {
		thread = Thread::GetAndLock(threadID);
		if (thread == NULL)
			return B_BAD_THREAD_ID;

		thread->Unlock();
	}
	BReference<Thread> threadReference(thread, true);

	// create the timer
	return create_timer(clockID, -1, team, thread, flags, event,
		userThreadAttributes != NULL ? &threadAttributes : NULL,
		userEvent == NULL);
}


status_t
_user_delete_timer(int32 timerID, thread_id threadID)
{
	// can only delete user-defined timers
	if (timerID < USER_TIMER_FIRST_USER_DEFINED_ID)
		return B_BAD_VALUE;

	// get the timer
	TimerLocker timerLocker;
	UserTimer* timer;
	status_t error = timerLocker.LockAndGetTimer(threadID, timerID, timer);
	if (error != B_OK)
		return error;

	// cancel, remove, and delete it
	timer->Cancel();

	if (threadID >= 0)
		timerLocker.thread->RemoveUserTimer(timer);
	else
		timerLocker.team->RemoveUserTimer(timer);

	delete timer;

	return B_OK;
}


status_t
_user_get_timer(int32 timerID, thread_id threadID,
	struct user_timer_info* userInfo)
{
	// get the timer
	TimerLocker timerLocker;
	UserTimer* timer;
	status_t error = timerLocker.LockAndGetTimer(threadID, timerID, timer);
	if (error != B_OK)
		return error;

	// get the info
	user_timer_info info;
	timer->GetInfo(info.remaining_time, info.interval, info.overrun_count);

	// Sanitize remaining_time. If it's <= 0, we set it to 1, the least valid
	// value.
	if (info.remaining_time <= 0)
		info.remaining_time = 1;

	timerLocker.Unlock();

	// copy it back to userland
	if (userInfo != NULL
		&& (!IS_USER_ADDRESS(userInfo)
			|| user_memcpy(userInfo, &info, sizeof(info)) != B_OK)) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
_user_set_timer(int32 timerID, thread_id threadID, bigtime_t startTime,
	bigtime_t interval, uint32 flags, struct user_timer_info* userOldInfo)
{
	// check the values
	if (startTime < 0 || interval < 0)
		return B_BAD_VALUE;

	// get the timer
	TimerLocker timerLocker;
	UserTimer* timer;
	status_t error = timerLocker.LockAndGetTimer(threadID, timerID, timer);
	if (error != B_OK)
		return error;

	// schedule the timer
	user_timer_info oldInfo;
	timer->Schedule(startTime, interval, flags, oldInfo.remaining_time,
		oldInfo.interval);

	// Sanitize remaining_time. If it's <= 0, we set it to 1, the least valid
	// value.
	if (oldInfo.remaining_time <= 0)
		oldInfo.remaining_time = 1;

	timerLocker.Unlock();

	// copy back the old info
	if (userOldInfo != NULL
		&& (!IS_USER_ADDRESS(userOldInfo)
			|| user_memcpy(userOldInfo, &oldInfo, sizeof(oldInfo)) != B_OK)) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}
