/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SMP_H_
#define _SMP_H_


#include <SupportDefs.h>


struct Mutex
{
	int32 fLock;
	
	Mutex(): fLock(0) {}

	bool TryLock()
	{
		return atomic_test_and_set(&fLock, -1, 0) == 0;
	}

	bool Lock()
	{
		while (!TryLock()) {}
		if (atomic_add(&fLock, -1) < 0) {
		}
		return true;
	}
	
	void Unlock()
	{
		atomic_add(&fLock, 1);
	}
};


struct CpuInfo
{
	uint32 phandle;
	uint32 hartId;
	uint32 plicContext;
};


CpuInfo* smp_find_cpu(uint32 phandle);

void smp_init_other_cpus(void);
void smp_boot_other_cpus(uint64 pageTable, uint64 kernel_entry);

void smp_init();


#endif	// _SMP_H_
