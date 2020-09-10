/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* This file contains the cpu functions (init, etc). */


#include <cpu.h>
#include <arch/cpu.h>
#include <arch/system_info.h>

#include <string.h>

#include <cpufreq.h>
#include <cpuidle.h>

#include <boot/kernel_args.h>
#include <kscheduler.h>
#include <thread_types.h>
#include <util/AutoLock.h>


/* global per-cpu structure */
cpu_ent gCPU[SMP_MAX_CPUS];

uint32 gCPUCacheLevelCount;
static cpu_topology_node sCPUTopology;

static cpufreq_module_info* sCPUPerformanceModule;
static cpuidle_module_info* sCPUIdleModule;

static spinlock sSetCpuLock;


status_t
cpu_init(kernel_args *args)
{
	return arch_cpu_init(args);
}


status_t
cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	return arch_cpu_init_percpu(args, curr_cpu);
}


status_t
cpu_init_post_vm(kernel_args *args)
{
	return arch_cpu_init_post_vm(args);
}


static void
load_cpufreq_module()
{
	void* cookie = open_module_list(CPUFREQ_MODULES_PREFIX);

	while (true) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);
		cpufreq_module_info* current = NULL;

		if (read_next_module_name(cookie, name, &nameLength) != B_OK)
			break;

		if (get_module(name, (module_info**)&current) == B_OK) {
			dprintf("found cpufreq module: %s\n", name);

			if (sCPUPerformanceModule != NULL) {
				if (sCPUPerformanceModule->rank < current->rank) {
					put_module(sCPUPerformanceModule->info.name);
					sCPUPerformanceModule = current;
				} else
					put_module(name);
			} else
				sCPUPerformanceModule = current;
		}
	}

	close_module_list(cookie);

	if (sCPUPerformanceModule == NULL)
		dprintf("no valid cpufreq module found\n");
	else
		scheduler_update_policy();
}


static void
load_cpuidle_module()
{
	void* cookie = open_module_list(CPUIDLE_MODULES_PREFIX);

	while (true) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);
		cpuidle_module_info* current = NULL;

		if (read_next_module_name(cookie, name, &nameLength) != B_OK)
			break;

		if (get_module(name, (module_info**)&current) == B_OK) {
			dprintf("found cpuidle module: %s\n", name);

			if (sCPUIdleModule != NULL) {
				if (sCPUIdleModule->rank < current->rank) {
					put_module(sCPUIdleModule->info.name);
					sCPUIdleModule = current;
				} else
					put_module(name);
			} else
				sCPUIdleModule = current;
		}
	}

	close_module_list(cookie);

	if (sCPUIdleModule == NULL)
		dprintf("no valid cpuidle module found\n");
}


status_t
cpu_init_post_modules(kernel_args *args)
{
	status_t result = arch_cpu_init_post_modules(args);
	if (result != B_OK)
		return result;

	load_cpufreq_module();
	load_cpuidle_module();
	return B_OK;
}


status_t
cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	// set the cpu number in the local cpu structure so that
	// we can use it for get_current_cpu
	memset(&gCPU[curr_cpu], 0, sizeof(gCPU[curr_cpu]));
	gCPU[curr_cpu].cpu_num = curr_cpu;

	list_init(&gCPU[curr_cpu].irqs);
	B_INITIALIZE_SPINLOCK(&gCPU[curr_cpu].irqs_lock);

	return arch_cpu_preboot_init_percpu(args, curr_cpu);
}


bigtime_t
cpu_get_active_time(int32 cpu)
{
	if (cpu < 0 || cpu > smp_get_num_cpus())
		return 0;

	bigtime_t activeTime;
	uint32 count;

	do {
		count = acquire_read_seqlock(&gCPU[cpu].active_time_lock);
		activeTime = gCPU[cpu].active_time;
	} while (!release_read_seqlock(&gCPU[cpu].active_time_lock, count));

	return activeTime;
}


uint64
cpu_frequency(int32 cpu)
{
	if (cpu < 0 || cpu >= smp_get_num_cpus())
		return 0;
	uint64 frequency = 0;
	arch_get_frequency(&frequency, cpu);
	return frequency;
}


void
clear_caches(void *address, size_t length, uint32 flags)
{
	// ToDo: implement me!
}


static status_t
cpu_create_topology_node(cpu_topology_node* node, int32* maxID, int32 id)
{
	cpu_topology_level level = static_cast<cpu_topology_level>(node->level - 1);
	ASSERT(level >= 0);

	cpu_topology_node* newNode = new(std::nothrow) cpu_topology_node;
	if (newNode == NULL)
		return B_NO_MEMORY;
	node->children[id] = newNode;

	newNode->level = level;
	if (level != CPU_TOPOLOGY_SMT) {
		newNode->children_count = maxID[level - 1];
		newNode->children
			= new(std::nothrow) cpu_topology_node*[maxID[level - 1]];
		if (newNode->children == NULL)
			return B_NO_MEMORY;

		memset(newNode->children, 0,
			maxID[level - 1] * sizeof(cpu_topology_node*));
	} else {
		newNode->children_count = 0;
		newNode->children = NULL;
	}

	return B_OK;
}


static void
cpu_rebuild_topology_tree(cpu_topology_node* node, int32* lastID)
{
	if (node->children == NULL)
		return;

	int32 count = 0;
	for (int32 i = 0; i < node->children_count; i++) {
		if (node->children[i] == NULL)
			continue;

		if (count != i)
			node->children[count] = node->children[i];

		if (node->children[count]->level != CPU_TOPOLOGY_SMT)
			node->children[count]->id = lastID[node->children[count]->level]++;

		cpu_rebuild_topology_tree(node->children[count], lastID);
		count++;
	}
	node->children_count = count;
}


