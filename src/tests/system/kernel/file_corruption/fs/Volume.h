/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <fs_interface.h>
#include <fs_volume.h>


class Volume {
public:
								Volume(uint32 flags);
								~Volume();

			status_t			Init(const char* device);
			status_t			Init(int fd, uint64 totalBlocks);

			status_t			Mount();
			status_t			Initialize(const char* name);

	inline	bool				IsReadOnly() const;
	inline	uint64				TotalBlocks() const	{ return fTotalBlocks; }
	inline	void*				BlockCache() const	{ return fBlockCache; }
	inline	const char*			Name() const		{ return fName; }

private:
			status_t			_Init(uint64 totalBlocks);

private:
			int					fFD;
			uint32				fFlags;
			void*				fBlockCache;
			uint64				fTotalBlocks;
			char*				fName;
};


bool
Volume::IsReadOnly() const
{
	return (fFlags & B_MOUNT_READ_ONLY) != 0;
}


#endif	// VOLUME_H
