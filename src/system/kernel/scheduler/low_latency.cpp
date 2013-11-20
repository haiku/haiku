/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <util/AutoLock.h>

#include "scheduler_common.h"
#include "scheduler_modes.h"


using namespace Scheduler;


static void
switch_to_mode(void)
{
}


static bool
has_cache_expired(Thread* thread)
{
	ASSERT(!gSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	CoreEntry* coreEntry = &gCoreEntries[schedulerThreadData->previous_core];
	return atomic_get64(&coreEntry->fActiveTime)
			- schedulerThreadData->went_sleep_active > kCacheExpire;
}


static int32
choose_core(Thread* thread)
{
	CoreEntry* entry;

	if (gIdlePackageList->Last() != NULL) {
		// wake new package
		PackageEntry* package = gIdlePackageList->Last();
		entry = package->fIdleCores.Last();
	} else if (gPackageUsageHeap->PeekMaximum() != NULL) {
		// wake new core
		PackageEntry* package = gPackageUsageHeap->PeekMaximum();
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
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	CoreEntry* coreEntry = &gCoreEntries[schedulerThreadData->previous_core];

	// If the thread produces more than 50% of the load, leave it here. In
	// such situation it is better to move other threads away.
	if (schedulerThreadData->load >= coreEntry->fLoad / 2)
		return false;

	// If there is high load on this core but this thread does not contribute
	// significantly consider giving it to someone less busy.
	if (coreEntry->fLoad > kHighLoad) {
		SpinLocker coreLocker(gCoreHeapsLock);

		CoreEntry* other = gCoreLoadHeap->PeekMinimum();
		if (other != NULL && coreEntry->fLoad - other->fLoad >= kLoadDifference)
			return true;
	}

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	SpinLocker coreLocker(gCoreHeapsLock);

	CoreEntry* other = gCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = gCoreHighLoadHeap->PeekMinimum();
	return coreEntry->fLoad - other->fLoad >= kLoadDifference * 2;
}


static void
rebalance_irqs(bool idle)
{
	if (idle)
		return;

	cpu_ent* cpu = get_cpu_struct();
	SpinLocker locker(cpu->irqs_lock);

	irq_assignment* chosen = NULL;
	irq_assignment* irq = (irq_assignment*)list_get_first_item(&cpu->irqs);

	int32 totalLoad = 0;
	while (irq != NULL) {
		if (chosen == NULL || chosen->load < irq->load)
			chosen = irq;
		totalLoad += irq->load;
		irq = (irq_assignment*)list_get_next_item(&cpu->irqs, irq);
	}

	locker.Unlock();

	if (chosen == NULL || totalLoad < kLowLoad)
		return;

	SpinLocker coreLocker(gCoreHeapsLock);
	CoreEntry* other = gCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = gCoreHighLoadHeap->PeekMinimum();

	int32 newCPU = gCPUPriorityHeaps[other->fCoreID].PeekMinimum()->fCPUNumber;
	coreLocker.Unlock();

	ASSERT(other != NULL);

	int32 thisCore = gCPUToCore[smp_get_current_cpu()];
	if (other->fCoreID == thisCore)
		return;

	if (other->fLoad + kLoadDifference >= gCoreEntries[thisCore].fLoad)
		return;

	assign_io_interrupt_to_cpu(chosen->irq, newCPU);
}


scheduler_mode_operations gSchedulerLowLatencyMode = {
	"low latency",

	true,

	switch_to_mode,
	has_cache_expired,
	choose_core,
	should_rebalance,
	rebalance_irqs,
};

