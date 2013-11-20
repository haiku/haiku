/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <util/AutoLock.h>

#include "scheduler_common.h"
#include "scheduler_modes.h"


using namespace Scheduler;


static int32 sSmallTaskCore;


static bool
has_cache_expired(Thread* thread)
{
	ASSERT(!gSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	return system_time() - schedulerThreadData->went_sleep > kCacheExpire;
}


static void
switch_to_mode(void)
{
	sSmallTaskCore = -1;
}


static bool
try_small_task_packing(Thread* thread)
{
	int32 core = sSmallTaskCore;
	return (core == -1 && gCoreLoadHeap->PeekMaximum() != NULL)
		|| (core != -1
			&& gCoreEntries[core].fLoad + thread->scheduler_data->load
				< kHighLoad);
}


static int32
choose_small_task_core(void)
{
	CoreEntry* candidate = gCoreLoadHeap->PeekMaximum();
	if (candidate == NULL)
		return sSmallTaskCore;

	int32 core = candidate->fCoreID;
	int32 smallTaskCore = atomic_test_and_set(&sSmallTaskCore, core, -1);
	if (smallTaskCore == -1)
		return core;
	return smallTaskCore;
}


static int32
choose_core(Thread* thread)
{
	CoreEntry* entry;

	if (try_small_task_packing(thread)) {
		// try to pack all threads on one core
		entry = &gCoreEntries[choose_small_task_core()];
	} else if (gCoreLoadHeap->PeekMinimum() != NULL) {
		// run immediately on already woken core
		entry = gCoreLoadHeap->PeekMinimum();
	} else if (gPackageUsageHeap->PeekMinimum() != NULL) {
		// wake new core
		PackageEntry* package = gPackageUsageHeap->PeekMinimum();
		entry = package->fIdleCores.Last();
	} else if (gIdlePackageList->Last() != NULL) {
		// wake new package
		PackageEntry* package = gIdlePackageList->Last();
		entry = package->fIdleCores.Last();
	} else {
		// no idle cores, use least occupied core
		entry = gCoreLoadHeap->PeekMinimum();
		if (entry == NULL)
			entry = gCoreHighLoadHeap->PeekMinimum();
	}

	ASSERT(entry != NULL);
	return entry->fCoreID;
}


static bool
should_rebalance(Thread* thread)
{
	ASSERT(!gSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	int32 core = schedulerThreadData->previous_core;
	CoreEntry* coreEntry = &gCoreEntries[core];

	if (coreEntry->fLoad > kHighLoad) {
		SpinLocker coreLocker(gCoreHeapsLock);
		if (sSmallTaskCore == core) {
			CoreEntry* other = gCoreLoadHeap->PeekMaximum();
				
			if (other == NULL)
				sSmallTaskCore = -1;
			else if (coreEntry->fLoad - schedulerThreadData->load < kHighLoad)
				return true;
			else 
				sSmallTaskCore = other->fCoreID;
			return coreEntry->fLoad > kVeryHighLoad;
		}

		CoreEntry* other = gCoreHighLoadHeap->PeekMinimum();
		if (other == NULL)
			other = gCoreHighLoadHeap->PeekMaximum();
		return coreEntry->fLoad - other->fLoad >= kLoadDifference / 2;
	}

	return choose_small_task_core() != core;
}


static inline void
pack_irqs(void)
{
	cpu_ent* cpu = get_cpu_struct();
	int32 core = gCPUToCore[cpu->cpu_num];

	SpinLocker locker(cpu->irqs_lock);
	while (sSmallTaskCore != core && list_get_first_item(&cpu->irqs) != NULL) {
		irq_assignment* irq = (irq_assignment*)list_get_first_item(&cpu->irqs);
		locker.Unlock();

		SpinLocker coreLocker(gCoreHeapsLock);
		int32 newCPU
			= gCPUPriorityHeaps[sSmallTaskCore].PeekMinimum()->fCPUNumber;
		coreLocker.Unlock();

		if (newCPU != cpu->cpu_num)
			assign_io_interrupt_to_cpu(irq->irq, newCPU);

		locker.Lock();
	}
}


static void
rebalance_irqs(bool idle)
{
	if (idle && sSmallTaskCore != -1) {
		pack_irqs();
		return;
	}

	if (idle || sSmallTaskCore != -1)
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

	SpinLocker coreLocker(gCoreHeapsLock);
	CoreEntry* other = gCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		return;
	int32 newCPU = gCPUPriorityHeaps[other->fCoreID].PeekMinimum()->fCPUNumber;
	coreLocker.Unlock();

	int32 thisCore = gCPUToCore[smp_get_current_cpu()];
	if (other->fCoreID == thisCore)
		return;

	if (other->fLoad + kLoadDifference >= gCoreEntries[thisCore].fLoad)
		return;

	assign_io_interrupt_to_cpu(chosen->irq, newCPU);
}


scheduler_mode_operations gSchedulerPowerSavingMode = {
	"power saving",

	false,

	switch_to_mode,
	has_cache_expired,
	choose_core,
	should_rebalance,
	rebalance_irqs,
};

