/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_MODES_H
#define KERNEL_SCHEDULER_MODES_H


#include <kscheduler.h>
#include <thread_types.h>


struct scheduler_mode_operations {
	const char*		name;

	bool			avoid_boost;

	bigtime_t		base_quantum;
	bigtime_t		minimal_quantum;
	bigtime_t		quantum_multipliers[2];

	bigtime_t		maximum_latency;

	void			(*switch_to_mode)(void);
	void			(*set_cpu_enabled)(int32 cpu, bool enabled);
	bool			(*has_cache_expired)(Thread* thread);
	int32			(*choose_core)(Thread* thread);
	bool			(*should_rebalance)(Thread* thread);
	void			(*rebalance_irqs)(bool idle);
};

extern struct scheduler_mode_operations gSchedulerLowLatencyMode;
extern struct scheduler_mode_operations gSchedulerPowerSavingMode;

#endif	// KERNEL_SCHEDULER_MODES_H

