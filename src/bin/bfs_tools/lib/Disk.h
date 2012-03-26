#ifndef DISK_H
#define DISK_H
/* Disk - handles BFS super block, disk access etc.
**
** Copyright (c) 2001-2003 pinc Software. All Rights Reserved.
*/


#include <File.h>
#include <Path.h>

#include <string.h>

#include "Bitmap.h"
#include "bfs.h"
#include "Cache.h"


class Disk;

class BlockRunCache : public Cache<block_run>
{
	public:
		BlockRunCache(Disk *disk);
//		~BlockRunCache();
		
		virtual Cacheable *NewCacheable(block_run run);
	
	protected:
		Disk	*fDisk;
};


class Disk : public BPositionIO
{
	public:
		Disk(const char *device, bool rawMode = false, off_t start = 0, off_t stop = -1);
		virtual ~Disk();

		status_t			InitCheck();
		const BPath			&Path() const { return fPath; }

		off_t				Size() const { return fSize; }
		off_t				NumBlocks() const { return fSuperBlock.num_blocks; }
		uint32				BlockSize() const { return fSuperBlock.block_size; }
		uint32				BlockShift() const { return fSuperBlock.block_shift; }
		uint32				AllocationGroups() const { return fSuperBlock.num_ags; }
		uint32				AllocationGroupShift() const { return fSuperBlock.ag_shift; }
		uint32				BitmapSize() const { return fBitmap.Size(); }
		off_t				LogSize() const;

		disk_super_block	*SuperBlock() { return &fSuperBlock; }
		block_run			Root() const { return fSuperBlock.root_dir; }
		block_run			Indices() const { return fSuperBlock.indices; }
		block_run			Log() const { return fSuperBlock.log_blocks; }
		Bitmap				*BlockBitmap() { return &fBitmap; }

		const char			*Name() const { return fSuperBlock.name; }
		void				SetName(const char *name) { strcpy(fSuperBlock.name,name); }

		off_t				ToOffset(block_run run) const { return ToBlock(run) << fSuperBlock.block_shift; }
		off_t				ToBlock(block_run run) const { return ((((off_t)run.allocation_group) << fSuperBlock.ag_shift) | (off_t)run.start); }
		block_run			ToBlockRun(off_t start,int16 length = 1) const;

		uint8				*ReadBlockRun(block_run run);

		status_t			ScanForSuperBlock(off_t start = 0,off_t stop = -1);
		status_t			ValidateSuperBlock();
		status_t			RecreateSuperBlock();

		status_t			DumpBootBlockToFile();

		// BPositionIO methods
		virtual ssize_t		Read(void *buffer, size_t size);
		virtual ssize_t		Write(const void *buffer, size_t size);

		virtual ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
		virtual ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

		virtual off_t		Seek(off_t position, uint32 seek_mode);
		virtual off_t		Position() const;

		virtual status_t	SetSize(off_t size);

	protected:
		status_t			GetNextSpecialInode(char *,off_t *,off_t,bool);
		void				SaveInode(bfs_inode *,bool *,bfs_inode *,bool *,bfs_inode *);
		status_t			ScanForIndexAndRoot(bfs_inode *,bfs_inode *);
		status_t			DetermineBlockSize();
		status_t			ValidateSuperBlock(disk_super_block &superBlock);

		status_t			LoadBootBlock();

		BFile				fFile;
		BPath				fPath;
		off_t				fRawDiskOffset;
		off_t				fSize;
		disk_super_block	fSuperBlock;
		Bitmap				fBitmap;

		block_run			fValidBlockRun;
		off_t				fValidOffset;
		off_t				fLogStart;
		
		BlockRunCache		fCache;

		bool				fRawMode;
};

#endif	/* DISK_H */
