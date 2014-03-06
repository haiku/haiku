/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <util/AutoLock.h>

#include "scheduler_common.h"
#include "scheduler_cpu.h"
#include "scheduler_modes.h"
#include "scheduler_profiler.h"
#include "scheduler_thread.h"


using namespace Scheduler;


const bigtime_t kCacheExpire = 100000;


static void
switch_to_mode()
{
}


static void
set_cpu_enabled(int32 /* cpu */, bool /* enabled */)
{
}


static bool
has_cache_expired(const ThreadData* threadData)
{
	SCHEDULER_ENTER_FUNCTION();
	if (threadData->WentSleepActive() == 0)
		return false;
	CoreEntry* core = threadData->Core();
	bigtime_t activeTime = core->GetActiveTime();
	return activeTime - threadData->WentSleepActive() > kCacheExpire;
}


static CoreEntry*
choose_core(const ThreadData* /* threadData */)
{
	SCHEDULER_ENTER_FUNCTION();

	// wake new package
	PackageEntry* package = gIdlePackageList.Last();
	if (package == NULL) {
		// wake new core
		package = PackageEntry::GetMostIdlePackage();
	}

	CoreEntry* core = NULL;
	if (package != NULL)
		core = package->GetIdleCore();

	if (core == NULL) {
		ReadSpinLocker coreLocker(gCoreHeapsLock);
		// no idle cores, use least occupied core
		core = gCoreLoadHeap.PeekMinimum();
		if (core == NULL)
			core = gCoreHighLoadHeap.PeekMinimum();
	}

	ASSERT(core != NULL);
	return core;
}


static bool
should_rebalance(const ThreadData* threadData)
{
	SCHEDULER_ENTER_FUNCTION();

	int32 coreLoad = threadData->Core()->GetLoad();
	int32 threadLoad = threadData->GetLoad() / threadData->Core()->CPUCount();

	// If the thread produces more than 50% of the load, leave it here. In
	// such situation it is better to move other threads away.
	if (threadLoad >= coreLoad / 2)
		return false;

	int32 coreNewLoad = coreLoad - threadLoad;

	ReadSpinLocker coreLocker(gCoreHeapsLock);
	CoreEntry* other = gCoreLoadHeap.PeekMinimum();
	if (other == NULL)
		other = gCoreHighLoadHeap.PeekMinimum();
	coreLocker.Unlock();

	ASSERT(other != NULL);
	int32 otherNewLoad = other->GetLoad() + threadLoad;

	// If there is high load on this core but this thread does not contribute
	// significantly consider giving it to someone less busy.
	if (coreLoad > kHighLoad) {
		if (coreNewLoad - otherNewLoad >= kLoadDifference)
			return true;
	}

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	if (other->GetLoad() == 0 && coreNewLoad != 0)
		return true;
	return coreNewLoad - otherNewLoad >= kLoadDifference * 2;
}


static void
rebalance_irqs(bool idle)
{
	SCHEDULER_ENTER_FUNCTION();

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
	CoreEntry* other = gCoreLoadHeap.PeekMinimum();
	if (other == NULL)
		other = gCoreHighLoadHeap.PeekMinimum();
	coreLocker.Unlock();

	int32 newCPU = other->CPUHeap()->PeekRoot()->ID();

	ASSERT(other != NULL);

	CoreEntry* core = CoreEntry::GetCore(cpu->cpu_num);
	if (other == core)
		return;
	if (other->GetLoad() + kLoadDifference >= core->GetLoad())
		return;

	assign_io_interrupt_to_cpu(chosen->irq, newCPU);
}


scheduler_mode_operations gSchedulerLowLatencyMode = {
	"low latency",

	1000,
	100,
	{ 2, 5 },

	5000,

	switch_to_mode,
	set_cpu_enabled,
	has_cache_expired,
	choose_core,
	should_rebalance,
	rebalance_irqs,
};

