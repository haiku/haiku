/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_THREAD_H
#define KERNEL_SCHEDULER_THREAD_H


#include <thread.h>
#include <util/AutoLock.h>

#include "scheduler_common.h"
#include "scheduler_cpu.h"
#include "scheduler_locking.h"
#include "scheduler_profiler.h"


namespace Scheduler {


struct ThreadData : public DoublyLinkedListLinkImpl<ThreadData>,
	RunQueueLinkImpl<ThreadData> {
private:
	inline	void		_InitBase();

	inline	int32		_GetMinimalPriority() const;

	inline	CoreEntry*	_ChooseCore() const;
	inline	CPUEntry*	_ChooseCPU(CoreEntry* core,
							bool& rescheduleNeeded) const;

	inline	bigtime_t	_GetBaseQuantum() const;

public:
						ThreadData(Thread* thread);

			void		Init();
			void		Init(CoreEntry* core);

			void		Dump() const;

	inline	bool		HasCacheExpired() const;
	inline	bool		ShouldRebalance() const;

	inline	int32		GetEffectivePriority() const;

	inline	void		CancelPenalty();
	inline	bool		ShouldCancelPenalty() const;

	inline	bool		IsCPUBound() const	{ return fAdditionalPenalty != 0; }

			bool		ChooseCoreAndCPU(CoreEntry*& targetCore,
							CPUEntry*& targetCPU);

	inline	void		SetLastInterruptTime(bigtime_t interruptTime)
							{ fLastInterruptTime = interruptTime; }
	inline	void		SetStolenInterruptTime(bigtime_t interruptTime);

	inline	void		GoesAway();
	inline	bigtime_t	WentSleep() const	{ return fWentSleep; }
	inline	bigtime_t	WentSleepActive() const	{ return fWentSleepActive; }

	inline	void		PutBack();
	inline	void		Enqueue();
	inline	bool		Dequeue();

	inline	void		UpdateActivity(bigtime_t active);
			void		ComputeLoad();

	inline	bool		HasQuantumEnded(bool wasPreempted, bool hasYielded);
			bigtime_t	ComputeQuantum();
	inline	void		StartQuantum();

	inline	bool		IsEnqueued() const	{ return fEnqueued; }
	inline	void		SetDequeued()	{ fEnqueued = false; }

	inline	Thread*		GetThread() const	{ return fThread; }
	inline	int32		GetLoad() const	{ return fLoad; }

	inline	CoreEntry*	Core() const	{ return fCore; }
	inline	void		UnassignCore() { fCore = NULL; }

	static	void		ComputeQuantumLengths();

private:
	inline	void		_IncreasePenalty();
	inline	int32		_GetPenalty() const;

			void		_ComputeEffectivePriority() const;

	static	bigtime_t	_ScaleQuantum(bigtime_t maxQuantum,
							bigtime_t minQuantum, int32 maxPriority,
							int32 minPriority, int32 priority);

			bigtime_t	fStolenTime;
			bigtime_t	fQuantumStart;
			bigtime_t	fLastInterruptTime;

			bigtime_t	fWentSleep;
			bigtime_t	fWentSleepActive;
			int32		fWentSleepCount;
			int32		fWentSleepCountIdle;

			bool		fEnqueued;

			Thread*		fThread;

			int32		fPriorityPenalty;
			int32		fAdditionalPenalty;
			bool		fReceivedPenalty;
			bool		fHasSlept;

	mutable	int32		fEffectivePriority;

			bigtime_t	fTimeLeft;

			bigtime_t	fMeasureActiveTime;
			bigtime_t	fMeasureTime;
			int32		fLoad;

			CoreEntry*	fCore;
};

class ThreadProcessing {
public:
	virtual				~ThreadProcessing();

	virtual	void		operator()(ThreadData* thread) = 0;
};


inline int32
ThreadData::_GetMinimalPriority() const
{
	SCHEDULER_ENTER_FUNCTION();

	const int32 kDivisor = 5;

	const int32 kMaximalPriority = 25;
	const int32 kMinimalPriority = B_LOWEST_ACTIVE_PRIORITY;

	int32 priority = fThread->priority / kDivisor;
	return std::max(std::min(priority, kMaximalPriority), kMinimalPriority);
}


inline bool
ThreadData::HasCacheExpired() const
{
	SCHEDULER_ENTER_FUNCTION();
	return gCurrentMode->has_cache_expired(this);
}


inline bool
ThreadData::ShouldRebalance() const
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(!gSingleCore);
	return gCurrentMode->should_rebalance(this);
}


inline int32
ThreadData::GetEffectivePriority() const
{
	SCHEDULER_ENTER_FUNCTION();
	return fEffectivePriority;
}


