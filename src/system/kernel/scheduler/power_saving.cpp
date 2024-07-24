/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <util/atomic.h>
#include <util/AutoLock.h>

#include "scheduler_common.h"
#include "scheduler_cpu.h"
#include "scheduler_modes.h"
#include "scheduler_profiler.h"
#include "scheduler_thread.h"


using namespace Scheduler;


const bigtime_t kCacheExpire = 100000;

static CoreEntry* sSmallTaskCore;


static void
switch_to_mode()
{
	sSmallTaskCore = NULL;
}


static void
set_cpu_enabled(int32 cpu, bool enabled)
{
	if (!enabled)
		sSmallTaskCore = NULL;
}


static bool
has_cache_expired(const ThreadData* threadData)
{
	SCHEDULER_ENTER_FUNCTION();
	if (threadData->WentSleep() == 0)
		return false;
	return system_time() - threadData->WentSleep() > kCacheExpire;
}


static CoreEntry*
choose_small_task_core()
{
	SCHEDULER_ENTER_FUNCTION();

	ReadSpinLocker coreLocker(gCoreHeapsLock);
	CoreEntry* core = gCoreLoadHeap.PeekMaximum();
	if (core == NULL)
		return sSmallTaskCore;

	CoreEntry* smallTaskCore
		= atomic_pointer_test_and_set(&sSmallTaskCore, core, (CoreEntry*)NULL);
	if (smallTaskCore == NULL)
		return core;
	return smallTaskCore;
}


static CoreEntry*
choose_idle_core()
{
	SCHEDULER_ENTER_FUNCTION();

	PackageEntry* package = PackageEntry::GetLeastIdlePackage();

	if (package == NULL)
		package = gIdlePackageList.Last();

	if (package != NULL)
		return package->GetIdleCore();
	return NULL;
}


static CoreEntry*
choose_core(const ThreadData* threadData)
{
	SCHEDULER_ENTER_FUNCTION();

	CoreEntry* core = NULL;

	CPUSet mask = threadData->GetCPUMask();
	const bool useMask = !mask.IsEmpty();

	// try to pack all threads on one core
	core = choose_small_task_core();
	if (core != NULL && (useMask && !core->CPUMask().Matches(mask)))
		core = NULL;

	if (core == NULL || core->GetLoad() + threadData->GetLoad() >= kHighLoad) {
		ReadSpinLocker coreLocker(gCoreHeapsLock);

		// run immediately on already woken core
		int32 index = 0;
		do {
			core = gCoreLoadHeap.PeekMinimum(index++);
		} while (useMask && core != NULL && !core->CPUMask().Matches(mask));
		if (core == NULL) {
			coreLocker.Unlock();

			core = choose_idle_core();
			if (useMask && !core->CPUMask().Matches(mask))
				core = NULL;

			if (core == NULL) {
				coreLocker.Lock();
				index = 0;
				do {
					core = gCoreHighLoadHeap.PeekMinimum(index++);
				} while (useMask && core != NULL && !core->CPUMask().Matches(mask));
			}
		}
	}

	ASSERT(core != NULL);
	return core;
}


static CoreEntry*
rebalance(const ThreadData* threadData)
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(!gSingleCore);

	CPUSet mask = threadData->GetCPUMask();
	const bool useMask = !mask.IsEmpty();

	CoreEntry* core = threadData->Core();

	int32 coreLoad = core->GetLoad();
	int32 threadLoad = threadData->GetLoad() / core->CPUCount();
	if (coreLoad > kHighLoad) {
		if (sSmallTaskCore == core) {
			sSmallTaskCore = NULL;
			CoreEntry* smallTaskCore = choose_small_task_core();

			if (threadLoad > coreLoad / 3 || smallTaskCore == NULL
					|| (useMask && !smallTaskCore->CPUMask().Matches(mask))) {
				return core;
			}
			return coreLoad > kVeryHighLoad ? smallTaskCore : core;
		}

		if (threadLoad >= coreLoad / 2)
			return core;

		ReadSpinLocker coreLocker(gCoreHeapsLock);
		CoreEntry* other;
		int32 index = 0;
		do {
			other = gCoreLoadHeap.PeekMaximum(index++);
		} while (useMask && other != NULL && !other->CPUMask().Matches(mask));
		if (other == NULL) {
			index = 0;
			do {
				other = gCoreHighLoadHeap.PeekMinimum(index++);
			} while (useMask && other != NULL && !other->CPUMask().Matches(mask));
		}
		coreLocker.Unlock();
		ASSERT(other != NULL);

		int32 coreNewLoad = coreLoad - threadLoad;
		int32 otherNewLoad = other->GetLoad() + threadLoad;
		return coreNewLoad - otherNewLoad >= kLoadDifference / 2 ? other : core;
	}

	if (coreLoad >= kMediumLoad)
		return core;

	CoreEntry* smallTaskCore = choose_small_task_core();
	if (smallTaskCore == NULL || (useMask && !smallTaskCore->CPUMask().Matches(mask)))
		return core;
	return smallTaskCore->GetLoad() + threadLoad < kHighLoad
		? smallTaskCore : core;
}


static inline void
pack_irqs()
{
	SCHEDULER_ENTER_FUNCTION();

	CoreEntry* smallTaskCore = atomic_pointer_get(&sSmallTaskCore);
	if (smallTaskCore == NULL)
		return;

	cpu_ent* cpu = get_cpu_struct();
	if (smallTaskCore == CoreEntry::GetCore(cpu->cpu_num))
		return;

	SpinLocker locker(cpu->irqs_lock);
	while (list_get_first_item(&cpu->irqs) != NULL) {
		irq_assignment* irq = (irq_assignment*)list_get_first_item(&cpu->irqs);
		locker.Unlock();

		int32 newCPU = smallTaskCore->CPUHeap()->PeekRoot()->ID();

		if (newCPU != cpu->cpu_num)
			assign_io_interrupt_to_cpu(irq->irq, newCPU);

		locker.Lock();
	}
}


static void
rebalance_irqs(bool idle)
{
	SCHEDULER_ENTER_FUNCTION();

	if (idle && sSmallTaskCore != NULL) {
		pack_irqs();
		return;
	}

	if (idle || sSmallTaskCore != NULL)
		return;

	cpu_ent* cpu = get_cpu_struct();
	SpinLocker locker(cpu->irqs_lock);

	irq_assignment* chosen = NULL;
	irq_assignment* irq = (irq_assignment*)list_get_first_item(&cpu->irqs);

	while (irq != NULL) {
		if (chosen == NULL || chosen->load < irq->load)
			chosen = irq;
		irq = (irq_assignment*)list_get_next_item(&cpu->irqs, irq);
	}

	locker.Unlock();

	if (chosen == NULL || chosen->load < kLowLoad)
		return;

	ReadSpinLocker coreLocker(gCoreHeapsLock);
	CoreEntry* other = gCoreLoadHeap.PeekMinimum();
	coreLocker.Unlock();
	if (other == NULL)
		return;
	int32 newCPU = other->CPUHeap()->PeekRoot()->ID();

	CoreEntry* core = CoreEntry::GetCore(smp_get_current_cpu());
	if (other == core)
		return;
	if (other->GetLoad() + kLoadDifference >= core->GetLoad())
		return;

	assign_io_interrupt_to_cpu(chosen->irq, newCPU);
}


scheduler_mode_operations gSchedulerPowerSavingMode = {
	"power saving",

	2000,
	500,
	{ 3, 10 },

	20000,

	switch_to_mode,
	set_cpu_enabled,
	has_cache_expired,
	choose_core,
	rebalance,
	rebalance_irqs,
};

