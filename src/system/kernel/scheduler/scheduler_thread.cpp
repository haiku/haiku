/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */

#include "scheduler_thread.h"


namespace Scheduler {


int32 gReadyThreadCount;


}	// namespace Scheduler

using namespace Scheduler;


static bigtime_t sQuantumLengths[THREAD_MAX_SET_PRIORITY + 1];


void
ThreadData::_InitBase()
{
	fPriorityPenalty = 0;
	fAdditionalPenalty = 0;
	fEffectivePriority = fThread->priority;

	fReceivedPenalty = false;
	fHasSlept = false;

	fTimeLeft = 0;
	fStolenTime = 0;

	fMeasureActiveTime = 0;
	fMeasureTime = 0;
	fLoad = 0;

	fWentSleep = 0;
	fWentSleepActive = 0;
	fWentSleepCount = 0;
	fWentSleepCountIdle = 0;

	fEnqueued = false;
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
		CPUEntry* previousCPU
			= CPUEntry::GetCPU(fThread->previous_cpu->cpu_num);
		if (previousCPU->Core() == core && !fThread->previous_cpu->disabled) {
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


ThreadData::ThreadData(Thread* thread)
	:
	fThread(thread)
{
}


void
ThreadData::Init()
{
	_InitBase();

	ThreadData* currentThreadData = thread_get_current_thread()->scheduler_data;
	fCore = currentThreadData->fCore;
}


void
ThreadData::Init(CoreEntry* core)
{
	_InitBase();

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
	kprintf("\teffective_priority:\t%" B_PRId32 "\n", GetEffectivePriority());

	kprintf("\treceived_penalty:\t%s\n", fReceivedPenalty ? "true" : "false");
	kprintf("\thas_slept:\t\t%s\n", fHasSlept ? "true" : "false");

	bigtime_t quantum = _GetBaseQuantum();
	if (fThread->priority < B_FIRST_REAL_TIME_PRIORITY) {
		int32 threadCount = (fCore->ThreadCount() + 1) / fCore->CPUCount();
		threadCount = max_c(threadCount, 1);

		quantum
			= std::min(gCurrentMode->maximum_latency / threadCount, quantum);
		quantum = std::max(quantum,	gCurrentMode->minimal_quantum);
	}
	kprintf("\ttime_left:\t\t%" B_PRId64 " us (quantum: %" B_PRId64 " us)\n",
		fTimeLeft, quantum);

	kprintf("\tstolen_time:\t\t%" B_PRId64 " us\n", fStolenTime);
	kprintf("\tquantum_start:\t\t%" B_PRId64 " us\n", fQuantumStart);
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


void
ThreadData::ComputeLoad()
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(gTrackLoad);
	compute_load(fMeasureTime, fMeasureActiveTime, fLoad);
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

	int32 threadCount = fCore->ThreadCount() / fCore->CPUCount();
	if (threadCount >= 1) {
		quantum
			= std::min(gCurrentMode->maximum_latency / threadCount, quantum);
		quantum = std::max(quantum,	gCurrentMode->minimal_quantum);
	}

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

