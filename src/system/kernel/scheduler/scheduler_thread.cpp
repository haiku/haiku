/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */

#include "scheduler_thread.h"


using namespace Scheduler;


static bigtime_t sQuantumLengths[THREAD_MAX_SET_PRIORITY + 1];


ThreadData::ThreadData(Thread* thread)
	:
	fThread(thread)
{
}


void
ThreadData::Init()
{
	fAdditionalPenalty = 0;
	fEffectivePriority = -1;

	fTimeLeft = 0;
	fStolenTime = 0;

	fMeasureActiveTime = 0;
	fMeasureTime = 0;
	fLoad = 0;

	fWentSleep = 0;
	fWentSleepActive = 0;
	fWentSleepCount = -1;

	fEnqueued = false;

	Thread* currentThread = thread_get_current_thread();
	ASSERT(currentThread != NULL);
	if (!thread_is_idle_thread(currentThread)) {
		ThreadData* currentThreadData = currentThread->scheduler_data;
		int32 penalty = currentThreadData->fPriorityPenalty;

		int32 minimalPriority = _GetMinimalPriority();
		if (fThread->priority - penalty >= minimalPriority)
			fPriorityPenalty = penalty;
		else
			fPriorityPenalty = fThread->priority - minimalPriority;

		fCore = currentThreadData->fCore;
	} else {
		fPriorityPenalty = 0;
		fAdditionalPenalty = 0;

		fCore = NULL;
	}
}


void
ThreadData::Init(CoreEntry* core)
{
	Init();
	fCore = core;
}


void
ThreadData::Dump() const
{
	kprintf("\tpriority_penalty:\t%" B_PRId32 "\n", fPriorityPenalty);

	int32 additionalPenalty = 0;
	const int kMinimalPriority = _GetMinimalPriority();
	if (kMinimalPriority > 0)
		additionalPenalty = fAdditionalPenalty % kMinimalPriority;
	kprintf("\tadditional_penalty:\t%" B_PRId32 " (%" B_PRId32 ")\n",
		additionalPenalty, fAdditionalPenalty);
	kprintf("\tstolen_time:\t\t%" B_PRId64 "\n", fStolenTime);
	kprintf("\tload:\t\t\t%" B_PRId32 "%%\n", fLoad / 10);
	kprintf("\twent_sleep:\t\t%" B_PRId64 "\n", fWentSleep);
	kprintf("\twent_sleep_active:\t%" B_PRId64 "\n", fWentSleepActive);
	kprintf("\twent_sleep_count:\t%" B_PRId32 "\n", fWentSleepCount);
	kprintf("\tcore:\t\t\t%" B_PRId32 "\n",
		fCore != NULL ? fCore->ID() : -1);
	if (fCore != NULL && HasCacheExpired())
		kprintf("\tcache affinity has expired\n");
}


bool
ThreadData::ChooseCoreAndCPU(CoreEntry*& targetCore, CPUEntry*& targetCPU)
{
	SCHEDULER_ENTER_FUNCTION();

	bool rescheduleNeeded = false;

	if (targetCore == NULL && targetCPU != NULL)
		targetCore = targetCPU->Core();
	else if (targetCore != NULL && targetCPU == NULL)
		targetCPU = _ChooseCPU(targetCore, rescheduleNeeded);
	else if (targetCore == NULL && targetCPU == NULL) {
		targetCore = _ChooseCore();
		targetCPU = _ChooseCPU(targetCore, rescheduleNeeded);
	}

	ASSERT(targetCore != NULL);
	ASSERT(targetCPU != NULL);

	fCore = targetCore;
	return rescheduleNeeded;
}