inline void
ThreadData::_IncreasePenalty()
{
	SCHEDULER_ENTER_FUNCTION();

	if (fThread->priority < B_LOWEST_ACTIVE_PRIORITY)
		return;
	if (fThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return;

	TRACE("increasing thread %ld penalty\n", fThread->id);

	fReceivedPenalty = true;
	int32 oldPenalty = fPriorityPenalty++;

	ASSERT(fThread->priority - oldPenalty >= B_LOWEST_ACTIVE_PRIORITY);

	const int kMinimalPriority = _GetMinimalPriority();
	if (fThread->priority - oldPenalty <= kMinimalPriority) {
		fPriorityPenalty = oldPenalty;
		fAdditionalPenalty++;
	}

	_ComputeEffectivePriority();
}


inline void
ThreadData::CancelPenalty()
{
	SCHEDULER_ENTER_FUNCTION();

	int32 oldPenalty = fPriorityPenalty;

	fAdditionalPenalty = 0;
	fPriorityPenalty = 0;

	if (oldPenalty != 0) {
		TRACE("cancelling thread %ld penalty\n", fThread->id);
		_ComputeEffectivePriority();
	}
}


inline bool
ThreadData::ShouldCancelPenalty() const
{
	SCHEDULER_ENTER_FUNCTION();

	if (fCore == NULL)
		return false;
	if (system_time() - fWentSleep > gCurrentMode->minimal_quantum * 2)
		return false;

	if (GetEffectivePriority() != B_LOWEST_ACTIVE_PRIORITY
		&& !IsCPUBound()) {
		if (fCore->StarvationCounter() != fWentSleepCount)
			return true;
	}

	return fCore->StarvationCounterIdle() != fWentSleepCountIdle;
}


inline void
ThreadData::SetStolenInterruptTime(bigtime_t interruptTime)
{
	SCHEDULER_ENTER_FUNCTION();

	interruptTime -= fLastInterruptTime;
	fStolenTime += interruptTime;
	fMeasureActiveTime -= interruptTime;
}


inline void
ThreadData::GoesAway()
{
	SCHEDULER_ENTER_FUNCTION();

	if (!fReceivedPenalty)
		_IncreasePenalty();
	fHasSlept = true;

	fLastInterruptTime = 0;

	fWentSleep = system_time();
	fWentSleepCount = fCore->StarvationCounter();
	fWentSleepCountIdle = fCore->StarvationCounterIdle();
	fWentSleepActive = fCore->GetActiveTime();
}


inline void
ThreadData::PutBack()
{
	SCHEDULER_ENTER_FUNCTION();

	if (gTrackLoad)
		ComputeLoad();

	int32 priority = GetEffectivePriority();

	if (fThread->pinned_to_cpu > 0) {
		ASSERT(fThread->cpu != NULL);
		CPUEntry* cpu = CPUEntry::GetCPU(fThread->cpu->cpu_num);

		CPURunQueueLocker _(cpu);
		ASSERT(!fEnqueued);
		fEnqueued = true;

		cpu->PushFront(this, priority);
	} else {
		CoreRunQueueLocker _(fCore);
		ASSERT(!fEnqueued);
		fEnqueued = true;

		fCore->PushFront(this, priority);
	}
}


inline void
ThreadData::Enqueue()
{
	SCHEDULER_ENTER_FUNCTION();

	fThread->state = B_THREAD_READY;

	if (gTrackLoad)
		ComputeLoad();

	int32 priority = GetEffectivePriority();

	if (fThread->pinned_to_cpu > 0) {
		ASSERT(fThread->previous_cpu != NULL);
		CPUEntry* cpu = CPUEntry::GetCPU(fThread->previous_cpu->cpu_num);

		CPURunQueueLocker _(cpu);
		ASSERT(!fEnqueued);
		fEnqueued = true;

		cpu->PushBack(this, priority);
	} else {
		CoreRunQueueLocker _(fCore);
		ASSERT(!fEnqueued);
		fEnqueued = true;

		fCore->PushBack(this, priority);
	}
}


inline bool
ThreadData::Dequeue()
{
	SCHEDULER_ENTER_FUNCTION();

	if (fThread->pinned_to_cpu > 0) {
		ASSERT(fThread->previous_cpu != NULL);
		CPUEntry* cpu = CPUEntry::GetCPU(fThread->previous_cpu->cpu_num);

		CPURunQueueLocker _(cpu);
		if (!fEnqueued)
			return false;
		cpu->Remove(this);
		ASSERT(!fEnqueued);
		return true;
	}

	CoreRunQueueLocker _(fCore);
	if (!fEnqueued)
		return false;

	fCore->Remove(this);
	ASSERT(!fEnqueued);
	return true;
}


inline void
ThreadData::UpdateActivity(bigtime_t active)
{
	SCHEDULER_ENTER_FUNCTION();
	fMeasureActiveTime += active;
}


inline bool
ThreadData::HasQuantumEnded(bool wasPreempted, bool hasYielded)
{
	SCHEDULER_ENTER_FUNCTION();

	if (hasYielded) {
		fTimeLeft = 0;
		return true;
	}

	bigtime_t timeUsed = system_time() - fQuantumStart;
	if (timeUsed > 0);
		fTimeLeft -= timeUsed;
	fTimeLeft = std::max(fTimeLeft, bigtime_t(0));

	// too little time left, it's better make the next quantum a bit longer
	int32 skipTime = gCurrentMode->minimal_quantum;
	skipTime -= skipTime / 10;
	if (wasPreempted || fTimeLeft <= skipTime) {
		fStolenTime += fTimeLeft;
		fTimeLeft = 0;
	}

	if (fTimeLeft == 0) {
		if (!fReceivedPenalty && !fHasSlept)
			_IncreasePenalty();
		fReceivedPenalty = false;
		fHasSlept = false;
	}

	return fTimeLeft == 0;
}


inline void
ThreadData::StartQuantum()
{
	SCHEDULER_ENTER_FUNCTION();
	fQuantumStart = system_time();
}


}	// namespace Scheduler


#endif	// KERNEL_SCHEDULER_THREAD_H