status_t
cpu_build_topology_tree(void)
{
	sCPUTopology.level = CPU_TOPOLOGY_LEVELS;

	int32 maxID[CPU_TOPOLOGY_LEVELS];
	memset(&maxID, 0, sizeof(maxID));

	const int32 kCPUCount = smp_get_num_cpus();
	for (int32 i = 0; i < kCPUCount; i++) {
		for (int32 j = 0; j < CPU_TOPOLOGY_LEVELS; j++)
			maxID[j] = max_c(maxID[j], gCPU[i].topology_id[j]);
	}

	for (int32 j = 0; j < CPU_TOPOLOGY_LEVELS; j++)
		maxID[j]++;

	sCPUTopology.children_count = maxID[CPU_TOPOLOGY_LEVELS - 1];
	sCPUTopology.children
		= new(std::nothrow) cpu_topology_node*[maxID[CPU_TOPOLOGY_LEVELS - 1]];
	if (sCPUTopology.children == NULL)
		return B_NO_MEMORY;
	memset(sCPUTopology.children, 0,
		maxID[CPU_TOPOLOGY_LEVELS - 1] * sizeof(cpu_topology_node*));

	for (int32 i = 0; i < kCPUCount; i++) {
		cpu_topology_node* node = &sCPUTopology;
		for (int32 j = CPU_TOPOLOGY_LEVELS - 1; j >= 0; j--) {
			int32 id = gCPU[i].topology_id[j];
			if (node->children[id] == NULL) {
				status_t result = cpu_create_topology_node(node, maxID, id);
				if (result != B_OK)
					return result;
			}

			node = node->children[id];
		}

		ASSERT(node->level == CPU_TOPOLOGY_SMT);
		node->id = i;
	}

	int32 lastID[CPU_TOPOLOGY_LEVELS];
	memset(&lastID, 0, sizeof(lastID));
	cpu_rebuild_topology_tree(&sCPUTopology, lastID);

	return B_OK;
}


const cpu_topology_node*
get_cpu_topology(void)
{
	return &sCPUTopology;
}


void
cpu_set_scheduler_mode(enum scheduler_mode mode)
{
	if (sCPUPerformanceModule != NULL)
		sCPUPerformanceModule->cpufreq_set_scheduler_mode(mode);
	if (sCPUIdleModule != NULL)
		sCPUIdleModule->cpuidle_set_scheduler_mode(mode);
}


status_t
increase_cpu_performance(int delta)
{
	if (sCPUPerformanceModule != NULL)
		return sCPUPerformanceModule->cpufreq_increase_performance(delta);
	return B_NOT_SUPPORTED;
}


status_t
decrease_cpu_performance(int delta)
{
	if (sCPUPerformanceModule != NULL)
		return sCPUPerformanceModule->cpufreq_decrease_performance(delta);
	return B_NOT_SUPPORTED;
}


void
cpu_idle(void)
{
#if KDEBUG
	if (!are_interrupts_enabled())
		panic("cpu_idle() called with interrupts disabled.");
#endif

	if (sCPUIdleModule != NULL)
		sCPUIdleModule->cpuidle_idle();
	else
		arch_cpu_idle();
}


void
cpu_wait(int32* variable, int32 test)
{
	if (sCPUIdleModule != NULL)
		sCPUIdleModule->cpuidle_wait(variable, test);
	else
		arch_cpu_pause();
}


//	#pragma mark -


void
_user_clear_caches(void *address, size_t length, uint32 flags)
{
	clear_caches(address, length, flags);
}


bool
_user_cpu_enabled(int32 cpu)
{
	if (cpu < 0 || cpu >= smp_get_num_cpus())
		return false;

	return !gCPU[cpu].disabled;
}


status_t
_user_set_cpu_enabled(int32 cpu, bool enabled)
{
	int32 i, count;

	if (geteuid() != 0)
		return B_PERMISSION_DENIED;
	if (cpu < 0 || cpu >= smp_get_num_cpus())
		return B_BAD_VALUE;

	// We need to lock here to make sure that no one can disable
	// the last CPU

	InterruptsSpinLocker locker(sSetCpuLock);

	if (!enabled) {
		// check if this is the last CPU to be disabled
		for (i = 0, count = 0; i < smp_get_num_cpus(); i++) {
			if (!gCPU[i].disabled)
				count++;
		}

		if (count == 1)
			return B_NOT_ALLOWED;
	}

	bool oldState = gCPU[cpu].disabled;

	if (oldState != !enabled)
		scheduler_set_cpu_enabled(cpu, enabled);

	if (!enabled) {
		if (smp_get_current_cpu() == cpu) {
			locker.Unlock();
			thread_yield();
			locker.Lock();
		}

		// someone reenabled the CPU while we were rescheduling
		if (!gCPU[cpu].disabled)
			return B_OK;

		ASSERT(smp_get_current_cpu() != cpu);
		while (!thread_is_idle_thread(gCPU[cpu].running_thread)) {
			locker.Unlock();
			thread_yield();
			locker.Lock();

			if (!gCPU[cpu].disabled)
				return B_OK;
			ASSERT(smp_get_current_cpu() != cpu);
		}
	}

	return B_OK;
}

