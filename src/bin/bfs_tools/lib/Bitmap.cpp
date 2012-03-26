/*
 * Copyright 2001-2008, pinc Software. All Rights Reserved.
 */

//!	handles the BFS block bitmap


#include "Bitmap.h"
#include "Disk.h"
#include "Inode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


Bitmap::Bitmap(Disk *disk)
	:
	fBitmap(NULL),
	fBackupBitmap(NULL)
{
	SetTo(disk);
}


Bitmap::Bitmap()
	:
	fDisk(NULL),
	fBitmap(NULL),
	fBackupBitmap(NULL),
	fSize(0L),
	fByteSize(0L),
	fUsedBlocks(0LL)
{
}


Bitmap::~Bitmap()
{
	free(fBitmap);
	free(fBackupBitmap);
}


status_t
Bitmap::SetTo(Disk *disk)
{
	free(fBitmap);

	fDisk = disk;
	fSize = divide_roundup(disk->NumBlocks(),disk->BlockSize() * 8);
	fByteSize = disk->BlockSize() * disk->SuperBlock()->num_ags
		* disk->SuperBlock()->blocks_per_ag;

	fBitmap = (uint32 *)malloc(fByteSize);
	if (!fBitmap)
		return B_NO_MEMORY;

	fBackupBitmap = (uint32 *)malloc(fByteSize);
	if (fBackupBitmap == NULL)
		return B_NO_MEMORY;

	memset(fBackupBitmap, 0, fByteSize);

	// set root block, block bitmap, log as used
	off_t end = disk->ToBlock(disk->Log()) + disk->Log().length;
	for (off_t block = 0; block < end; block++) {
		if (!BackupSetAt(block, true))
			break;
	}

	ssize_t size;
	if ((size = disk->ReadAt(disk->BlockSize(), fBitmap, fByteSize)) < B_OK) {
		free(fBitmap);
		fBitmap = NULL;

		free(fBackupBitmap);
		fBackupBitmap = NULL;
		return size;
	}

	// calculate used blocks

	fUsedBlocks = 0LL;	
	for (uint32 i = fByteSize >> 2;i-- > 0;) {
		uint32 compare = 1;
		for (int16 j = 0;j < 32;j++,compare <<= 1) {
			if (compare & fBitmap[i])
				fUsedBlocks++;
		}
	}

	return B_OK;
}


status_t
Bitmap::InitCheck()
{
	return fBitmap ? B_OK : B_ERROR;
}


off_t
Bitmap::FreeBlocks() const
{
	if (fDisk == NULL)
		return 0;

	return fDisk->NumBlocks() - fUsedBlocks;
}


bool
Bitmap::UsedAt(off_t block) const
{
	uint32 index = block / 32;	// 32bit resolution, (beginning with block 1?)
	if (index > fByteSize / 4)
		return false;

	return fBitmap[index] & (1L << (block & 0x1f));
}


bool
Bitmap::BackupUsedAt(off_t block) const
{
	uint32 index = block / 32;	// 32bit resolution, (beginning with block 1?)
	if (index > fByteSize / 4)
		return false;

	return fBackupBitmap[index] & (1L << (block & 0x1f));
}


bool
Bitmap::BackupSetAt(off_t block, bool used)
{
	uint32 index = block / 32;
	if (index > fByteSize / 4) {
		fprintf(stderr, "Bitmap::BackupSetAt(): Block %Ld outside bitmap!\n",
			block);
		return false;
	}

	int32 mask = 1L << (block & 0x1f);

	bool oldUsed = fBackupBitmap[index] & mask;

	if (used)
		fBackupBitmap[index] |= mask;
	else
		fBackupBitmap[index] &= ~mask;

	return oldUsed;
}


void
Bitmap::BackupSet(Inode *inode, bool used)
{
	// set inode and its data-stream

	// the attributes are ignored for now, because they will
	// be added anyway since all inodes from disk are collected.
	
//	printf("a: %Ld\n",inode->Block());
	BackupSetAt(inode->Block(),used);

	// the data stream of symlinks is no real data stream	
	if (inode->IsSymlink() && (inode->Flags() & INODE_LONG_SYMLINK) == 0)
		return;

	// direct blocks

	const bfs_inode *node = inode->InodeBuffer();
	for (int32 i = 0; i < NUM_DIRECT_BLOCKS; i++) {
		if (node->data.direct[i].IsZero())
			break;

		off_t start = fDisk->ToBlock(node->data.direct[i]);
		off_t end = start + node->data.direct[i].length;
		for (off_t block = start; block < end; block++) {
			if (!BackupSetAt(block, used)) {
				//dump_inode(node);
				break;
			}
		}
	}

	// indirect blocks

	if (node->data.max_indirect_range == 0 || node->data.indirect.IsZero())
		return;

//	printf("c: %Ld\n",fDisk->ToBlock(node->data.indirect));
	BackupSetAt(fDisk->ToBlock(node->data.indirect), used);

	DataStream *stream = dynamic_cast<DataStream *>(inode);
	if (stream == NULL)
		return;

	// load indirect blocks
	int32 bytes = node->data.indirect.length << fDisk->BlockShift();
	block_run *indirect = (block_run *)malloc(bytes);
	if (indirect == NULL)
		return;

	if (fDisk->ReadAt(fDisk->ToOffset(node->data.indirect), indirect,
			bytes) <= 0)
		return;

	int32 runs = bytes / sizeof(block_run);
	for (int32 i = 0; i < runs; i++) {
		if (indirect[i].IsZero())
			break;

		off_t start = fDisk->ToBlock(indirect[i]);
		off_t end = start + indirect[i].length;
		for (off_t block = start; block < end; block++) {
//			printf("d: %Ld\n", block);
			if (!BackupSetAt(block, used))
				break;
		}
	}
	free(indirect);

	// double indirect blocks

	if (node->data.max_double_indirect_range == 0
		|| node->data.double_indirect.IsZero())
		return;

//	printf("e: %Ld\n",fDisk->ToBlock(node->data.double_indirect));
	BackupSetAt(fDisk->ToBlock(node->data.double_indirect), used);

	// FIXME: to be implemented...
	puts("double indirect blocks to block bitmap requested...");
}


status_t
Bitmap::Validate()
{
	return B_OK;
}


status_t
Bitmap::CompareWithBackup()
{
	for (int32 i = fByteSize / 4;i-- > 0;) {
		if (fBitmap[i] != fBackupBitmap[i]) {
			printf("differ at %ld (block %ld) -> bitmap = %08lx, backup = %08lx\n",i,i*32,fBitmap[i],fBackupBitmap[i]);
		}
	}
	return B_OK;
}


bool
Bitmap::TrustBlockContaining(off_t /*block*/) const
{
	return true;
}

