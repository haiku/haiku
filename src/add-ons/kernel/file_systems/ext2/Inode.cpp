/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Inode.h"

#include <fs_cache.h>

#include "CachedBlock.h"


Inode::Inode(Volume* volume, ino_t id)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL),
	fNode(NULL)
{
	rw_lock_init(&fLock, "ext2 inode");

	uint32 block;
	if (volume->GetInodeBlock(id, block) == B_OK) {
		ext2_inode* inodes = (ext2_inode*)block_cache_get(volume->BlockCache(),
			block);
		if (inodes != NULL)
			fNode = inodes + volume->InodeBlockIndex(id);
	}

	if (fNode != NULL) {
		fCache = file_cache_create(fVolume->ID(), ID(), Size());
		fMap = file_map_create(fVolume->ID(), ID(), Size());
	}
}


Inode::~Inode()
{
	file_cache_delete(FileCache());
	file_map_delete(Map());

	if (fNode != NULL) {
		uint32 block;
		if (fVolume->GetInodeBlock(ID(), block) == B_OK)
			block_cache_put(fVolume->BlockCache(), block);
	}
}


status_t
Inode::InitCheck()
{
	return fNode != NULL ? B_OK : B_ERROR;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	uid_t user = geteuid();
	gid_t group = getegid();

	// you never have write access to a read-only volume
	if (accessMode & W_OK && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// root users always have full access (but they can't execute files without
	// any execute permissions set)
	if (user == 0) {
		if (!((accessMode & X_OK) != 0 && (Mode() & S_IXUSR) == 0)
			|| S_ISDIR(Mode()))
			return B_OK;
	}

	// shift mode bits, to check directly against accessMode
	mode_t mode = Mode();
	if (user == (uid_t)fNode->UserID())
		mode >>= 6;
	else if (group == (gid_t)fNode->GroupID())
		mode >>= 3;

	if (accessMode & ~(mode & S_IRWXO))
		return B_NOT_ALLOWED;

	return B_OK;
}


status_t
Inode::FindBlock(off_t offset, uint32& block)
{
	uint32 perBlock = fVolume->BlockSize() / 4;
	uint32 perIndirectBlock = perBlock * perBlock;
	uint32 index = offset >> fVolume->BlockShift();

	if (offset >= Size())
		return B_ENTRY_NOT_FOUND;

	if (index < EXT2_DIRECT_BLOCKS) {
		// direct blocks
		block = B_LENDIAN_TO_HOST_INT32(Node().stream.direct[index]);
	} else if ((index -= EXT2_DIRECT_BLOCKS) < perBlock) {
		// indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(Node().stream.indirect);
		if (indirectBlocks != NULL)
			return B_IO_ERROR;

		block = B_LENDIAN_TO_HOST_INT32(indirectBlocks[index]);
	} else if ((index -= perBlock) < perIndirectBlock) {
		// double indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			Node().stream.double_indirect));
		if (indirectBlocks != NULL)
			return B_IO_ERROR;

		indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			indirectBlocks[index / perBlock]));
		if (indirectBlocks != NULL)
			return B_IO_ERROR;

		block = B_LENDIAN_TO_HOST_INT32(indirectBlocks[index & (perBlock - 1)]);
	} else if ((index -= perIndirectBlock) / perBlock < perIndirectBlock) {
		// triple indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			Node().stream.triple_indirect));
		if (indirectBlocks != NULL)
			return B_IO_ERROR;

		indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			indirectBlocks[index / perIndirectBlock]));
		if (indirectBlocks != NULL)
			return B_IO_ERROR;

		indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			indirectBlocks[(index / perBlock) & (perBlock - 1)]));
		if (indirectBlocks != NULL)
			return B_IO_ERROR;

		block = B_LENDIAN_TO_HOST_INT32(indirectBlocks[index & (perBlock - 1)]);
	} else {
		// outside of the possible data stream
		dprintf("ext2: block outside datastream!\n");
		return B_ERROR;
	}

	//dprintf("FindBlock(offset %Ld): %lu\n", offset, block);
	return B_OK;
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0)
		return B_BAD_VALUE;

	if (pos >= Size() || length == 0) {
		*_length = 0;
		return B_NO_ERROR;
	}

	return file_cache_read(FileCache(), NULL, pos, buffer, _length);
}

