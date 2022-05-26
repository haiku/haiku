/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Inode.h"
#include <string.h>


#define TRACE_UFS2
#ifdef TRACE_UFS2
#define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mufs2:\33[0m " x)


Inode::Inode(Volume* volume, ino_t id)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL)
{
	rw_lock_init(&fLock, "ufs2 inode");

	fInitStatus = B_OK;//UpdateNodeFromDisk();
	if (fInitStatus == B_OK) {
		if (!IsDirectory() && !IsSymLink()) {
			fCache = file_cache_create(fVolume->ID(), ID(), Size());
			fMap = file_map_create(fVolume->ID(), ID(), Size());
		}
	}
	int fd = fVolume->Device();
	ufs2_super_block* superblock = (ufs2_super_block* )&fVolume->SuperBlock();
	int64_t fs_block = ino_to_fsba(superblock, id);
	int64_t offset_in_block = ino_to_fsbo(superblock, id);
	int64_t offset = fs_block * MINBSIZE + offset_in_block * sizeof(fNode);

	if (read_pos(fd, offset, (void*)&fNode, sizeof(fNode)) != sizeof(fNode))
		ERROR("Inode::Inode(): IO Error\n");


}


Inode::Inode(Volume* volume, ino_t id, const ufs2_inode& item)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL),
	fInitStatus(B_OK),
	fNode(item)
{
	if (!IsDirectory() && !IsSymLink()) {
		fCache = file_cache_create(fVolume->ID(), ID(), Size());
		fMap = file_map_create(fVolume->ID(), ID(), Size());
	}
}


Inode::Inode(Volume* volume)
	:
	fVolume(volume),
	fID(0),
	fCache(NULL),
	fMap(NULL),
	fInitStatus(B_NO_INIT)
{
	rw_lock_init(&fLock, "ufs2 inode");
}


Inode::~Inode()
{
	TRACE("Inode destructor\n");
	file_cache_delete(FileCache());
	file_map_delete(Map());
	TRACE("Inode destructor: Done\n");
}


status_t
Inode::InitCheck()
{
	return fInitStatus;
}


status_t
Inode::ReadAt(off_t file_offset, uint8* buffer, size_t* _length)
{
	int fd = fVolume->Device();
	ufs2_super_block super_block = fVolume->SuperBlock();
	int32_t blockSize = super_block.fs_bsize;
	off_t pos;
	int64_t size = Size();
	off_t startBlockNumber = file_offset / blockSize;
	off_t endBlockNumber = (file_offset + *_length) / blockSize;
	off_t blockOffset = file_offset % blockSize;
	ssize_t length = 0;
	if (startBlockNumber != endBlockNumber) {
		ssize_t remainingLength = blockSize - blockOffset;
		for (; startBlockNumber <= endBlockNumber; startBlockNumber++) {
			//code for reading multiple blocks
			pos = FindBlock(startBlockNumber, blockOffset);
			length += read_pos(fd, pos, buffer, remainingLength);
			blockOffset = 0;
			remainingLength = *_length - length;
			if (remainingLength > blockSize)
				remainingLength = blockSize;
		}
		*_length = length;
		return B_OK;
	}
	pos = FindBlock(startBlockNumber, blockOffset);
	length = read_pos(fd, pos, buffer, *_length);
	*_length = length;
	return B_OK;

}


off_t
Inode::FindBlock(off_t blockNumber, off_t blockOffset)
{
	int fd = fVolume->Device();
	ufs2_super_block super_block = fVolume->SuperBlock();
	int32_t blockSize = super_block.fs_bsize;
	int32_t fragmentSize = super_block.fs_fsize;
	off_t indirectOffset;
	int64_t directBlock;
	off_t numberOfBlockPointers = blockSize / 8;
	const off_t numberOfIndirectBlocks = numberOfBlockPointers;
	const off_t numberOfDoubleIndirectBlocks = numberOfBlockPointers
		* numberOfBlockPointers;
	if (blockNumber < 12) {
		// read from direct block
		return GetBlockPointer(blockNumber) * fragmentSize + blockOffset;

	} else if (blockNumber < numberOfIndirectBlocks + 12) {
		//read from indirect block
		blockNumber = blockNumber - 12;
		indirectOffset = GetIndirectBlockPointer() *
			fragmentSize + (8 * blockNumber);
		read_pos(fd, indirectOffset,
			(void*)&directBlock, sizeof(directBlock));

		return directBlock * fragmentSize + blockOffset;

	} else if (blockNumber < numberOfDoubleIndirectBlocks
			+ numberOfIndirectBlocks + 12) {
		// Data is in double indirect block
		// Subract the already read blocks
		blockNumber = blockNumber - numberOfBlockPointers - 12;
		// Calculate indirect block inside double indirect block
		off_t indirectBlockNumber = blockNumber / numberOfBlockPointers;
		indirectOffset = GetDoubleIndirectBlockPtr() *
			fragmentSize + (8 * indirectBlockNumber);

		int64_t indirectPointer;
		read_pos(fd, indirectOffset,
			(void*)&indirectPointer, sizeof(directBlock));

		indirectOffset = indirectPointer * fragmentSize
			+ (8 * (blockNumber % numberOfBlockPointers));

		read_pos(fd, indirectOffset,
			(void*)&directBlock, sizeof(directBlock));

		return directBlock * fragmentSize + blockOffset;

	} else if (blockNumber < (numberOfIndirectBlocks
				* numberOfDoubleIndirectBlocks)
				+ (numberOfDoubleIndirectBlocks + numberOfBlockPointers + 12)) {
		// Reading from triple indirect block
		blockNumber = blockNumber - numberOfDoubleIndirectBlocks
			- numberOfBlockPointers - 12;

		// Get double indirect block
		// Double indirect block no
		off_t indirectBlockNumber = blockNumber / numberOfDoubleIndirectBlocks;

		// offset to double indirect block ptr
		indirectOffset = GetTripleIndirectBlockPtr() *
			fragmentSize + (8 * indirectBlockNumber);

		int64_t indirectPointer;
		// Get the double indirect block ptr
		read_pos(fd, indirectOffset,
			(void*)&indirectPointer, sizeof(directBlock));

		// Get the indirect block
		// number of indirect block ptr
		indirectBlockNumber = blockNumber / numberOfBlockPointers;
		// Indirect block ptr offset
		indirectOffset = indirectPointer * fragmentSize
			+ (8 * indirectBlockNumber);

		read_pos(fd, indirectOffset,
			(void*)&indirectPointer, sizeof(directBlock));

		// Get direct block pointer
		indirectOffset = indirectPointer * fragmentSize
			+ (8 * (blockNumber % numberOfBlockPointers));

		read_pos(fd, indirectOffset,
			(void*)&directBlock, sizeof(directBlock));

		return directBlock * fragmentSize + blockOffset;
	}

	return B_BAD_VALUE;
}


status_t
Inode::ReadLink(char* buffer, size_t *_bufferSize)
{
	strlcpy(buffer, fNode.symlinkpath, *_bufferSize);
	return B_OK;
}
