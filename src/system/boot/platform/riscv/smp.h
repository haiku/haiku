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


#endif	// _SMP_H_
