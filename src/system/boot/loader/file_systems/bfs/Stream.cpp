/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//!	Inode stream access functions


#include "Stream.h"
#include "Directory.h"
#include "File.h"
#include "Link.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>


using namespace BFS;
using std::nothrow;


class CachedBlock {
public:
								CachedBlock(Volume& volume);
								CachedBlock(Volume& volume, block_run run);
								~CachedBlock();

			uint8*				SetTo(block_run run);
			uint8*				SetTo(off_t offset);

			void				Unset();

			uint8*				Block() const { return fBlock; }
			off_t				BlockNumber() const { return fBlockNumber; }
			uint32				BlockSize() const { return fVolume.BlockSize(); }
			uint32				BlockShift() const
									{ return fVolume.BlockShift(); }

private:
			Volume&				fVolume;
			off_t				fBlockNumber;
			uint8*				fBlock;
};


CachedBlock::CachedBlock(Volume& volume)
	:
	fVolume(volume),
	fBlockNumber(-1LL),
	fBlock(NULL)
{
}


CachedBlock::CachedBlock(Volume &volume, block_run run)
	:
	fVolume(volume),
	fBlockNumber(-1LL),
	fBlock(NULL)
{
	SetTo(run);
}


CachedBlock::~CachedBlock()
{
	free(fBlock);
}


inline void
CachedBlock::Unset()
{
	fBlockNumber = -1;
}


inline uint8*
CachedBlock::SetTo(off_t block)
{
	if (block == fBlockNumber)
		return fBlock;
	if (fBlock == NULL) {
		fBlock = (uint8*)malloc(BlockSize());
		if (fBlock == NULL)
			return NULL;
	}

	fBlockNumber = block;
	if (read_pos(fVolume.Device(), block << BlockShift(), fBlock, BlockSize())
			< (ssize_t)BlockSize())
		return NULL;

	return fBlock;
}


inline uint8*
CachedBlock::SetTo(block_run run)
{
	return SetTo(fVolume.ToBlock(run));
}


//	#pragma mark -


Stream::Stream(Volume& volume, block_run run)
	:
	fVolume(volume)
{
	if (read_pos(volume.Device(), volume.ToOffset(run), this, sizeof(bfs_inode))
			!= sizeof(bfs_inode))
		return;
}


Stream::Stream(Volume& volume, off_t id)
	:
	fVolume(volume)
{
	if (read_pos(volume.Device(), volume.ToOffset(id), this, sizeof(bfs_inode))
			!= sizeof(bfs_inode))
		return;
}


Stream::~Stream()
{
}


status_t
Stream::InitCheck()
{
	return bfs_inode::InitCheck(&fVolume);
}


status_t
Stream::GetNextSmallData(const small_data** _smallData) const
{
	// TODO: Stream derives from bfs_inode and we read only sizeof(bfs_inode)
	// bytes from disk, i.e. the small data region is not in memory.
	panic("Stream::GetNextSmallData(): small data region is not loaded!");

	const small_data* smallData = *_smallData;

	// begin from the start?
	if (smallData == NULL)
		smallData = small_data_start;
	else
		smallData = smallData->Next();

	// is already last item?
	if (smallData->IsLast(this))
		return B_ENTRY_NOT_FOUND;

	*_smallData = smallData;

	return B_OK;
}


status_t
Stream::GetName(char* name, size_t size) const
{
	const small_data* smallData = NULL;
	while (GetNextSmallData(&smallData) == B_OK) {
		if (*smallData->Name() == FILE_NAME_NAME
			&& smallData->NameSize() == FILE_NAME_NAME_LENGTH) {
			strlcpy(name, (const char*)smallData->Data(), size);
			return B_OK;
		}
	}
	return B_ERROR;
}


status_t
Stream::ReadLink(char* buffer, size_t bufferSize)
{
	// link in the stream

	if (Flags() & INODE_LONG_SYMLINK)
		return ReadAt(0, (uint8*)buffer, &bufferSize);

	// link in the inode

	strlcpy(buffer, short_symlink, bufferSize);
	return B_OK;
}


