/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_MODES_H
#define KERNEL_SCHEDULER_MODES_H


#include <kscheduler.h>
#include <thread_types.h>


struct scheduler_mode_operations {
	const char*				name;

	bigtime_t				base_quantum;
	bigtime_t				minimal_quantum;
	bigtime_t				quantum_multipliers[2];

	bigtime_t				maximum_latency;

	void					(*switch_to_mode)();
	void					(*set_cpu_enabled)(int32 cpu, bool enabled);
	bool					(*has_cache_expired)(
								const Scheduler::ThreadData* threadData);
	Scheduler::CoreEntry*	(*choose_core)(
								const Scheduler::ThreadData* threadData);
	Scheduler::CoreEntry*	(*rebalance)(
								const Scheduler::ThreadData* threadData);
	void					(*rebalance_irqs)(bool idle);
};

extern struct scheduler_mode_operations gSchedulerLowLatencyMode;
extern struct scheduler_mode_operations gSchedulerPowerSavingMode;


namespace Scheduler {


extern scheduler_mode gCurrentModeID;
extern scheduler_mode_operations* gCurrentMode;


}


#endif	// KERNEL_SCHEDULER_MODES_H

