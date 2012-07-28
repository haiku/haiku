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
#ifndef __TASK_LOOP__
#define __TASK_LOOP__


//	Delayed Tasks, Periodic Delayed Tasks, Periodic Delayed Tasks with timeout,
//	Run when idle tasks, accumulating delayed tasks


#include <Locker.h>

#include "FunctionObject.h"
#include "ObjectList.h"


namespace BPrivate {


// Task flavors

class DelayedTask {
public:
	DelayedTask(bigtime_t delay);
	virtual ~DelayedTask();

	virtual bool RunIfNeeded(bigtime_t currentTime) = 0;
		// returns true if done and should not be called again

	bigtime_t RunAfterTime() const;

protected:
	bigtime_t fRunAfter;
};


// called once after a specified delay
class OneShotDelayedTask : public DelayedTask {
public:
	OneShotDelayedTask(FunctionObject* functor, bigtime_t delay);
	virtual ~OneShotDelayedTask();
	
	virtual bool RunIfNeeded(bigtime_t currentTime);

protected:
	FunctionObject* fFunctor;
};


// called periodically till functor return true
class PeriodicDelayedTask : public DelayedTask {
public:
	PeriodicDelayedTask(FunctionObjectWithResult<bool>* functor,
		bigtime_t initialDelay, bigtime_t period);
	virtual ~PeriodicDelayedTask();

	virtual bool RunIfNeeded(bigtime_t currentTime);

protected:
	bigtime_t fPeriod;
	FunctionObjectWithResult<bool>* fFunctor;
};


// called periodically till functor returns true or till time out
class PeriodicDelayedTaskWithTimeout : public PeriodicDelayedTask {
public:
	PeriodicDelayedTaskWithTimeout(FunctionObjectWithResult<bool>* functor,
		bigtime_t initialDelay, bigtime_t period, bigtime_t timeout);

	virtual bool RunIfNeeded(bigtime_t currentTime);

protected:
	bigtime_t fTimeoutAfter;
};


// after initial delay starts periodically calling functor if system is idle
// until functor returns true
class RunWhenIdleTask : public PeriodicDelayedTask {
public:
	RunWhenIdleTask(FunctionObjectWithResult<bool>* functor, bigtime_t initialDelay,
		bigtime_t idleFor, bigtime_t heartBeat);
	virtual ~RunWhenIdleTask();

	virtual bool RunIfNeeded(bigtime_t currentTime);

protected:
	void ResetIdleTimer(bigtime_t currentTime);
	bool IdleTimerExpired(bigtime_t currentTime);
	bool StillIdle(bigtime_t currentTime);
	bool IsIdle(bigtime_t currentTime, float taskOverhead);

	bigtime_t fIdleFor;

	enum State {
		kInitialDelay,
		kInitialIdleWait,
		kInIdleState
	};

	State fState;
	bigtime_t fActivityLevelStart;
	bigtime_t fActivityLevel;
	bigtime_t fLastCPUTooBusyTime;

private:
	typedef PeriodicDelayedTask _inherited;
};


// This class is used for clumping up function objects that
// can be done as a single object. For instance the mime
// notification mechanism sends out multiple notifications on
// a single change and we need to accumulate the resulting
// icon update into a single one
class AccumulatingFunctionObject : public FunctionObject {
public:
	virtual bool CanAccumulate(const AccumulatingFunctionObject*) const = 0;
	virtual void Accumulate(AccumulatingFunctionObject*) = 0;
};


// task loop is a separate thread that hosts tasks that keep getting called
// periodically; if a task returns true, it is done - it gets removed from
// the list and deleted
class TaskLoop {
public:
	TaskLoop(bigtime_t heartBeat = 10000);
	virtual ~TaskLoop();

	void RunLater(DelayedTask*);
	void RunLater(FunctionObject* functor, bigtime_t delay);
		// execute a function object after a delay

	void RunLater(FunctionObjectWithResult<bool>* functor, bigtime_t delay,
		bigtime_t period);
		// periodically execute function object after initial delay until
		// function object returns true

	void RunLater(FunctionObjectWithResult<bool>* functor, bigtime_t delay,
		bigtime_t period, bigtime_t timeout);
		// periodically execute function object after initial delay until
		// function object returns true or timeout is reached

	void AccumulatedRunLater(AccumulatingFunctionObject* functor,
		bigtime_t delay, bigtime_t maxAccumulatingTime = 0,
		int32 maxAccumulateCount = 0);
		// will search the delayed task loop for other accumulating functors
		// and will accumulate with them, else will create a new delayed task
		// the task will no longer accumulate if past the
		// <maxAccumulatingTime> delay unless <maxAccumulatingTime> is zero
		// no more than <maxAccumulateCount> will get accumulated, unless
		// <maxAccumulateCount> is zero

	void RunWhenIdle(FunctionObjectWithResult<bool>* functor,
		bigtime_t initialDelay, bigtime_t idleTime,
		bigtime_t heartBeat = 1000000);
		// after initialDelay starts looking for a slot when the system is
		// idle for at least idleTime

protected:
	void AddTask(DelayedTask*);
	void RemoveTask(DelayedTask*);

	bool Pulse();
		// return true if quitting
	bigtime_t LatestRunTime() const;
	
	virtual bool KeepPulsingWhenEmpty() const = 0;
	virtual void StartPulsingIfNeeded() = 0;

	BLocker fLock;
	BObjectList<DelayedTask> fTaskList;
	bigtime_t fHeartBeat;
};


class StandAloneTaskLoop : public TaskLoop {
	// this task loop can work on it's own, just instantiate it
	// and use it; It has to start it's own thread
public:
	StandAloneTaskLoop(bool keepThread, bigtime_t heartBeat = 400000);
	~StandAloneTaskLoop();

protected:
	void AddTask(DelayedTask*);

private:
	static status_t RunBinder(void*);
	void Run();

	virtual bool KeepPulsingWhenEmpty() const;
	virtual void StartPulsingIfNeeded();

	volatile bool fNeedToQuit;
	volatile thread_id fScanThread;
	bool fKeepThread;
	
	typedef TaskLoop _inherited;
};


class PiggybackTaskLoop : public TaskLoop {
	// this TaskLoop needs periodic calls from a viewable's Pulse
	// or some similar pulsing mechanism
	// it does not have to need it's own thread, instead it uses an existing
	// thread of a looper, etc.
public:
	PiggybackTaskLoop(bigtime_t heartBeat = 100000);
	~PiggybackTaskLoop();
	virtual void PulseMe();
private:
	virtual bool KeepPulsingWhenEmpty() const;
	virtual void StartPulsingIfNeeded();
	
	bigtime_t fNextHeartBeatTime;
	bool fPulseMe;
};


inline bigtime_t
DelayedTask::RunAfterTime() const
{
	return fRunAfter;
}


} // namespace BPrivate

using namespace BPrivate;

#endif	// __TASK_LOOP__
