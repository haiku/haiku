/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_LOCKING_H
#define KERNEL_SCHEDULER_LOCKING_H


#include <util/AutoLock.h>

#include "scheduler_cpu.h"


namespace Scheduler {


class CPURunQueueLocking {
public:
	inline bool Lock(CPUEntry* cpu)
	{
		cpu->LockRunQueue();
		return true;
	}

	inline void Unlock(CPUEntry* cpu)
	{
		cpu->UnlockRunQueue();
	}
};

typedef AutoLocker<CPUEntry, CPURunQueueLocking> CPURunQueueLocker;


class CoreRunQueueLocking {
public:
	inline bool Lock(CoreEntry* core)
	{
		core->LockRunQueue();
		return true;
	}

	inline void Unlock(CoreEntry* core)
	{
		core->UnlockRunQueue();
	}
};

typedef AutoLocker<CoreEntry, CoreRunQueueLocking> CoreRunQueueLocker;

class CoreCPUHeapLocking {
public:
	inline bool Lock(CoreEntry* core)
	{
		core->LockCPUHeap();
		return true;
	}

	inline void Unlock(CoreEntry* core)
	{
		core->UnlockCPUHeap();
	}
};

typedef AutoLocker<CoreEntry, CoreCPUHeapLocking> CoreCPUHeapLocker;

class SchedulerModeLocking {
public:
	bool Lock(int* /* lockable */)
	{
		CPUEntry::GetCPU(smp_get_current_cpu())->EnterScheduler();
		return true;
	}

	void Unlock(int* /* lockable */)
	{
		CPUEntry::GetCPU(smp_get_current_cpu())->ExitScheduler();
	}
};

class SchedulerModeLocker :
	public AutoLocker<int, SchedulerModeLocking> {
public:
	SchedulerModeLocker(bool alreadyLocked = false, bool lockIfNotLocked = true)
		:
		AutoLocker<int, SchedulerModeLocking>(&fDummy, alreadyLocked,
			lockIfNotLocked)
	{
	}

private:
	int		fDummy;
};

class InterruptsSchedulerModeLocking {
public:
	bool Lock(int* lockable)
	{
		*lockable = disable_interrupts();
		CPUEntry::GetCPU(smp_get_current_cpu())->EnterScheduler();
		return true;
	}

	void Unlock(int* lockable)
	{
		CPUEntry::GetCPU(smp_get_current_cpu())->ExitScheduler();
		restore_interrupts(*lockable);
	}
};

class InterruptsSchedulerModeLocker :
	public AutoLocker<int, InterruptsSchedulerModeLocking> {
public:
	InterruptsSchedulerModeLocker(bool alreadyLocked = false,
		bool lockIfNotLocked = true)
		:
		AutoLocker<int, InterruptsSchedulerModeLocking>(&fState, alreadyLocked,
			lockIfNotLocked)
	{
	}

private:
	int		fState;
};

class InterruptsBigSchedulerLocking {
public:
	bool Lock(int* lockable)
	{
		*lockable = disable_interrupts();
		for (int32 i = 0; i < smp_get_num_cpus(); i++)
			CPUEntry::GetCPU(i)->LockScheduler();
		return true;
	}

	void Unlock(int* lockable)
	{
		for (int32 i = 0; i < smp_get_num_cpus(); i++)
			CPUEntry::GetCPU(i)->UnlockScheduler();
		restore_interrupts(*lockable);
	}
};

class InterruptsBigSchedulerLocker :
	public AutoLocker<int, InterruptsBigSchedulerLocking> {
public:
	InterruptsBigSchedulerLocker()
		:
		AutoLocker<int, InterruptsBigSchedulerLocking>(&fState, false, true)
	{
	}

private:
	int		fState;
};


}	// namespace Scheduler


#endif	// KERNEL_SCHEDULER_LOCKING_H

