/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef _PHYSICAL_MEMORY_ALLOCATOR_H_
#define _PHYSICAL_MEMORY_ALLOCATOR_H_

#include <condition_variable.h>
#include <SupportDefs.h>
#include <lock.h>


class PhysicalMemoryAllocator {
public:
									PhysicalMemoryAllocator(const char *name,
										size_t minSize,
										size_t maxSize,
										uint32 minCountPerBlock);
									~PhysicalMemoryAllocator();

		status_t					InitCheck() { return fStatus; };

		status_t					Allocate(size_t size,
										void **logicalAddress,
										phys_addr_t *physicalAddress);

		// one of both addresses needs to be provided, the other may be NULL
		status_t					Deallocate(size_t size,
										void *logicalAddress,
										phys_addr_t physicalAddress);

		void						PrintToStream();
		void						DumpArrays();
		void						DumpLastArray();
		void						DumpFreeSlots();

private:
		bool						_Lock();
		void						_Unlock();

		char						*fName;

		size_t						fOverhead;
		size_t						fManagedMemory;
		status_t					fStatus;

		mutex						fLock;
		area_id						fArea;
		void						*fLogicalBase;
		phys_addr_t					fPhysicalBase;

		int32						fArrayCount;
		size_t						*fBlockSize;
		size_t						*fArrayLength;
		size_t						*fArrayOffset;
		uint8						**fArray;

		ConditionVariable			fNoMemoryCondition;
		uint32						fMemoryWaitersCount;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		uint32						fDebugBase;
		uint32						fDebugChunkSize;
		uint64						fDebugUseMap;
#endif
};

#endif // !_PHYSICAL_MEMORY_ALLOCATOR_H_
