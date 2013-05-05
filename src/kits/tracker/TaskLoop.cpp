/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Debug.h>
#include <InterfaceDefs.h>

#include "AutoLock.h"
#include "TaskLoop.h"


DelayedTask::DelayedTask(bigtime_t delay)
	:	fRunAfter(system_time() + delay)
{
}

DelayedTask::~DelayedTask()
{
}

OneShotDelayedTask::OneShotDelayedTask(FunctionObject* functor,
	bigtime_t delay)
	:	DelayedTask(delay),
		fFunctor(functor)
{
}


OneShotDelayedTask::~OneShotDelayedTask()
{
	delete fFunctor;
}


bool
OneShotDelayedTask::RunIfNeeded(bigtime_t currentTime)
{
	if (currentTime < fRunAfter)
		return false;

	(*fFunctor)();
	return true;
}


PeriodicDelayedTask::PeriodicDelayedTask(
	FunctionObjectWithResult<bool>* functor, bigtime_t initialDelay,
	bigtime_t period)
	:	DelayedTask(initialDelay),
		fPeriod(period),
		fFunctor(functor)
{
}


PeriodicDelayedTask::~PeriodicDelayedTask()
{
	delete fFunctor;
}


bool
PeriodicDelayedTask::RunIfNeeded(bigtime_t currentTime)
{
	if (!currentTime < fRunAfter)
		return false;

	fRunAfter = currentTime + fPeriod;
	(*fFunctor)();
	return fFunctor->Result();
}


PeriodicDelayedTaskWithTimeout::PeriodicDelayedTaskWithTimeout(
	FunctionObjectWithResult<bool>* functor, bigtime_t initialDelay,
	bigtime_t period, bigtime_t timeout)
	:	PeriodicDelayedTask(functor, initialDelay, period),
		fTimeoutAfter(system_time() + timeout)
{
}


bool
PeriodicDelayedTaskWithTimeout::RunIfNeeded(bigtime_t currentTime)
{
	if (currentTime < fRunAfter)
		return false;

	fRunAfter = currentTime + fPeriod;
	(*fFunctor)();
	if (fFunctor->Result())
		return true;

	// if call didn't terminate the task yet, check if timeout is due
	return currentTime > fTimeoutAfter;
}


RunWhenIdleTask::RunWhenIdleTask(FunctionObjectWithResult<bool>* functor,
	bigtime_t initialDelay, bigtime_t idleFor, bigtime_t heartBeat)
	:	PeriodicDelayedTask(functor, initialDelay, heartBeat),
		fIdleFor(idleFor),
		fState(kInitialDelay)
{
}


RunWhenIdleTask::~RunWhenIdleTask()
{
}


bool
RunWhenIdleTask::RunIfNeeded(bigtime_t currentTime)
{
	if (currentTime < fRunAfter)
		return false;

	fRunAfter = currentTime + fPeriod;
	// PRINT(("runWhenIdle: runAfter %Ld, current time %Ld, period %Ld\n",
	//	fRunAfter, currentTime, fPeriod));

	if (fState == kInitialDelay) {
//		PRINT(("run when idle task - past intial delay\n"));
		ResetIdleTimer(currentTime);
	} else if (fState == kInIdleState && !StillIdle(currentTime)) {
		fState = kInitialIdleWait;
		ResetIdleTimer(currentTime);
	} else if (fState != kInitialIdleWait || IdleTimerExpired(currentTime)) {
		fState = kInIdleState;
		(*fFunctor)();
		return fFunctor->Result();
	}
	return false;
}


static bigtime_t
ActivityLevel()
{
	// stolen from roster server
	bigtime_t time = 0;
	system_info	sinfo;
	get_system_info(&sinfo);
	for (int32 index = 0; index < sinfo.cpu_count; index++)
		time += sinfo.cpu_infos[index].active_time;
	return time / ((bigtime_t) sinfo.cpu_count);
}


void
RunWhenIdleTask::ResetIdleTimer(bigtime_t currentTime)
{
	fActivityLevel = ActivityLevel();
	fActivityLevelStart = currentTime;
	fLastCPUTooBusyTime = currentTime;
	fState = kInitialIdleWait;
}

const float kTaskOverhead = 0.01f;
	// this should really be specified by the task itself
const float kIdleTreshold = 0.15f;

bool
RunWhenIdleTask::IsIdle(bigtime_t currentTime, float taskOverhead)
{
	bigtime_t currentActivityLevel = ActivityLevel();
	float load = (float)(currentActivityLevel - fActivityLevel)
		/ (float)(currentTime - fActivityLevelStart);

	fActivityLevel = currentActivityLevel;
	fActivityLevelStart = currentTime;

	load -= taskOverhead;

	bool idle = true;

	if (load > kIdleTreshold) {
//		PRINT(("not idle enough %f\n", load));
		idle = false;
	} else if ((currentTime - fLastCPUTooBusyTime) < fIdleFor
		|| idle_time() < fIdleFor) {
//		PRINT(("load %f, not idle long enough %Ld, %Ld\n", load,
//			currentTime - fLastCPUTooBusyTime,
//			idle_time()));
		idle = false;
	}

#if xDEBUG
	else
		PRINT(("load %f, idle for %Ld sec, go\n", load,
			(currentTime - fLastCPUTooBusyTime) / 1000000));
#endif

	return idle;
}


