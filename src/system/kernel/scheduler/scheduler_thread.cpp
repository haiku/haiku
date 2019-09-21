/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */

#include "scheduler_thread.h"


using namespace Scheduler;


static bigtime_t sQuantumLengths[THREAD_MAX_SET_PRIORITY + 1];

const int32 kMaximumQuantumLengthsCount	= 20;
static bigtime_t sMaximumQuantumLengths[kMaximumQuantumLengthsCount];


void
ThreadData::_InitBase()
{
	fStolenTime = 0;
	fQuantumStart = 0;
	fLastInterruptTime = 0;

	fWentSleep = 0;
	fWentSleepActive = 0;

	fEnqueued = false;
	fReady = false;

	fPriorityPenalty = 0;
	fAdditionalPenalty = 0;

	fEffectivePriority = GetPriority();
	fBaseQuantum = sQuantumLengths[GetEffectivePriority()];

	fTimeUsed = 0;

	fMeasureAvailableActiveTime = 0;
	fLastMeasureAvailableTime = 0;
	fMeasureAvailableTime = 0;
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


ThreadData::ThreadData(Thread* thread)
	:
	fThread(thread)
{
}


void
ThreadData::Init()
{
	_InitBase();
	fCore = NULL;

	Thread* currentThread = thread_get_current_thread();
	ThreadData* currentThreadData = currentThread->scheduler_data;
	fNeededLoad = currentThreadData->fNeededLoad;

	if (!IsRealTime()) {
		fPriorityPenalty = std::min(currentThreadData->fPriorityPenalty,
				std::max(GetPriority() - _GetMinimalPriority(), int32(0)));
		fAdditionalPenalty = currentThreadData->fAdditionalPenalty;

		_ComputeEffectivePriority();
	}
}


void
ThreadData::Init(CoreEntry* core)
{
	_InitBase();

	fCore = core;
	fReady = true;
	fNeededLoad = 0;
}


void
ThreadData::Dump() const
{
	kprintf("\tpriority_penalty:\t%" B_PRId32 "\n", fPriorityPenalty);

	int32 priority = GetPriority() - _GetPenalty();
	priority = std::max(priority, int32(1));
	kprintf("\tadditional_penalty:\t%" B_PRId32 " (%" B_PRId32 ")\n",
		fAdditionalPenalty % priority, fAdditionalPenalty);
	kprintf("\teffective_priority:\t%" B_PRId32 "\n", GetEffectivePriority());

	kprintf("\ttime_used:\t\t%" B_PRId64 " us (quantum: %" B_PRId64 " us)\n",
		fTimeUsed, ComputeQuantum());
	kprintf("\tstolen_time:\t\t%" B_PRId64 " us\n", fStolenTime);
	kprintf("\tquantum_start:\t\t%" B_PRId64 " us\n", fQuantumStart);
	kprintf("\tneeded_load:\t\t%" B_PRId32 "%%\n", fNeededLoad / 10);
	kprintf("\twent_sleep:\t\t%" B_PRId64 "\n", fWentSleep);
	kprintf("\twent_sleep_active:\t%" B_PRId64 "\n", fWentSleepActive);
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

	if (fCore != targetCore) {
		fLoadMeasurementEpoch = targetCore->LoadMeasurementEpoch() - 1;
		if (fReady) {
			if (fCore != NULL)
				fCore->RemoveLoad(fNeededLoad, true);
			targetCore->AddLoad(fNeededLoad, fLoadMeasurementEpoch, true);
		}
	}

	fCore = targetCore;
	return rescheduleNeeded;
}


bigtime_t
ThreadData::ComputeQuantum() const
{
	SCHEDULER_ENTER_FUNCTION();

	if (IsRealTime())
		return fBaseQuantum;

	int32 threadCount = fCore->ThreadCount();
	if (fCore->CPUCount() > 0)
		threadCount /= fCore->CPUCount();

	bigtime_t quantum = fBaseQuantum;
	if (threadCount < kMaximumQuantumLengthsCount)
		quantum = std::min(sMaximumQuantumLengths[threadCount], quantum);
	return quantum;
}


void
ThreadData::UnassignCore(bool running)
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(fCore != NULL);
	if (running || fThread->state == B_THREAD_READY)
		fReady = false;
	if (!fReady)
		fCore = NULL;
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

	for (int32 threadCount = 0; threadCount < kMaximumQuantumLengthsCount;
		threadCount++) {

		bigtime_t quantum = gCurrentMode->maximum_latency;
		if (threadCount != 0)
			quantum /= threadCount;
		quantum = std::max(quantum, gCurrentMode->minimal_quantum);
		sMaximumQuantumLengths[threadCount] = quantum;
	}
}


inline int32
ThreadData::_GetPenalty() const
{
	SCHEDULER_ENTER_FUNCTION();
	return fPriorityPenalty;
}


void
ThreadData::_ComputeNeededLoad()
{
	SCHEDULER_ENTER_FUNCTION();
	ASSERT(!IsIdle());

	int32 oldLoad = compute_load(fLastMeasureAvailableTime,
		fMeasureAvailableActiveTime, fNeededLoad, fMeasureAvailableTime);
	if (oldLoad < 0 || oldLoad == fNeededLoad)
		return;

	fCore->ChangeLoad(fNeededLoad - oldLoad);
}


void
ThreadData::_ComputeEffectivePriority() const
{
	SCHEDULER_ENTER_FUNCTION();

	if (IsIdle())
		fEffectivePriority = B_IDLE_PRIORITY;
	else if (IsRealTime())
		fEffectivePriority = GetPriority();
	else {
		fEffectivePriority = GetPriority();
		fEffectivePriority -= _GetPenalty();
		if (fEffectivePriority > 0)
			fEffectivePriority -= fAdditionalPenalty % fEffectivePriority;

		ASSERT(fEffectivePriority < B_FIRST_REAL_TIME_PRIORITY);
		ASSERT(fEffectivePriority >= B_LOWEST_ACTIVE_PRIORITY);
	}

	fBaseQuantum = sQuantumLengths[GetEffectivePriority()];
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

