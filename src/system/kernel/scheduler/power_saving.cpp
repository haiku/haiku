/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <util/AutoLock.h>

#include "scheduler_common.h"
#include "scheduler_modes.h"


using namespace Scheduler;


static bigtime_t sDisableSmallTaskPacking;
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


static inline bool
is_small_task_packing_enabled(void)
{
	if (sDisableSmallTaskPacking == -1)
		return false;
	return sDisableSmallTaskPacking < system_time();
}


static inline void
disable_small_task_packing(void)
{
	ASSERT(!gSingleCore);

	ASSERT(is_small_task_packing_enabled());
	ASSERT(sSmallTaskCore == gCPUToCore[smp_get_current_cpu()]);

	sDisableSmallTaskPacking = system_time() + kThreadQuantum * 100;
	sSmallTaskCore = -1;
}


static inline bool
is_task_small(Thread* thread)
{
	return thread->scheduler_data->load <= 200;
}


static void
switch_to_mode(void)
{
	sDisableSmallTaskPacking = -1;
	sSmallTaskCore = -1;
}


static int32
choose_core(Thread* thread)
{
	CoreEntry* entry;

	if (is_small_task_packing_enabled() && is_task_small(thread)
		&& gCoreLoadHeap->PeekMaximum() != NULL) {
		// try to pack all threads on one core
		if (sSmallTaskCore < 0)
			sSmallTaskCore = gCoreLoadHeap->PeekMaximum()->fCoreID;
		entry = &gCoreEntries[sSmallTaskCore];
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

	// If the thread produces more than 50% of the load, leave it here. In
	// such situation it is better to move other threads away.
	// Unless we are trying to pack small tasks here, in such case get rid
	// of CPU hungry thread and continue packing.
	if (schedulerThreadData->load >= coreEntry->fLoad / 2)
		return is_small_task_packing_enabled() && sSmallTaskCore == core;

	// All cores try to give us small tasks, check whether we have enough.
	if (is_small_task_packing_enabled() && sSmallTaskCore == core) {
		if (coreEntry->fLoad > kHighLoad) {
			if (!is_task_small(thread))
				return true;
		} else if (coreEntry->fLoad > kVeryHighLoad)
			disable_small_task_packing();
	}

	// Try small task packing.
	if (is_small_task_packing_enabled() && is_task_small(thread))
		return sSmallTaskCore != core;

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	SpinLocker coreLocker(gCoreHeapsLock);

	CoreEntry* other = gCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = gCoreHighLoadHeap->PeekMinimum();
	return coreEntry->fLoad - other->fLoad >= kLoadDifference;
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
	if (idle && !is_small_task_packing_enabled() && sSmallTaskCore != -1) {
		pack_irqs();
		return;
	}

	if (idle)
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

