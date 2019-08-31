/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "AllocationInfo.h"

#include "DebugSupport.h"

#include "Attribute.h"
#include "Directory.h"
#include "Entry.h"
#include "File.h"
#include "SymLink.h"

// constructor
AllocationInfo::AllocationInfo()
	: fNodeTableArraySize(0),
	  fNodeTableVectorSize(0),
	  fNodeTableElementCount(0),
	  fDirectoryEntryTableArraySize(0),
	  fDirectoryEntryTableVectorSize(0),
	  fDirectoryEntryTableElementCount(0),

	  fAttributeCount(0),
	  fAttributeSize(0),
	  fDirectoryCount(0),
	  fEntryCount(0),
	  fFileCount(0),
	  fFileSize(0),
	  fSymLinkCount(0),
	  fSymLinkSize(0),

	  fAreaCount(0),
	  fAreaSize(0),
	  fBlockCount(0),
	  fBlockSize(0),
	  fListCount(0),
	  fListSize(0),
	  fOtherCount(0),
	  fOtherSize(0),
	  fStringCount(0),
	  fStringSize(0)
{
}

// destructor
AllocationInfo::~AllocationInfo()
{
}

// AddNodeTableAllocation
void
AllocationInfo::AddNodeTableAllocation(size_t arraySize, size_t vectorSize,
									   size_t elementSize, size_t elementCount)
{
	fNodeTableArraySize += arraySize;
	fNodeTableVectorSize += vectorSize * elementSize;
	fNodeTableElementCount += elementCount;
}

// AddDirectoryEntryTableAllocation
void
AllocationInfo::AddDirectoryEntryTableAllocation(size_t arraySize,
												 size_t vectorSize,
												 size_t elementSize,
												 size_t elementCount)
{
	fDirectoryEntryTableArraySize += arraySize;
	fDirectoryEntryTableVectorSize += vectorSize * elementSize;
	fDirectoryEntryTableElementCount += elementCount;
}

// AddAttributeAllocation
void
AllocationInfo::AddAttributeAllocation(size_t size)
{
	fAttributeCount++;
	fAttributeSize += size;
}

// AddDirectoryAllocation
void
AllocationInfo::AddDirectoryAllocation()
{
	fDirectoryCount++;
}

// AddEntryAllocation
void
AllocationInfo::AddEntryAllocation()
{
	fEntryCount++;
}

// AddFileAllocation
void
AllocationInfo::AddFileAllocation(size_t size)
{
	fFileCount++;
	fFileSize += size;
}

// AddSymLinkAllocation
void
AllocationInfo::AddSymLinkAllocation(size_t size)
{
	fSymLinkCount++;
	fSymLinkSize += size;
}

// AddAreaAllocation
void
AllocationInfo::AddAreaAllocation(size_t size, size_t count)
{
	fAreaCount += count;
	fAreaSize += count * size;
}

// AddBlockAllocation
void
AllocationInfo::AddBlockAllocation(size_t size)
{
	fBlockCount++;
	fBlockSize += size;
}

// AddListAllocation
void
AllocationInfo::AddListAllocation(size_t capacity, size_t elementSize)
{
	fListCount += 1;
	fListSize += capacity * elementSize;
}

// AddOtherAllocation
void
AllocationInfo::AddOtherAllocation(size_t size, size_t count)
{
	fOtherCount += count;
	fOtherSize += size * count;
}

// AddStringAllocation
void
AllocationInfo::AddStringAllocation(size_t size)
{
	fStringCount++;
	fStringSize += size;
}

// Dump
void
AllocationInfo::Dump() const
{
	size_t heapCount = 0;
	size_t heapSize = 0;
	size_t areaCount = 0;
	size_t areaSize = 0;

	PRINT("  node table:\n");
	PRINT("    array size:  %9lu\n", fNodeTableArraySize);
	PRINT("    vector size: %9lu\n", fNodeTableVectorSize);
	PRINT("    elements:    %9lu\n", fNodeTableElementCount);
	areaCount += 2;
	areaSize += fNodeTableArraySize * sizeof(int32) + fNodeTableVectorSize;

	PRINT("  entry table:\n");
	PRINT("    array size:  %9lu\n", fDirectoryEntryTableArraySize);
	PRINT("    vector size: %9lu\n", fDirectoryEntryTableVectorSize);
	PRINT("    elements:    %9lu\n", fDirectoryEntryTableElementCount);
	areaCount += 2;
	areaSize += fDirectoryEntryTableArraySize * sizeof(int32)
				+ fDirectoryEntryTableVectorSize;

	PRINT("  attributes:  %9lu, size: %9lu\n", fAttributeCount, fAttributeSize);
	heapCount += fAttributeCount;
	heapSize += fAttributeCount * sizeof(Attribute);

	PRINT("  directories: %9lu\n", fDirectoryCount);
	heapCount += fDirectoryCount;
	heapSize += fDirectoryCount * sizeof(Directory);

	PRINT("  entries:     %9lu\n", fEntryCount);
	heapCount += fEntryCount;
	heapSize += fEntryCount * sizeof(Entry);

	PRINT("  files:       %9lu, size: %9lu\n", fFileCount, fFileSize);
	heapCount += fFileCount;
	heapSize += fFileCount * sizeof(File);

	PRINT("  symlinks:    %9lu, size: %9lu\n", fSymLinkCount, fSymLinkSize);
	heapCount += fSymLinkCount;
	heapSize += fSymLinkCount * sizeof(SymLink);

	PRINT("  areas:       %9lu, size: %9lu\n", fAreaCount, fAreaSize);
	areaCount += fAreaCount;
	areaSize += fAreaSize;

	PRINT("  blocks:      %9lu, size: %9lu\n", fBlockCount, fBlockSize);

	PRINT("  lists:       %9lu, size: %9lu\n", fListCount, fListSize);
	heapCount += fListCount;
	heapSize += fListSize;

	PRINT("  other:       %9lu, size: %9lu\n", fOtherCount, fOtherSize);
	heapCount += fOtherCount;
	heapSize += fOtherSize;

	PRINT("  strings:     %9lu, size: %9lu\n", fStringCount, fStringSize);
	heapCount += fStringCount;
	heapSize += fStringSize;

	PRINT("heap:  %9lu allocations, size: %9lu\n", heapCount, heapSize);
	PRINT("areas: %9lu allocations, size: %9lu\n", areaCount, areaSize);
}