bigtime_t
ThreadData::ComputeQuantum()
{
	SCHEDULER_ENTER_FUNCTION();

	bigtime_t quantum;
	if (fTimeLeft != 0)
		quantum = fTimeLeft;
	else
		quantum = _GetBaseQuantum();

	if (fThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return quantum;

	quantum += fStolenTime;
	fStolenTime = 0;

	int32 threadCount = (fCore->ThreadCount() + 1) / fCore->CPUCount();
	threadCount = max_c(threadCount, 1);

	quantum = std::min(gCurrentMode->maximum_latency / threadCount, quantum);
	quantum = std::max(quantum,	gCurrentMode->minimal_quantum);

	fTimeLeft = quantum;

	return quantum;
}


/* static */ void
ThreadData::ComputeQuantumLengths()
{
	SCHEDULER_ENTER_FUNCTION();

	for (int32 priority = 0; priority <= THREAD_MAX_SET_PRIORITY; priority++) {
		const bigtime_t kQuantum0 = gCurrentMode->base_quantum;
		if (priority >= B_URGENT_DISPLAY_PRIORITY) {
			sQuantumLengths[priority] = kQuantum0;
			continue;
		}

		const bigtime_t kQuantum1
			= kQuantum0 * gCurrentMode->quantum_multipliers[0];
		if (priority > B_NORMAL_PRIORITY) {
			sQuantumLengths[priority] = _ScaleQuantum(kQuantum1, kQuantum0,
				B_URGENT_DISPLAY_PRIORITY, B_NORMAL_PRIORITY, priority);
			continue;
		}

		const bigtime_t kQuantum2
			= kQuantum0 * gCurrentMode->quantum_multipliers[1];
		sQuantumLengths[priority] = _ScaleQuantum(kQuantum2, kQuantum1,
			B_NORMAL_PRIORITY, B_IDLE_PRIORITY, priority);
	}
}


void
ThreadData::_ComputeEffectivePriority() const
{
	SCHEDULER_ENTER_FUNCTION();

	if (thread_is_idle_thread(fThread))
		fEffectivePriority = B_IDLE_PRIORITY;
	else if (fThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		fEffectivePriority = fThread->priority;
	else {
		fEffectivePriority = fThread->priority;
		fEffectivePriority -= _GetPenalty();

		ASSERT(fEffectivePriority < B_FIRST_REAL_TIME_PRIORITY);
		ASSERT(fEffectivePriority >= B_LOWEST_ACTIVE_PRIORITY);
	}
}


inline CoreEntry*
ThreadData::_ChooseCore() const
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(!gSingleCore);
	return gCurrentMode->choose_core(this);
}


inline CPUEntry*
ThreadData::_ChooseCPU(CoreEntry* core, bool& rescheduleNeeded) const
{
	SCHEDULER_ENTER_FUNCTION();

	int32 threadPriority = GetEffectivePriority();

	if (fThread->previous_cpu != NULL) {
		CPUEntry* previousCPU = &gCPUEntries[fThread->previous_cpu->cpu_num];
		if (previousCPU->Core() == core) {
			CoreCPUHeapLocker _(core);
			if (CPUPriorityHeap::GetKey(previousCPU) < threadPriority) {
				previousCPU->UpdatePriority(threadPriority);
				rescheduleNeeded = true;
				return previousCPU;
			}
		}
	}

	CoreCPUHeapLocker _(core);
	CPUEntry* cpu = core->CPUHeap()->PeekRoot();
	ASSERT(cpu != NULL);

	if (CPUPriorityHeap::GetKey(cpu) < threadPriority) {
		cpu->UpdatePriority(threadPriority);
		rescheduleNeeded = true;
	} else
		rescheduleNeeded = false;

	return cpu;
}


inline bigtime_t
ThreadData::_GetBaseQuantum() const
{
	SCHEDULER_ENTER_FUNCTION();

	return sQuantumLengths[GetEffectivePriority()];
}


/* static */ bigtime_t
ThreadData::_ScaleQuantum(bigtime_t maxQuantum, bigtime_t minQuantum,
	int32 maxPriority, int32 minPriority, int32 priority)
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(priority <= maxPriority);
	ASSERT(priority >= minPriority);

	bigtime_t result = (maxQuantum - minQuantum) * (priority - minPriority);
	result /= maxPriority - minPriority;
	return maxQuantum - result;
}


ThreadProcessing::~ThreadProcessing()
{
}