status_t
Stream::FindBlockRun(off_t pos, block_run& run, off_t& offset)
{
	// find matching block run

	if (data.MaxDirectRange() > 0 && pos >= data.MaxDirectRange()) {
		if (data.MaxDoubleIndirectRange() > 0
			&& pos >= data.MaxIndirectRange()) {
			// access to double indirect blocks

			CachedBlock cached(fVolume);

			int32 runsPerBlock;
			int32 directSize;
			int32 indirectSize;
			get_double_indirect_sizes(data.double_indirect.Length(),
				cached.BlockSize(), runsPerBlock, directSize, indirectSize);

			off_t start = pos - data.MaxIndirectRange();
			int32 index = start / indirectSize;

			block_run* indirect = (block_run*)cached.SetTo(
				fVolume.ToBlock(data.double_indirect) + index / runsPerBlock);
			if (indirect == NULL)
				return B_ERROR;

			//printf("\tstart = %Ld, indirectSize = %ld, directSize = %ld, index = %ld\n",start,indirectSize,directSize,index);
			//printf("\tlook for indirect block at %ld,%d\n",indirect[index].allocation_group,indirect[index].start);

			int32 current = (start % indirectSize) / directSize;

			indirect = (block_run*)cached.SetTo(fVolume.ToBlock(indirect[
					index % runsPerBlock]) + current / runsPerBlock);
			if (indirect == NULL)
				return B_ERROR;

			run = indirect[current % runsPerBlock];
			offset = data.MaxIndirectRange() + (index * indirectSize)
				+ (current * directSize);
			//printf("\tfCurrent = %ld, fRunFileOffset = %Ld, fRunBlockEnd = %Ld, fRun = %ld,%d\n",fCurrent,fRunFileOffset,fRunBlockEnd,fRun.allocation_group,fRun.start);
		} else {
			// access to indirect blocks

			int32 runsPerBlock = fVolume.BlockSize() / sizeof(block_run);
			off_t runBlockEnd = data.MaxDirectRange();

			CachedBlock cached(fVolume);
			off_t block = fVolume.ToBlock(data.indirect);

			for (int32 i = 0; i < data.indirect.Length(); i++) {
				block_run* indirect = (block_run *)cached.SetTo(block + i);
				if (indirect == NULL)
					return B_IO_ERROR;

				int32 current = -1;
				while (++current < runsPerBlock) {
					if (indirect[current].IsZero())
						break;

					runBlockEnd
						+= (uint32)indirect[current].Length() << cached.BlockShift();
					if (runBlockEnd > pos) {
						run = indirect[current];
						offset = runBlockEnd
							- ((uint32)run.Length() << cached.BlockShift());
						//printf("reading from indirect block: %ld,%d\n",fRun.allocation_group,fRun.start);
						//printf("### indirect-run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.length,fRunFileOffset);
						return fVolume.ValidateBlockRun(run);
					}
				}
			}
			return B_ERROR;
		}
	} else {
		// access from direct blocks

		off_t runBlockEnd = 0LL;
		int32 current = -1;

		while (++current < NUM_DIRECT_BLOCKS) {
			if (data.direct[current].IsZero())
				break;

			runBlockEnd += (uint32)data.direct[current].Length() << fVolume.BlockShift();
			if (runBlockEnd > pos) {
				run = data.direct[current];
				offset = runBlockEnd - ((uint32)run.Length() << fVolume.BlockShift());
				//printf("### run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.length,fRunFileOffset);
				return fVolume.ValidateBlockRun(run);
			}
		}
		//PRINT(("FindBlockRun() failed in direct range: size = %Ld, pos = %Ld\n",data.size,pos));
		return B_ENTRY_NOT_FOUND;
	}
	return fVolume.ValidateBlockRun(run);
}


