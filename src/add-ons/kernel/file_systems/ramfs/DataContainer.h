/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DATA_CONTAINER_H
#define DATA_CONTAINER_H

#include <OS.h>

struct vm_page;
class VMCache;
class AllocationInfo;
class Volume;

// Size of the DataContainer's small buffer. If it contains data up to this
// size, no blocks are allocated, but the small buffer is used instead.
// 16 bytes are for free, since they are shared with the block list.
// (actually even more, since the list has an initial size).
// I ran a test analyzing what sizes the attributes in my system have:
//     size   percentage   bytes used in average
//   <=   0         0.00                   93.45
//   <=   4        25.46                   75.48
//   <=   8        30.54                   73.02
//   <=  16        52.98                   60.37
//   <=  32        80.19                   51.74
//   <=  64        94.38                   70.54
//   <= 126        96.90                  128.23
//
// For average memory usage it is assumed, that attributes larger than 126
// bytes have size 127, that the list has an initial capacity of 10 entries
// (40 bytes), that the block reference consumes 4 bytes and the block header
// 12 bytes. The optimal length is actually 35, with 51.05 bytes per
// attribute, but I conservatively rounded to 32.
static const size_t kSmallDataContainerSize = 32;

class DataContainer {
public:
	DataContainer(Volume *volume);
	virtual ~DataContainer();

	status_t InitCheck() const;

	Volume *GetVolume() const	{ return fVolume; }

	status_t Resize(off_t newSize);
	off_t GetSize() const { return fSize; }

	VMCache* GetCache();

	virtual status_t ReadAt(off_t offset, void *buffer, size_t size,
							size_t *bytesRead);
	virtual status_t WriteAt(off_t offset, const void *buffer, size_t size,
							 size_t *bytesWritten);

	// debugging
	void GetAllocationInfo(AllocationInfo &info);

private:
	inline bool _RequiresCacheMode(size_t size);
	inline bool _IsCacheMode() const;
	status_t _SwitchToCacheMode();
	void _GetPages(off_t offset, off_t length, bool isWrite, vm_page** pages);
	void _PutPages(off_t offset, off_t length, vm_page** pages, bool success);
	status_t _DoCacheIO(const off_t offset, uint8* buffer, ssize_t length,
		size_t* bytesProcessed, bool isWrite);

	inline int32 _CountBlocks() const;

private:
	Volume				*fVolume;
	off_t				fSize;
	VMCache*			fCache;
	uint8				fSmallBuffer[kSmallDataContainerSize];
};

#endif	// DATA_CONTAINER_H
