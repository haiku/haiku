/*
 * Copyright 2006-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <malloc.h>
#include <string.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <util/AutoLock.h>
#include <util/kernel_cpp.h>

#include "PhysicalMemoryAllocator.h"


//#define TRACE_PHYSICAL_MEMORY_ALLOCATOR
#ifdef TRACE_PHYSICAL_MEMORY_ALLOCATOR
#define TRACE(x)		dprintf x
#define TRACE_ERROR(x)	dprintf x
#else
#define TRACE(x)		/* nothing */
#define TRACE_ERROR(x)	dprintf x
#endif


PhysicalMemoryAllocator::PhysicalMemoryAllocator(const char *name,
	size_t minSize, size_t maxSize, uint32 minCountPerBlock)
	:	fOverhead(0),
		fStatus(B_NO_INIT),
		fMemoryWaitersCount(0)
{
	fName = strdup(name);
	mutex_init_etc(&fLock, fName, MUTEX_FLAG_CLONE_NAME);

	fArrayCount = 1;
	size_t biggestSize = minSize;
	while (biggestSize < maxSize) {
		fArrayCount++;
		biggestSize *= 2;
	}

	size_t size = fArrayCount * sizeof(uint8 *);
	fArray = (uint8 **)malloc(size);
	fOverhead += size;

	size = fArrayCount * sizeof(size_t);
	fBlockSize = (size_t *)malloc(size);
	fArrayLength = (size_t *)malloc(size);
	fArrayOffset = (size_t *)malloc(size);
	fOverhead += size * 3;

	size_t arraySlots = biggestSize / minSize;
	for (int32 i = 0; i < fArrayCount; i++) {
		size = arraySlots * minCountPerBlock * sizeof(uint8);
		fArrayLength[i] = arraySlots * minCountPerBlock;
		fBlockSize[i] = biggestSize / arraySlots;
		fArrayOffset[i] = fArrayLength[i] - 1;

		fArray[i] = (uint8 *)malloc(size);
		memset(fArray[i], 0, fArrayLength[i]);

		fOverhead += size;
		arraySlots /= 2;
	}

	fManagedMemory = fBlockSize[0] * fArrayLength[0];

	size_t roundedSize = biggestSize * minCountPerBlock;
	fDebugBase = roundedSize;
	fDebugChunkSize = 128;
	fDebugUseMap = 0;
	roundedSize += sizeof(fDebugUseMap) * 8 * fDebugChunkSize;
	roundedSize = (roundedSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	fArea = create_area(fName, &fLogicalBase, B_ANY_KERNEL_ADDRESS,
		roundedSize, B_32_BIT_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (fArea < B_OK) {
		TRACE_ERROR(("PMA: failed to create memory area\n"));
		return;
	}

	physical_entry physicalEntry;
	if (get_memory_map(fLogicalBase, roundedSize, &physicalEntry, 1) < B_OK) {
		TRACE_ERROR(("PMA: failed to get memory map\n"));
		return;
	}

	fPhysicalBase = physicalEntry.address;

	fNoMemoryCondition.Init(this, "USB PMA");
	fStatus = B_OK;
}


PhysicalMemoryAllocator::~PhysicalMemoryAllocator()
{
	mutex_lock(&fLock);

	for (int32 i = 0; i < fArrayCount; i++)
		free(fArray[i]);

	free(fArray);
	free(fArrayLength);
	free(fBlockSize);
	free(fArrayOffset);
	free(fName);

	delete_area(fArea);
	mutex_destroy(&fLock);
}


status_t
PhysicalMemoryAllocator::Allocate(size_t size, void **logicalAddress,
	phys_addr_t *physicalAddress)
{
	if (debug_debugger_running()) {
		if (size > fDebugChunkSize) {
			kprintf("usb allocation of %" B_PRIuSIZE
				" does not fit debug chunk size %" B_PRIu32 "!\n",
				size, fDebugChunkSize);
			return B_NO_MEMORY;
		}

		for (size_t i = 0; i < sizeof(fDebugUseMap) * 8; i++) {
			uint64 mask = 1LL << i;
			if ((fDebugUseMap & mask) == 0) {
				fDebugUseMap |= mask;
				*logicalAddress = (void *)((uint8 *)fLogicalBase + fDebugBase
					+ i * fDebugChunkSize);
				*physicalAddress = (phys_addr_t)(fPhysicalBase + fDebugBase
					+ i * fDebugChunkSize);
				return B_OK;
			}
		}

		return B_NO_MEMORY;
	}

	if (size == 0 || size > fBlockSize[fArrayCount - 1]) {
		TRACE_ERROR(("PMA: bad value for allocate (%ld bytes)\n", size));
		return B_BAD_VALUE;
	}

	size_t arrayLength = 0;
	int32 arrayToUse = 0;
	for (int32 i = 0; i < fArrayCount; i++) {
		if (fBlockSize[i] >= size) {
			arrayToUse = i;
			arrayLength = fArrayLength[i];
			break;
		}
	}

	MutexLocker locker(&fLock);
	if (!locker.IsLocked())
		return B_ERROR;

	const bigtime_t limit = system_time() + 2 * 1000 * 1000;
	do {
		TRACE(("PMA: will use array %ld (blocksize: %ld) to allocate %ld bytes\n", arrayToUse, fBlockSize[arrayToUse], size));
		uint8 *targetArray = fArray[arrayToUse];
		uint32 arrayOffset = fArrayOffset[arrayToUse] % arrayLength;
		for (size_t i = arrayOffset + 1; i != arrayOffset; i++) {
			if (i >= arrayLength)
				i -= arrayLength;

 			if (targetArray[i] == 0) {
				// found a free slot
				fArrayOffset[arrayToUse] = i;

				// fill upwards to the smallest block
				uint32 fillSize = 1;
				uint32 arrayIndex = i;
				for (int32 j = arrayToUse; j >= 0; j--) {
					memset(&fArray[j][arrayIndex], 1, fillSize);
					fillSize <<= 1;
					arrayIndex <<= 1;
				}

				// fill downwards to the biggest block
				arrayIndex = i >> 1;
				for (int32 j = arrayToUse + 1; j < fArrayCount; j++) {
					fArray[j][arrayIndex]++;
					if (fArray[j][arrayIndex] > 1)
						break;

					arrayIndex >>= 1;
				}

				size_t offset = fBlockSize[arrayToUse] * i;
				*logicalAddress = (void *)((uint8 *)fLogicalBase + offset);
				*physicalAddress = (phys_addr_t)(fPhysicalBase + offset);
				return B_OK;
			}
		}

		// no slot found, we need to wait

		ConditionVariableEntry entry;
		fNoMemoryCondition.Add(&entry);
		fMemoryWaitersCount++;

		locker.Unlock();

		TRACE_ERROR(("PMA: found no free slot to store %ld bytes, waiting\n",
			size));

		if (entry.Wait(B_RELATIVE_TIMEOUT, 1 * 1000 * 1000) == B_TIMED_OUT)
			break;

		if (!locker.Lock())
			return B_ERROR;

		fMemoryWaitersCount--;
	} while (system_time() < limit);

	TRACE_ERROR(("PMA: timed out waiting for a free slot, giving up\n"));
	return B_NO_MEMORY;
}


status_t
PhysicalMemoryAllocator::Deallocate(size_t size, void *logicalAddress,
	phys_addr_t physicalAddress)
{
	if (debug_debugger_running()) {
		uint32 index = ((uint8 *)logicalAddress - (uint8 *)fLogicalBase
			- fDebugBase) / fDebugChunkSize;
		fDebugUseMap &= ~(1LL << index);
		return B_OK;
	}

	if (size == 0 || size > fBlockSize[fArrayCount - 1]) {
		TRACE_ERROR(("PMA: bad value for deallocate (%ld bytes)\n", size));
		return B_BAD_VALUE;
	}

	int32 arrayToUse = 0;
	for (int32 i = 0; i < fArrayCount; i++) {
		if (fBlockSize[i] >= size) {
			arrayToUse = i;
			break;
		}
	}

	phys_addr_t offset;
	if (logicalAddress)
		offset = (addr_t)logicalAddress - (addr_t)fLogicalBase;
	else if (physicalAddress)
		offset = (addr_t)physicalAddress - fPhysicalBase;
	else {
		TRACE_ERROR(("PMA: no value given for either physical or logical address\n"));
		return B_BAD_VALUE;
	}

	uint32 index = offset / fBlockSize[arrayToUse];
	if (index >= fArrayLength[arrayToUse]) {
		TRACE_ERROR(("PMA: provided address resulted in invalid index\n"));
		return B_BAD_VALUE;
	}

	TRACE(("PMA: will use array %ld (index: %ld) to deallocate %ld bytes\n", arrayToUse, index, size));
	if (fArray[arrayToUse][index] == 0) {
		TRACE_ERROR(("PMA: address was not allocated!\n"));
		return B_BAD_VALUE;
	}

	MutexLocker _(&fLock);
	if (!_.IsLocked())
		return B_ERROR;

	// clear upwards to the smallest block
	uint32 fillSize = 1;
	uint32 arrayIndex = index;
	for (int32 i = arrayToUse; i >= 0; i--) {
		memset(&fArray[i][arrayIndex], 0, fillSize);
		fillSize <<= 1;
		arrayIndex <<= 1;
	}

	// clear downwards to the biggest block
	arrayIndex = index >> 1;
	for (int32 i = arrayToUse + 1; i < fArrayCount; i++) {
		fArray[i][arrayIndex]--;
		if (fArray[i][arrayIndex] > 0)
			break;

		arrayIndex >>= 1;
	}

	if (fMemoryWaitersCount > 0)
		fNoMemoryCondition.NotifyAll();

	return B_OK;
}


void
PhysicalMemoryAllocator::PrintToStream()
{
	dprintf("PhysicalMemoryAllocator \"%s\":\n", fName);
	dprintf("\tMin block size:\t\t\t%ld bytes\n", fBlockSize[0]);
	dprintf("\tMax block size:\t\t\t%ld bytes\n", fBlockSize[fArrayCount - 1]);
	dprintf("\tMin count per block:\t%ld\n\n", fArrayLength[fArrayCount - 1]);
	dprintf("\tArray count:\t\t\t%" B_PRId32 "\n", fArrayCount);

	dprintf("\tArray slots:\t\t\t% 8ld", fArrayLength[0]);
	for (int32 i = 1; i < fArrayCount; i++)
		dprintf(", % 8ld", fArrayLength[i]);
	dprintf("\n");
	DumpFreeSlots();

	dprintf("\tBlock sizes:\t\t\t% 8ld", fBlockSize[0]);
	for (int32 i = 1; i < fArrayCount; i++)
		dprintf(", % 8ld", fBlockSize[i]);
	dprintf("\n");
	DumpLastArray();
	dprintf("\n");

	dprintf("\tManaged memory:\t\t\t%ld bytes\n", fManagedMemory);
	dprintf("\tGranularity:\t\t\t%ld bytes\n", fBlockSize[0]);
	dprintf("\tMemory overhead:\t\t%ld bytes\n", fOverhead);
}


void
PhysicalMemoryAllocator::DumpArrays()
{
	uint32 padding = 2;
	for (int32 i = 0; i < fArrayCount; i++) {
		dprintf("\tArray(%" B_PRId32 "):\t", i);
		for (size_t j = 0; j < fArrayLength[i]; j++) {
			if (padding > 2) {
				for (uint32 k = 0; k < (padding - 2) / 4; k++)
					dprintf(" ");
				dprintf("\\");
				for (uint32 k = 0; k < (padding - 2) / 4; k++)
					dprintf("-");
				dprintf("%d", fArray[i][j]);
				for (uint32 k = 0; k < (padding - 2) / 4; k++)
					dprintf("-");
				dprintf("/");
				for (uint32 k = 0; k < (padding - 2) / 4 + 1; k++)
					dprintf(" ");
			} else {
				dprintf("%d ", fArray[i][j]);
			}
		}

		padding *= 2;
		dprintf("\n");
	}

	dprintf("\n");
}


void
PhysicalMemoryAllocator::DumpLastArray()
{
	dprintf("\tLast array:\t\t\t\t");
	for (size_t i = 0; i < fArrayLength[fArrayCount - 1]; i++)
		dprintf("%d", fArray[fArrayCount - 1][i]);
	dprintf("\n");
}


void
PhysicalMemoryAllocator::DumpFreeSlots()
{
	dprintf("\tFree slots:\t\t\t\t");
	for (int32 i = 0; i < fArrayCount; i++) {
		uint32 freeSlots = 0;
		for (size_t j = 0; j < fArrayLength[i]; j++) {
			if (fArray[i][j] == 0)
				freeSlots++;
		}

		if (i > 0)
			dprintf(", %8" B_PRIu32, freeSlots);
		else
			dprintf("%8" B_PRIu32, freeSlots);
	}
	dprintf("\n");
}