bool
RunWhenIdleTask::IdleTimerExpired(bigtime_t currentTime)
{
	return IsIdle(currentTime, 0);
}


bool
RunWhenIdleTask::StillIdle(bigtime_t currentTime)
{
	return IsIdle(currentTime, kIdleTreshold);
}


TaskLoop::TaskLoop(bigtime_t heartBeat)
	:	fTaskList(10, true),
		fHeartBeat(heartBeat)
{
}


TaskLoop::~TaskLoop()
{
}


void
TaskLoop::RunLater(DelayedTask* task)
{
	AddTask(task);
}


void
TaskLoop::RunLater(FunctionObject* functor, bigtime_t delay)
{
	RunLater(new OneShotDelayedTask(functor, delay));
}


void
TaskLoop::RunLater(FunctionObjectWithResult<bool>* functor,
	bigtime_t delay, bigtime_t period)
{
	RunLater(new PeriodicDelayedTask(functor, delay, period));
}


void
TaskLoop::RunLater(FunctionObjectWithResult<bool>* functor, bigtime_t delay,
	bigtime_t period, bigtime_t timeout)
{
	RunLater(new PeriodicDelayedTaskWithTimeout(functor, delay, period,
		timeout));
}


void
TaskLoop::RunWhenIdle(FunctionObjectWithResult<bool>* functor,
	bigtime_t initialDelay, bigtime_t idleTime, bigtime_t heartBeat)
{
	RunLater(new RunWhenIdleTask(functor, initialDelay, idleTime, heartBeat));
}


class AccumulatedOneShotDelayedTask : public OneShotDelayedTask {
	// supports accumulating functors
public:
	AccumulatedOneShotDelayedTask(AccumulatingFunctionObject* functor,
		bigtime_t delay, bigtime_t maxAccumulatingTime = 0,
		int32 maxAccumulateCount = 0)
		:	OneShotDelayedTask(functor, delay),
			maxAccumulateCount(maxAccumulateCount),
			accumulateCount(1),
			maxAccumulatingTime(maxAccumulatingTime),
			initialTime(system_time())
		{}

	bool CanAccumulate(const AccumulatingFunctionObject* accumulateThis) const
		{
			if (maxAccumulateCount && accumulateCount > maxAccumulateCount)
				// don't accumulate if too may accumulated already
				return false;

			if (maxAccumulatingTime && system_time() > initialTime
					+ maxAccumulatingTime) {
				// don't accumulate if too late past initial task
				return false;
			}

			return static_cast<AccumulatingFunctionObject*>(fFunctor)->
				CanAccumulate(accumulateThis);
		}

	virtual void Accumulate(AccumulatingFunctionObject* accumulateThis,
		bigtime_t delay)
		{
			fRunAfter = system_time() + delay;
				// reset fRunAfter
			accumulateCount++;
			static_cast<AccumulatingFunctionObject*>(fFunctor)->
				Accumulate(accumulateThis);
		}

private:
	int32 maxAccumulateCount;
	int32 accumulateCount;
	bigtime_t maxAccumulatingTime;
	bigtime_t initialTime;
};

void
TaskLoop::AccumulatedRunLater(AccumulatingFunctionObject* functor,
	bigtime_t delay, bigtime_t maxAccumulatingTime, int32 maxAccumulateCount)
{
	AutoLock<BLocker> autoLock(&fLock);
	if (!autoLock.IsLocked()) {
		return;
	}
	int32 count = fTaskList.CountItems();
	for (int32 index = 0; index < count; index++) {
		AccumulatedOneShotDelayedTask* task
			= dynamic_cast<AccumulatedOneShotDelayedTask*>(
				fTaskList.ItemAt(index));
		if (!task)
			continue;

		if (task->CanAccumulate(functor)) {
			task->Accumulate(functor, delay);
			return;
		}
	}
	RunLater(new AccumulatedOneShotDelayedTask(functor, delay,
		maxAccumulatingTime, maxAccumulateCount));
}


bool
TaskLoop::Pulse()
{
	ASSERT(fLock.IsLocked());

	int32 count = fTaskList.CountItems();
	if (count > 0) {
		bigtime_t currentTime = system_time();
		for (int32 index = 0; index < count; ) {
			DelayedTask* task = fTaskList.ItemAt(index);
			// give every task a try
			if (task->RunIfNeeded(currentTime)) {
				// if done, remove from list
				RemoveTask(task);
				count--;
			} else
				index++;
		}
	}
	return count == 0 && !KeepPulsingWhenEmpty();
}

const bigtime_t kInfinity = B_INFINITE_TIMEOUT;

