/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019-2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef DATA_CONTAINER_H
#define DATA_CONTAINER_H


#include <OS.h>


struct vm_page;
class VMCache;
class AllocationInfo;
class Volume;


class DataContainer {
public:
	DataContainer(Volume *volume);
	virtual ~DataContainer();

	status_t InitCheck() const;

	Volume *GetVolume() const	{ return fVolume; }

	status_t Resize(off_t newSize);
	off_t GetSize() const { return fSize; }

	VMCache* GetCache(struct vnode* vnode);

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
	status_t _DoCacheIO(const off_t offset, uint8* buffer, ssize_t length,
		size_t* bytesProcessed, bool isWrite);

	inline int32 _CountBlocks() const;

private:
	Volume				*fVolume;
	off_t				fSize;
	VMCache*			fCache;

	uint8*				fSmallBuffer;
	off_t				fSmallBufferSize;
};


#endif	// DATA_CONTAINER_H
