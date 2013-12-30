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
#include "scheduler_profiler.h"


namespace Scheduler {


struct ThreadData : public DoublyLinkedListLinkImpl<ThreadData>,
	RunQueueLinkImpl<ThreadData> {
public:
						ThreadData(Thread* thread);

			void		Init();
			void		Init(CoreEntry* core);

			void		Dump() const;

	inline	bool		HasCacheExpired() const;
	inline	bool		ShouldRebalance() const;

	inline	int32		GetEffectivePriority() const;

	inline	void		IncreasePenalty();
	inline	void		CancelPenalty();
	inline	bool		ShouldCancelPenalty() const;

			bool		ChooseCoreAndCPU(CoreEntry*& targetCore,
							CPUEntry*& targetCPU);

	inline	bigtime_t	LastInterruptTime() const
							{ return fLastInterruptTime; }
	inline	void		SetLastInterruptTime(bigtime_t interruptTime)
							{ fLastInterruptTime = interruptTime; }

	inline	void		IncreaseStolenTime(bigtime_t stolenTime);

	inline	void		GoesAway();
	inline	bigtime_t	WentSleep() const	{ return fWentSleep; }
	inline	bigtime_t	WentSleepActive() const	{ return fWentSleepActive; }
	inline	bigtime_t	WentSleepCount() const	{ return fWentSleepCount; }

	inline	void		PutBack();
	inline	void		Enqueue();
	inline	bool		Dequeue();

	inline	void		UpdateActivity(bigtime_t active);
	inline	void		ComputeLoad();

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
	inline	int32		_GetPenalty() const;
	inline	int32		_GetMinimalPriority() const;

			void		_ComputeEffectivePriority() const;

	inline	CoreEntry*	_ChooseCore() const;
	inline	CPUEntry*	_ChooseCPU(CoreEntry* core,
							bool& rescheduleNeeded) const;

	inline	bigtime_t	_GetBaseQuantum() const;
	static	bigtime_t	_ScaleQuantum(bigtime_t maxQuantum,
							bigtime_t minQuantum, int32 maxPriority,
							int32 minPriority, int32 priority);

			bigtime_t	fStolenTime;
			bigtime_t	fQuantumStart;
			bigtime_t	fLastInterruptTime;

			bigtime_t	fWentSleep;
			bigtime_t	fWentSleepActive;
			int32		fWentSleepCount;

			bool		fEnqueued;

			Thread*		fThread;

			int32		fPriorityPenalty;
			int32		fAdditionalPenalty;

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

	if (fEffectivePriority == -1)
		_ComputeEffectivePriority();
	return fEffectivePriority;
}


inline void
ThreadData::IncreasePenalty()
{
	SCHEDULER_ENTER_FUNCTION();

	if (fThread->priority < B_LOWEST_ACTIVE_PRIORITY)
		return;
	if (fThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return;

	TRACE("increasing thread %ld penalty\n", fThread->id);

	fEffectivePriority = -1;
	int32 oldPenalty = fPriorityPenalty++;

	ASSERT(fThread->priority - oldPenalty >= B_LOWEST_ACTIVE_PRIORITY);

	const int kMinimalPriority = _GetMinimalPriority();
	if (fThread->priority - oldPenalty <= kMinimalPriority) {
		fPriorityPenalty = oldPenalty;
		fAdditionalPenalty++;
	}
}


inline void
ThreadData::CancelPenalty()
{
	SCHEDULER_ENTER_FUNCTION();

	if (fPriorityPenalty != 0) {
		TRACE("cancelling thread %ld penalty\n", fThread->id);
		fEffectivePriority = -1;
	}

	fAdditionalPenalty = 0;
	fPriorityPenalty = 0;
}


inline bool
ThreadData::ShouldCancelPenalty() const
{
	SCHEDULER_ENTER_FUNCTION();

	if (fCore == NULL || fWentSleep == 0)
		return false;

	return fCore->StarvationCounter() != fWentSleepCount
		&& system_time() - fWentSleep > gCurrentMode->base_quantum;
}


inline void
ThreadData::IncreaseStolenTime(bigtime_t stolenTime)
{
	SCHEDULER_ENTER_FUNCTION();
	fStolenTime += stolenTime;
}


inline void
ThreadData::GoesAway()
{
	SCHEDULER_ENTER_FUNCTION();

	fLastInterruptTime = 0;

	fWentSleep = system_time();
	fWentSleepCount = fCore->StarvationCounter();
	fWentSleepActive = fCore->GetActiveTime();
}


inline void
ThreadData::PutBack()
{
	SCHEDULER_ENTER_FUNCTION();

	ComputeLoad();
	fWentSleepCount = -1;

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
	ComputeLoad();
	fWentSleepCount = 0;

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
	ASSERT(fWentSleepCount < 1);
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


inline void
ThreadData::ComputeLoad()
{
	SCHEDULER_ENTER_FUNCTION();

	if (fLastInterruptTime > 0) {
		bigtime_t interruptTime = gCPU[smp_get_current_cpu()].interrupt_time;
		interruptTime -= fLastInterruptTime;
		fMeasureActiveTime -= interruptTime;
	}

	compute_load(fMeasureTime, fMeasureActiveTime, fLoad);
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
	fTimeLeft -= timeUsed;
	fTimeLeft = std::max(fTimeLeft, bigtime_t(0));

	// too little time left, it's better make the next quantum a bit longer
	if (wasPreempted || fTimeLeft <= gCurrentMode->minimal_quantum) {
		fStolenTime += fTimeLeft;
		fTimeLeft = 0;
	}

	return fTimeLeft == 0;
}


inline void
ThreadData::StartQuantum()
{
	SCHEDULER_ENTER_FUNCTION();
	fQuantumStart = system_time();
}


inline int32
ThreadData::_GetPenalty() const
{
	SCHEDULER_ENTER_FUNCTION();

	int32 penalty = fPriorityPenalty;

	const int kMinimalPriority = _GetMinimalPriority();
	if (kMinimalPriority > 0)
		penalty += fAdditionalPenalty % kMinimalPriority;

	return penalty;
}


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


}	// namespace Scheduler


#endif	// KERNEL_SCHEDULER_THREAD_H