bigtime_t
TaskLoop::LatestRunTime() const
{
	ASSERT(fLock.IsLocked());
	bigtime_t result = kInfinity;

#if xDEBUG
	DelayedTask* nextTask = 0;
#endif
	int32 count = fTaskList.CountItems();
	for (int32 index = 0; index < count; index++) {
		bigtime_t runAfter = fTaskList.ItemAt(index)->RunAfterTime();
		if (runAfter < result) {
			result = runAfter;

#if xDEBUG
			nextTask = fTaskList.ItemAt(index);
#endif
		}
	}


#if xDEBUG
	if (nextTask)
		PRINT(("latestRunTime : next task %s\n", typeid(*nextTask).name));
	else
		PRINT(("latestRunTime : no next task\n"));
#endif

	return result;
}


void
TaskLoop::RemoveTask(DelayedTask* task)
{
	ASSERT(fLock.IsLocked());
	// remove the task
	fTaskList.RemoveItem(task);
}

void
TaskLoop::AddTask(DelayedTask* task)
{
	AutoLock<BLocker> autoLock(&fLock);
	if (!autoLock.IsLocked()) {
		delete task;
		return;
	}

	fTaskList.AddItem(task);
	StartPulsingIfNeeded();
}


StandAloneTaskLoop::StandAloneTaskLoop(bool keepThread, bigtime_t heartBeat)
	:	TaskLoop(heartBeat),
		fNeedToQuit(false),
		fScanThread(-1),
		fKeepThread(keepThread)
{
}


StandAloneTaskLoop::~StandAloneTaskLoop()
{
	fLock.Lock();
	fNeedToQuit = true;
	bool easyOut = (fScanThread == -1);
	fLock.Unlock();

	if (!easyOut)
		for (int32 timeout = 10000; ; timeout--) {
			// use a 10 sec timeout value in case the spawned
			// thread is stuck somewhere

			if (!timeout) {
				PRINT(("StandAloneTaskLoop timed out, quitting abruptly"));
				break;
			}

			bool done;

			fLock.Lock();
			done = (fScanThread == -1);
			fLock.Unlock();
			if (done)
				break;

			snooze(1000);
		}
}

void
StandAloneTaskLoop::StartPulsingIfNeeded()
{
	ASSERT(fLock.IsLocked());
	if (fScanThread < 0) {
		// no loop thread yet, spawn one
		fScanThread = spawn_thread(StandAloneTaskLoop::RunBinder,
			"TrackerTaskLoop", B_LOW_PRIORITY, this);
		resume_thread(fScanThread);
	}
}

bool
StandAloneTaskLoop::KeepPulsingWhenEmpty() const
{
	return fKeepThread;
}

status_t
StandAloneTaskLoop::RunBinder(void* castToThis)
{
	StandAloneTaskLoop* self = (StandAloneTaskLoop*)castToThis;
	self->Run();
	return B_OK;
}

void
StandAloneTaskLoop::Run()
{
	for(;;) {
		AutoLock<BLocker> autoLock(&fLock);
		if (!autoLock)
			return;

		if (fNeedToQuit) {
			// task loop being deleted, let go of the thread allowing the
			// to go through deletion
			fScanThread = -1;
			return;
		}

		if (Pulse()) {
			fScanThread = -1;
			return;
		}

		// figure out when to run next by checking out when the different
		// tasks wan't to be woken up, snooze until a little bit before that
		// time
		bigtime_t now = system_time();
		bigtime_t latestRunTime = LatestRunTime() - 1000;
		bigtime_t afterHeartBeatTime = now + fHeartBeat;
		bigtime_t snoozeTill = latestRunTime < afterHeartBeatTime ?
			latestRunTime : afterHeartBeatTime;

		autoLock.Unlock();

		if (snoozeTill > now)
			snooze_until(snoozeTill, B_SYSTEM_TIMEBASE);
		else
			snooze(1000);
	}
}

void
StandAloneTaskLoop::AddTask(DelayedTask* delayedTask)
{
	_inherited::AddTask(delayedTask);
	if (fScanThread < 0)
		return;

	// wake up the loop thread if it is asleep
	thread_info info;
	get_thread_info(fScanThread, &info);
	if (info.state == B_THREAD_ASLEEP) {
		suspend_thread(fScanThread);
		snooze(1000);	// snooze because BeBook sez so
		resume_thread(fScanThread);
	}
}

PiggybackTaskLoop::PiggybackTaskLoop(bigtime_t heartBeat)
	:	TaskLoop(heartBeat),
		fNextHeartBeatTime(0),
		fPulseMe(false)
{
}


PiggybackTaskLoop::~PiggybackTaskLoop()
{
}

void
PiggybackTaskLoop::PulseMe()
{
	if (!fPulseMe)
		return;

	bigtime_t time = system_time();
	if (fNextHeartBeatTime < time) {
		AutoLock<BLocker> autoLock(&fLock);
		if (Pulse())
			fPulseMe = false;
		fNextHeartBeatTime = time + fHeartBeat;
	}
}

bool
PiggybackTaskLoop::KeepPulsingWhenEmpty() const
{
	return false;
}

void
PiggybackTaskLoop::StartPulsingIfNeeded()
{
	fPulseMe = true;
}
