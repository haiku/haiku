/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <util/AutoLock.h>

#include "scheduler_common.h"
#include "scheduler_modes.h"


using namespace Scheduler;


const bigtime_t kCacheExpire = 100000;


static void
switch_to_mode(void)
{
}


static void
set_cpu_enabled(int32 /* cpu */, bool /* enabled */)
{
}


static bool
has_cache_expired(Thread* thread)
{
	ASSERT(!gSingleCore);

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	CoreEntry* coreEntry = &gCoreEntries[schedulerThreadData->previous_core];
	return atomic_get64(&coreEntry->fActiveTime)
			- schedulerThreadData->went_sleep_active > kCacheExpire;
}


static inline PackageEntry*
get_most_idle_package(void)
{
	PackageEntry* current = &gPackageEntries[0];
	for (int32 i = 1; i < gPackageCount; i++) {
		if (gPackageEntries[i].fIdleCoreCount > current->fIdleCoreCount)
			current = &gPackageEntries[i];
	}

	if (current->fIdleCoreCount == 0)
		return NULL;

	return current;
}


static int32
choose_core(Thread* thread)
{
	CoreEntry* entry = NULL;

	ReadSpinLocker locker(gIdlePackageLock);
	// wake new package
	PackageEntry* package = gIdlePackageList->Last();
	if (package == NULL) {
		// wake new core
		package = get_most_idle_package();
	}
	locker.Unlock();

	if (package != NULL) {
		ReadSpinLocker _(package->fCoreLock);
		entry = package->fIdleCores.Last();
	}

	if (entry == NULL) {
		ReadSpinLocker coreLocker(gCoreHeapsLock);
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

	int32 coreLoad = get_core_load(coreEntry);

	// If the thread produces more than 50% of the load, leave it here. In
	// such situation it is better to move other threads away.
	if (schedulerThreadData->load >= coreLoad / 2)
		return false;

	// If there is high load on this core but this thread does not contribute
	// significantly consider giving it to someone less busy.
	if (coreLoad > kHighLoad) {
		ReadSpinLocker coreLocker(gCoreHeapsLock);

		CoreEntry* other = gCoreLoadHeap->PeekMinimum();
		if (other != NULL && coreLoad - get_core_load(other)
				>= kLoadDifference) {
			return true;
		}
	}

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	ReadSpinLocker coreLocker(gCoreHeapsLock);

	CoreEntry* other = gCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = gCoreHighLoadHeap->PeekMinimum();
	ASSERT(other != NULL);
	return coreLoad - get_core_load(other) >= kLoadDifference * 2;
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

	ReadSpinLocker coreLocker(gCoreHeapsLock);
	CoreEntry* other = gCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = gCoreHighLoadHeap->PeekMinimum();
	coreLocker.Unlock();

	SpinLocker cpuLocker(other->fCPULock);
	int32 newCPU = gCPUPriorityHeaps[other->fCoreID].PeekMinimum()->fCPUNumber;
	cpuLocker.Unlock();


	ASSERT(other != NULL);

	int32 thisCore = gCPUToCore[smp_get_current_cpu()];
	if (other->fCoreID == thisCore)
		return;

	if (get_core_load(other) + kLoadDifference
			>= get_core_load(&gCoreEntries[thisCore])) {
		return;
	}

	assign_io_interrupt_to_cpu(chosen->irq, newCPU);
}


scheduler_mode_operations gSchedulerLowLatencyMode = {
	"low latency",

	true,

	2000,
	700,
	{ 2, 30 },

	60000,

	switch_to_mode,
	set_cpu_enabled,
	has_cache_expired,
	choose_core,
	should_rebalance,
	rebalance_irqs,
};

