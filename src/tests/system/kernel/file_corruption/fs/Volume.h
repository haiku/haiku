/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <fs_info.h>
#include <fs_interface.h>
#include <fs_volume.h>


class BlockAllocator;
class Directory;
class Node;


class Volume {
public:
								Volume(uint32 flags);
								~Volume();

			status_t			Init(const char* device);
			status_t			Init(int fd, uint64 totalBlocks);

			status_t			Mount(fs_volume* fsVolume);
			void				Unmount();

			status_t			Initialize(const char* name);

			void				GetInfo(fs_info& info);

			status_t			PublishNode(Node* node, uint32 flags);
			status_t			GetNode(uint64 blockIndex, Node*& _node);
			status_t			PutNode(Node* node);

			status_t			ReadNode(uint64 blockIndex, Node*& _node);

			status_t			CreateDirectory(mode_t mode,
									Directory*& _directory);

	inline	dev_t				ID() const			{ return fFSVolume->id; }
	inline	bool				IsReadOnly() const;
	inline	uint64				TotalBlocks() const	{ return fTotalBlocks; }
	inline	void*				BlockCache() const	{ return fBlockCache; }
	inline	const char*			Name() const		{ return fName; }
	inline	BlockAllocator*		GetBlockAllocator() const
									{ return fBlockAllocator; }
	inline	Directory*			RootDirectory() const
									{ return fRootDirectory; }

private:
			status_t			_Init(uint64 totalBlocks);

private:
			fs_volume*			fFSVolume;
			int					fFD;
			uint32				fFlags;
			void*				fBlockCache;
			uint64				fTotalBlocks;
			char*				fName;
			BlockAllocator*		fBlockAllocator;
			Directory*			fRootDirectory;
};


bool
Volume::IsReadOnly() const
{
	return (fFlags & B_FS_IS_READONLY) != 0;
}


#endif	// VOLUME_H
