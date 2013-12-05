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

	inline	void		GoesAway();

	inline	void		PutBack();
	inline	void		Enqueue();
	inline	bool		Dequeue();

	inline	void		UpdateActivity(bigtime_t active);
	inline	void		ComputeLoad();

	inline	bool		HasQuantumEnded(bool wasPreempted, bool hasYielded);
			bigtime_t	ComputeQuantum();

	inline	Thread*		GetThread() const	{ return fThread; }
	inline	int32		GetLoad() const	{ return fLoad; }

	inline	CoreEntry*	GetCore() const	{ return fCore; }
	inline	void		UnassignCore() { fCore = NULL; }

			bigtime_t	fStolenTime;
			bigtime_t	fQuantumStart;
			bigtime_t	fLastInterruptTime;

			bigtime_t	fWentSleep;
			bigtime_t	fWentSleepActive;
			int32		fWentSleepCount;

			bool		fEnqueued;

private:
	inline	int32		_GetPenalty() const;
	inline	int32		_GetMinimalPriority() const;

	inline	CoreEntry*	_ChooseCore() const;
	inline	CPUEntry*	_ChooseCPU(CoreEntry* core,
							bool& rescheduleNeeded) const;

	inline	bigtime_t	_GetBaseQuantum() const;
	static	bigtime_t	_ScaleQuantum(bigtime_t maxQuantum,
							bigtime_t minQuantum, int32 maxPriority,
							int32 minPriority, int32 priority);

			Thread*		fThread;

			int32		fPriorityPenalty;
			int32		fAdditionalPenalty;

			bigtime_t	fTimeLeft;

			bigtime_t	fMeasureActiveTime;
			bigtime_t	fMeasureTime;
			int32		fLoad;

			CoreEntry*	fCore;
};


inline bool
ThreadData::HasCacheExpired() const
{
	return gCurrentMode->has_cache_expired(this);
}


inline bool
ThreadData::ShouldRebalance() const
{
	ASSERT(!gSingleCore);
	return gCurrentMode->should_rebalance(this);
}


inline int32
ThreadData::GetEffectivePriority() const
{
	if (thread_is_idle_thread(fThread))
		return B_IDLE_PRIORITY;
	if (fThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return fThread->priority;

	int32 effectivePriority = fThread->priority;
	effectivePriority -= _GetPenalty();

	ASSERT(effectivePriority < B_FIRST_REAL_TIME_PRIORITY);
	ASSERT(effectivePriority >= B_LOWEST_ACTIVE_PRIORITY);

	return effectivePriority;
}


inline void
ThreadData::IncreasePenalty()
{
	if (fThread->priority < B_LOWEST_ACTIVE_PRIORITY)
		return;
	if (fThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return;

	TRACE("increasing thread %ld penalty\n", fThread->id);

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
	if (fPriorityPenalty != 0)
		TRACE("cancelling thread %ld penalty\n", fThread->id);

	fAdditionalPenalty = 0;
	fPriorityPenalty = 0;
}


inline bool
ThreadData::ShouldCancelPenalty() const
{
	if (fCore == NULL)
		return false;

	return atomic_get(&fCore->fStarvationCounter) != fWentSleepCount
		&& system_time() - fWentSleep > gCurrentMode->base_quantum;
}


inline void
ThreadData::GoesAway()
{
	fLastInterruptTime = 0;

	fWentSleep = system_time();
	fWentSleepActive = atomic_get64(&fCore->fActiveTime);
	fWentSleepCount = atomic_get(&fCore->fStarvationCounter);
}


inline void
ThreadData::PutBack()
{
	ComputeLoad();
	fWentSleepCount = -1;

	int32 priority = GetEffectivePriority();

	SpinLocker runQueueLocker(fCore->fQueueLock);
	ASSERT(!fEnqueued);
	fEnqueued = true;
	if (fThread->pinned_to_cpu > 0) {
		ASSERT(fThread->cpu != NULL);

		CPUEntry* cpu = &gCPUEntries[fThread->cpu->cpu_num];
		cpu->fRunQueue.PushFront(this, priority);
	} else {
		fCore->fRunQueue.PushFront(this, priority);
		atomic_add(&fCore->fThreadCount, 1);
	}
}


inline void
ThreadData::Enqueue()
{
	fThread->state = B_THREAD_READY;
	ComputeLoad();
	fWentSleepCount = 0;

	int32 priority = GetEffectivePriority();

	SpinLocker runQueueLocker(fCore->fQueueLock);
	ASSERT(!fEnqueued);
	fEnqueued = true;
	if (fThread->pinned_to_cpu > 0) {
		ASSERT(fThread->previous_cpu != NULL);

		CPUEntry* cpu = &gCPUEntries[fThread->previous_cpu->cpu_num];
		cpu->fRunQueue.PushBack(this, priority);
	} else {
		fCore->fRunQueue.PushBack(this, priority);
		fCore->fThreadList.Insert(this);

		atomic_add(&fCore->fThreadCount, 1);
	}
}


inline bool
ThreadData::Dequeue()
{
	SpinLocker runQueueLocker(fCore->fQueueLock);
	if (!fEnqueued)
		return false;

	fEnqueued = false;
	if (fThread->pinned_to_cpu > 0) {
		ASSERT(fThread->previous_cpu != NULL);

		CPUEntry* cpu = &gCPUEntries[fThread->previous_cpu->cpu_num];
		cpu->fRunQueue.Remove(this);
	} else {
		fCore->fRunQueue.Remove(this);

		ASSERT(fWentSleepCount < 1);
		if (fWentSleepCount == 0)
			fCore->fThreadList.Remove(this);
		atomic_add(&fCore->fThreadCount, -1);
	}

	return true;
}


inline void
ThreadData::UpdateActivity(bigtime_t active)
{
	fMeasureActiveTime += active;
	gCPUEntries[smp_get_current_cpu()].fMeasureActiveTime += active;
	atomic_add64(&fCore->fActiveTime, active);
}


inline void
ThreadData::ComputeLoad()
{
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


inline int32
ThreadData::_GetPenalty() const
{
	int32 penalty = fPriorityPenalty;

	const int kMinimalPriority = _GetMinimalPriority();
	if (kMinimalPriority > 0)
		penalty += fAdditionalPenalty % kMinimalPriority;

	return penalty;
}


inline int32
ThreadData::_GetMinimalPriority() const
{
	const int32 kDivisor = 5;

	const int32 kMaximalPriority = 25;
	const int32 kMinimalPriority = B_LOWEST_ACTIVE_PRIORITY;

	int32 priority = fThread->priority / kDivisor;
	return std::max(std::min(priority, kMaximalPriority), kMinimalPriority);
}


inline CoreEntry*
ThreadData::_ChooseCore() const
{
	ASSERT(!gSingleCore);
	return gCurrentMode->choose_core(this);
}


inline CPUEntry*
ThreadData::_ChooseCPU(CoreEntry* core, bool& rescheduleNeeded) const
{
	SpinLocker cpuLocker(core->fCPULock);
	CPUEntry* cpu = core->fCPUHeap.PeekMinimum();
	ASSERT(cpu != NULL);

	int32 threadPriority = GetEffectivePriority();
	if (CPUPriorityHeap::GetKey(cpu) < threadPriority) {
		cpu->UpdatePriority(threadPriority);
		rescheduleNeeded = true;
	} else
		rescheduleNeeded = false;

	return cpu;
}


}	// namespace Scheduler


#endif	// KERNEL_SCHEDULER_THREAD_H

