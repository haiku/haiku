/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ALLOCATION_INFO_H
#define ALLOCATION_INFO_H

#include <SupportDefs.h>

class AllocationInfo {
public:
	AllocationInfo();
	~AllocationInfo();

	void AddNodeTableAllocation(size_t arraySize, size_t vectorSize,
								size_t elementSize, size_t elementCount);
	void AddDirectoryEntryTableAllocation(size_t arraySize, size_t vectorSize,
										  size_t elementSize,
										  size_t elementCount);

	void AddAttributeAllocation(size_t size);
	void AddDirectoryAllocation();
	void AddEntryAllocation();
	void AddFileAllocation(size_t size);
	void AddSymLinkAllocation(size_t size);

	void AddAreaAllocation(size_t size, size_t count = 1);
	void AddBlockAllocation(size_t size);
	void AddListAllocation(size_t capacity, size_t elementSize);
	void AddOtherAllocation(size_t size, size_t count = 1);
	void AddStringAllocation(size_t size);

	void Dump() const;

private:
	size_t	fNodeTableArraySize;
	size_t	fNodeTableVectorSize;
	size_t	fNodeTableElementCount;
	size_t	fDirectoryEntryTableArraySize;
	size_t	fDirectoryEntryTableVectorSize;
	size_t	fDirectoryEntryTableElementCount;

	size_t	fAttributeCount;
	size_t	fAttributeSize;
	size_t	fDirectoryCount;
	size_t	fEntryCount;
	size_t	fFileCount;
	size_t	fFileSize;
	size_t	fSymLinkCount;
	size_t	fSymLinkSize;

	size_t	fAreaCount;
	size_t	fAreaSize;
	size_t	fBlockCount;
	size_t	fBlockSize;
	size_t	fListCount;
	size_t	fListSize;
	size_t	fOtherCount;
	size_t	fOtherSize;
	size_t	fStringCount;
	size_t	fStringSize;
};

#endif	// ALLOCATION_INFO_H