status_t
Stream::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	// set/check boundaries for pos/length

	if (pos < 0)
		return B_BAD_VALUE;
	if (pos >= data.Size()) {
		*_length = 0;
		return B_NO_ERROR;
	}

	size_t length = *_length;

	if (pos + (off_t)length > data.Size())
		length = data.Size() - pos;

	block_run run;
	off_t offset;
	if (FindBlockRun(pos, run, offset) < B_OK) {
		*_length = 0;
		return B_BAD_VALUE;
	}

	uint32 bytesRead = 0;
	uint32 blockSize = fVolume.BlockSize();
	uint32 blockShift = fVolume.BlockShift();
	uint8* block;

	// the first block_run we read could not be aligned to the block_size boundary
	// (read partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start()
			+ ((pos - offset) >> blockShift));
		run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length()
			- ((pos - offset) >> blockShift));

		CachedBlock cached(fVolume, run);
		if ((block = cached.Block()) == NULL) {
			*_length = 0;
			return B_BAD_VALUE;
		}

		bytesRead = blockSize - (pos % blockSize);
		if (length < bytesRead)
			bytesRead = length;

		memcpy(buffer, block + (pos % blockSize), bytesRead);
		pos += bytesRead;

		length -= bytesRead;
		if (length == 0) {
			*_length = bytesRead;
			return B_OK;
		}

		if (FindBlockRun(pos, run, offset) < B_OK) {
			*_length = bytesRead;
			return B_BAD_VALUE;
		}
	}

	// the first block_run is already filled in at this point
	// read the following complete blocks using cached_read(),
	// the last partial block is read using the generic Cache class

	bool partial = false;

	while (length > 0) {
		// offset is the offset to the current pos in the block_run
		run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start()
			+ ((pos - offset) >> blockShift));
		run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length()
			- ((pos - offset) >> blockShift));

		if (uint32(run.Length() << blockShift) > length) {
			if (length < blockSize) {
				CachedBlock cached(fVolume, run);
				if ((block = cached.Block()) == NULL) {
					*_length = bytesRead;
					return B_BAD_VALUE;
				}
				memcpy(buffer + bytesRead, block, length);
				bytesRead += length;
				break;
			}
			run.length = HOST_ENDIAN_TO_BFS_INT16(length >> blockShift);
			partial = true;
		}

		if (read_pos(fVolume.Device(), fVolume.ToOffset(run), buffer + bytesRead,
				run.Length() << fVolume.BlockShift()) < B_OK) {
			*_length = bytesRead;
			return B_BAD_VALUE;
		}

		int32 bytes = run.Length() << blockShift;
		length -= bytes;
		bytesRead += bytes;
		if (length == 0)
			break;

		pos += bytes;

		if (partial) {
			// if the last block was read only partially, point block_run
			// to the remaining part
			run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + run.Length());
			run.length = 1;
			offset = pos;
		} else if (FindBlockRun(pos, run, offset) < B_OK) {
			*_length = bytesRead;
			return B_BAD_VALUE;
		}
	}

	*_length = bytesRead;
	return B_NO_ERROR;
}


Node*
Stream::NodeFactory(Volume& volume, off_t id)
{
	Stream stream(volume, id);
	if (stream.InitCheck() != B_OK)
		return NULL;

	if (stream.IsContainer())
		return new(nothrow) Directory(stream);

	if (stream.IsSymlink())
		return new(nothrow) Link(stream);

	return new(nothrow) File(stream);
}


//	#pragma mark -


status_t
bfs_inode::InitCheck(Volume* volume) const
{
	if ((Flags() & INODE_NOT_READY) != 0) {
		// the other fields may not yet contain valid values
		return B_BUSY;
	}

	if (Magic1() != INODE_MAGIC1
		|| !(Flags() & INODE_IN_USE)
		|| inode_num.Length() != 1
		// matches inode size?
		|| (uint32)InodeSize() != volume->InodeSize()
		// parent resides on disk?
		|| parent.AllocationGroup() > int32(volume->AllocationGroups())
		|| parent.AllocationGroup() < 0
		|| parent.Start() > (1L << volume->AllocationGroupShift())
		|| parent.Length() != 1
		// attributes, too?
		|| attributes.AllocationGroup() > int32(volume->AllocationGroups())
		|| attributes.AllocationGroup() < 0
		|| attributes.Start() > (1L << volume->AllocationGroupShift()))
		return B_BAD_DATA;

	// TODO: Add some tests to check the integrity of the other stuff here,
	// especially for the data_stream!

	return B_OK;
}
