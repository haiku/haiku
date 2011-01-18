/*
 * Copyright 2001-2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "ExtentStream.h"

#include <string.h>

#include "CachedBlock.h"
#include "Volume.h"


#undef ASSERT
//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m ExtentStream::" x)
#	define ASSERT(x) { if (!(x)) kernel_debugger("ext2: assert failed: " #x "\n"); }
#else
#	define TRACE(x...) ;
#	define ASSERT(x) ;
#endif
#define ERROR(x...)	dprintf("\33[34mext2:\33[0m ExtentStream::" x)


ExtentStream::ExtentStream(Volume* volume, ext2_extent_stream* stream,
	off_t size)
	:
	fVolume(volume),
	fStream(stream),
	fFirstBlock(volume->FirstDataBlock()),
	fAllocatedPos(fFirstBlock),
	fSize(size)
{
	fNumBlocks = size == 0 ? 0 : ((size - 1) >> fVolume->BlockShift()) + 1;
}


ExtentStream::~ExtentStream()
{
}


status_t
ExtentStream::FindBlock(off_t offset, fsblock_t& block, uint32 *_count)
{
	fileblock_t index = offset >> fVolume->BlockShift();
	TRACE("FindBlock(%lld, %lld)\n", offset, index);

	if (offset >= fSize) {
		TRACE("FindBlock: offset larger than inode size\n");
		return B_ENTRY_NOT_FOUND;
	}

	ext2_extent_stream *stream = fStream;
	if (!stream->extent_header.IsValid())
		panic("ExtentStream::FindBlock() invalid header\n");

	CachedBlock cached(fVolume);
	while (stream->extent_header.Depth() != 0) {
		TRACE("FindBlock() depth %d\n",
			stream->extent_header.Depth());
		int32 i = 1;
		while (i < stream->extent_header.NumEntries()
			&& stream->extent_index[i].LogicalBlock() <= index) {
			i++;
		}
		TRACE("FindBlock() getting index %ld at %lld\n", i - 1,
			stream->extent_index[i - 1].PhysicalBlock());
		stream = (ext2_extent_stream *)cached.SetTo(
			stream->extent_index[i - 1].PhysicalBlock());
		if (!stream->extent_header.IsValid())
			panic("ExtentStream::FindBlock() invalid header\n");
	}

	if (stream->extent_header.NumEntries() > 7) {
		// binary search when enough entries
		int32 low = 0;
		int32 high = stream->extent_header.NumEntries() - 1;
		int32 middle = 0;
		while (low <= high) {
			middle = (high + low) >> 1;
			if (stream->extent_entries[middle].LogicalBlock() == index)
				break;
			if (stream->extent_entries[middle].LogicalBlock() < index)
				low = middle + 1;
			else
				high = middle - 1;
		}
		if (stream->extent_entries[middle].LogicalBlock() > index)
			middle--;
		fileblock_t diff = index 
			- stream->extent_entries[middle].LogicalBlock();
		if (diff > stream->extent_entries[middle].Length()) {
			// sparse block
			TRACE("FindBlock() sparse block index %lld at %ld\n", index,
				stream->extent_entries[middle].LogicalBlock());
			block = 0xffffffff;
			return B_OK;
		}

		block = stream->extent_entries[middle].PhysicalBlock() + diff;
		if (_count)
			*_count = stream->extent_entries[middle].Length() - diff;
		TRACE("FindBlock(offset %lld): %lld %ld\n", offset, 
			block, _count != NULL ? *_count : 1);
		return B_OK;
	}

	for (int32 i = 0; i < stream->extent_header.NumEntries(); i++) {
		if (stream->extent_entries[i].LogicalBlock() > index) {
			// sparse block
			TRACE("FindBlock() sparse block index %lld at %ld\n", index,
				stream->extent_entries[i].LogicalBlock());
			block = 0xffffffff;
			return B_OK;
		}
		fileblock_t diff = index - stream->extent_entries[i].LogicalBlock();
		if (diff < stream->extent_entries[i].Length()) {
			block = stream->extent_entries[i].PhysicalBlock() + diff;
			if (_count)
				*_count = stream->extent_entries[i].Length() - diff;
			TRACE("FindBlock(offset %lld): %lld %ld\n", offset, 
				block, _count != NULL ? *_count : 1);
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
ExtentStream::Enlarge(Transaction& transaction, off_t& numBlocks)
{
	TRACE("Enlarge(): current size: %llu, target size: %llu\n",
		fNumBlocks, numBlocks);
	
	off_t targetBlocks = numBlocks;
	numBlocks = targetBlocks - fNumBlocks;
	uint32 allocated = 0;

	while (fNumBlocks < targetBlocks) {
		// allocate new blocks
		uint32 blockGroup = (fAllocatedPos - fFirstBlock)
				/ fVolume->BlocksPerGroup();
		
		if (allocated == 0) {
			status_t status = fVolume->AllocateBlocks(transaction, 1,
				targetBlocks - fNumBlocks, blockGroup, fAllocatedPos,
				allocated);
			if (status != B_OK) {
				ERROR("Enlarge(): AllocateBlocks() failed()\n");
				return status;
			}
		}

		ASSERT(_CheckBlock(fStream, fAllocatedPos) == B_OK);

		ext2_extent_stream *stream = fStream;
		fsblock_t path[stream->extent_header.Depth()];
	
		// search for the leaf
		CachedBlock cached(fVolume);
		int32 level = -1;
		for (; stream->extent_header.Depth() != 0;) {
			if (stream->extent_header.NumEntries() == 0)
				panic("stream->extent_header.NumEntries() == 0\n");
			int32 lastIndex = stream->extent_header.NumEntries() - 1;
			TRACE("Enlarge() depth %d\n", stream->extent_header.Depth());
			TRACE("Enlarge() getting index %ld at %lld\n", lastIndex,
				stream->extent_index[lastIndex].PhysicalBlock());
			path[++level] = stream->extent_index[lastIndex].PhysicalBlock();
			stream = (ext2_extent_stream *)cached.SetTo(path[level]);
			if (stream == NULL)
				return B_IO_ERROR;
		}

		// check if the new extent is adjacent
		if (stream->extent_header.NumEntries() > 0) {
			ext2_extent_entry &last = stream->extent_entries[
				stream->extent_header.NumEntries() - 1];
			TRACE("Enlarge() last %lld allocatedPos %lld\n",
				last.PhysicalBlock() + last.Length(), fAllocatedPos);
			if (last.PhysicalBlock() + last.Length() == fAllocatedPos
				&& (last.Length() + allocated) <= EXT2_EXTENT_MAX_LENGTH) {
				if (stream != fStream) {
					stream = (ext2_extent_stream *)cached.SetToWritable(
						transaction, cached.BlockNumber());
					if (stream == NULL)
						return B_IO_ERROR;
				}
				stream->extent_entries[stream->extent_header.NumEntries() - 1]
					.SetLength(last.Length() + allocated);
				fNumBlocks += allocated;
				allocated = 0;
				TRACE("Enlarge() entry extended\n");
				continue;
			}
		}

		if (stream->extent_header.NumEntries()
			>= stream->extent_header.MaxEntries()) {
			TRACE("Enlarge() adding leaf and indexes at depth %d level %ld\n",
				stream->extent_header.Depth(), level);
			// try to add a leaf and indexes
			while (--level >= 0) {
				stream = (ext2_extent_stream *)cached.SetTo(path[level]);
				if (stream == NULL)
					return B_IO_ERROR;
				if (stream->extent_header.NumEntries()
					< stream->extent_header.MaxEntries()) {
					break;
				}
				TRACE("Enlarge() going up from level %ld\n", level);
			}

			if (level < 0 && fStream->extent_header.NumEntries()
					>= fStream->extent_header.MaxEntries()) {
				TRACE("Enlarge() add a level, increment root depth\n");
				fsblock_t newBlock = fStream->extent_index[0].PhysicalBlock();
				uint32 _allocated;
				status_t status = fVolume->AllocateBlocks(transaction, 1, 1,
					blockGroup, newBlock, _allocated);
				if (status != B_OK) {
					ERROR("Enlarge() AllocateBlocks() failed()\n");
					return status;
				}
				ASSERT(_CheckBlock(fStream, newBlock) == B_OK);
				TRACE("Enlarge() move root to block %lld\n", newBlock);
				numBlocks++;
				stream = (ext2_extent_stream *)cached.SetToWritable(
					transaction, newBlock);
				if (stream == NULL)
					return B_IO_ERROR;
				ASSERT(fStream->extent_header.IsValid());
				memcpy(stream, fStream, sizeof(ext2_extent_stream));
				fStream->extent_header.SetDepth(stream->extent_header.Depth()
					+ 1);
				TRACE("Enlarge() new root depth %d\n",
					fStream->extent_header.Depth());
				fStream->extent_header.SetNumEntries(1);
				fStream->extent_index[0].SetLogicalBlock(0);
				fStream->extent_index[0].SetPhysicalBlock(newBlock);
				stream->extent_header.SetMaxEntries((fVolume->BlockSize()
					- sizeof(ext2_extent_header)) / sizeof(ext2_extent_index));
				ASSERT(stream->extent_header.IsValid());

				ASSERT(Check());
				continue;
			}

			if (level < 0)
				stream = fStream;

			uint16 depth = stream->extent_header.Depth();
			while (depth > 1) {
				TRACE("Enlarge() adding an index block at depth %d\n", depth);
				fsblock_t newBlock = path[level];
				uint32 _allocated;
				status_t status = fVolume->AllocateBlocks(transaction, 1, 1,
					blockGroup, newBlock, _allocated);
				if (status != B_OK) {
					ERROR("Enlarge(): AllocateBlocks() failed()\n");
					return status;
				}
				ASSERT(_CheckBlock(fStream, newBlock) == B_OK);
				numBlocks++;
				int32 index = stream->extent_header.NumEntries();
				stream->extent_index[index].SetLogicalBlock(fNumBlocks);
				stream->extent_index[index].SetPhysicalBlock(newBlock);
				stream->extent_header.SetNumEntries(index + 1);
				path[level++] = newBlock;

				depth = stream->extent_header.Depth() - 1;
				TRACE("Enlarge() init index block %lld at depth %d\n",
					newBlock, depth);
				stream = (ext2_extent_stream *)cached.SetToWritable(
					transaction, newBlock);
				if (stream == NULL)
					return B_IO_ERROR;
				stream->extent_header.magic = EXT2_EXTENT_MAGIC;
				stream->extent_header.SetNumEntries(0);
				stream->extent_header.SetMaxEntries((fVolume->BlockSize()
					- sizeof(ext2_extent_header)) / sizeof(ext2_extent_index));
				stream->extent_header.SetDepth(depth);
				stream->extent_header.SetGeneration(0);

				ASSERT(Check());
			}

			TRACE("Enlarge() depth %d level %ld\n", 
				stream->extent_header.Depth(), level);

			if (stream->extent_header.Depth() == 1) {
				TRACE("Enlarge() adding an entry block at depth %d level %ld\n",
					depth, level);
				fsblock_t newBlock;
				if (level >= 0)
					newBlock = path[level];
				else
					newBlock = fStream->extent_index[0].PhysicalBlock();
				uint32 _allocated;
				status_t status = fVolume->AllocateBlocks(transaction, 1, 1,
					blockGroup, newBlock, _allocated);
				if (status != B_OK) {
					ERROR("Enlarge(): AllocateBlocks() failed()\n");
					return status;
				}

				ASSERT(_CheckBlock(fStream, newBlock) == B_OK);
				numBlocks++;
				int32 index = stream->extent_header.NumEntries();
				stream->extent_index[index].SetLogicalBlock(fNumBlocks);
				stream->extent_index[index].SetPhysicalBlock(newBlock);
				stream->extent_header.SetNumEntries(index + 1);

				TRACE("Enlarge() init entry block %lld at depth %d\n",
					newBlock, depth);
				stream = (ext2_extent_stream *)cached.SetToWritable(
					transaction, newBlock);
				if (stream == NULL)
					return B_IO_ERROR;
				stream->extent_header.magic = EXT2_EXTENT_MAGIC;
				stream->extent_header.SetNumEntries(0);
				stream->extent_header.SetMaxEntries((fVolume->BlockSize()
					- sizeof(ext2_extent_header)) / sizeof(ext2_extent_entry));
				stream->extent_header.SetDepth(0);
				stream->extent_header.SetGeneration(0);
				ASSERT(Check());
			}
		}
		
		// add a new entry
		TRACE("Enlarge() add entry %lld\n", fAllocatedPos);
		if (stream != fStream) {
			stream = (ext2_extent_stream *)cached.SetToWritable(
				transaction, cached.BlockNumber());
			if (stream == NULL)
				return B_IO_ERROR;
		}
		int32 index = stream->extent_header.NumEntries();
		stream->extent_entries[index].SetLogicalBlock(fNumBlocks);
		stream->extent_entries[index].SetLength(allocated);
		stream->extent_entries[index].SetPhysicalBlock(fAllocatedPos);
		stream->extent_header.SetNumEntries(index + 1);
		TRACE("Enlarge() entry added at index %ld\n", index);
		ASSERT(stream->extent_header.IsValid());

		fNumBlocks += allocated;
		allocated = 0;
	}
	
	return B_OK;
}


status_t
ExtentStream::Shrink(Transaction& transaction, off_t& numBlocks)
{
	TRACE("DataStream::Shrink(): current size: %llu, target size: %llu\n",
		fNumBlocks, numBlocks);

	off_t targetBlocks = numBlocks;
	numBlocks = fNumBlocks - targetBlocks;

	while (targetBlocks < fNumBlocks) {
		ext2_extent_stream *stream = fStream;
		fsblock_t path[stream->extent_header.Depth()];
	
		// search for the leaf
		CachedBlock cached(fVolume);
		int32 level = -1;
		for (; stream->extent_header.Depth() != 0;) {
			if (stream->extent_header.NumEntries() == 0)
				panic("stream->extent_header.NumEntries() == 0\n");
			int32 lastIndex = stream->extent_header.NumEntries() - 1;
			TRACE("Shrink() depth %d\n", stream->extent_header.Depth());
			TRACE("Shrink() getting index %ld at %lld\n", lastIndex,
				stream->extent_index[lastIndex].PhysicalBlock());
			path[++level] = stream->extent_index[lastIndex].PhysicalBlock();
			stream = (ext2_extent_stream *)cached.SetToWritable(transaction, 
				path[level]);
			if (stream == NULL)
				return B_IO_ERROR;
			if (!stream->extent_header.IsValid())
				panic("Shrink() invalid header\n");
		}
		
		int32 index = stream->extent_header.NumEntries() - 1;
		status_t status = B_OK;
		while (index >= 0 && targetBlocks < fNumBlocks) {
			ext2_extent_entry &last = stream->extent_entries[index];
			if (last.LogicalBlock() + last.Length() < targetBlocks) {
				status = B_BAD_DATA;
				break;
			}
			uint16 length = min_c(last.Length(), fNumBlocks - targetBlocks);
			fsblock_t block = last.PhysicalBlock() + last.Length() - length;
			TRACE("Shrink() free block %lld length %d\n", block, length); 
			status = fVolume->FreeBlocks(transaction, block, length);
			if (status != B_OK)
				break;
			fNumBlocks -= length;
			stream->extent_entries[index].SetLength(last.Length() - length);
			TRACE("Shrink() new length for %ld: %d\n", index, last.Length());
			if (last.Length() != 0)
				break;
			index--;
			TRACE("Shrink() next index: %ld\n", index);
		}
		TRACE("Shrink() new entry count: %ld\n", index + 1);
		stream->extent_header.SetNumEntries(index + 1);
		ASSERT(Check());
		
		if (status != B_OK)
			return status;

		while (stream != fStream && stream->extent_header.NumEntries() == 0) {
			TRACE("Shrink() empty leaf at depth %d\n", 
				stream->extent_header.Depth());
			level--;
			if (level >= 0) {
				stream = (ext2_extent_stream *)cached.SetToWritable(
					transaction, path[level]);
				if (stream == NULL)
					return B_IO_ERROR;
			} else
				stream = fStream;
			if (!stream->extent_header.IsValid())
				panic("Shrink() invalid header\n");
			uint16 index = stream->extent_header.NumEntries() - 1;
			ext2_extent_index &last = stream->extent_index[index];
			if (last.PhysicalBlock() != path[level + 1])
				panic("Shrink() last.PhysicalBlock() != path[level + 1]\n");
			status = fVolume->FreeBlocks(transaction, last.PhysicalBlock(), 1);
			if (status != B_OK)
				return status;
			numBlocks++;
			stream->extent_header.SetNumEntries(index);
			TRACE("Shrink() new entry count: %d\n", index);
		}
		if (stream == fStream && stream->extent_header.NumEntries() == 0)
			stream->extent_header.SetDepth(0);

		ASSERT(Check());
	}

	return B_OK;
}


void
ExtentStream::Init()
{
	fStream->extent_header.magic = EXT2_EXTENT_MAGIC;
	fStream->extent_header.SetNumEntries(0);
	fStream->extent_header.SetMaxEntries(4);
	fStream->extent_header.SetDepth(0);
	fStream->extent_header.SetGeneration(0);
}


status_t
ExtentStream::_Check(ext2_extent_stream *stream, fileblock_t &block)
{
	if (!stream->extent_header.IsValid()) {
		panic("_Check() invalid header\n");
		return B_BAD_VALUE;
	}
	if (stream->extent_header.Depth() == 0) {
		for (int32 i = 0; i < stream->extent_header.NumEntries(); i++) {
			ext2_extent_entry &entry = stream->extent_entries[i];
			if (entry.LogicalBlock() < block) {
				panic("_Check() entry %ld %lld %ld\n", i, block,
					entry.LogicalBlock());
				return B_BAD_VALUE;
			}
			block = entry.LogicalBlock() + entry.Length();
		}
		return B_OK;
	} 

	CachedBlock cached(fVolume);
	for (int32 i = 0; i < stream->extent_header.NumEntries(); i++) {
		ext2_extent_index &index = stream->extent_index[i];
		if (index.LogicalBlock() < block) {
			panic("_Check() index %ld %lld %ld\n", i, block,
				index.LogicalBlock());
			return B_BAD_VALUE;
		}
		ext2_extent_stream *child = (ext2_extent_stream *)cached.SetTo(
			index.PhysicalBlock());
		if (child->extent_header.Depth() != (stream->extent_header.Depth() - 1) 
			|| _Check(child, block) != B_OK)
			return B_BAD_VALUE;
	}
	return B_OK;
}


bool
ExtentStream::Check()
{
	fileblock_t block = 0;
	return _Check(fStream, block) == B_OK;
}


status_t
ExtentStream::_CheckBlock(ext2_extent_stream *stream, fsblock_t block)
{
	if (stream->extent_header.Depth() == 0) {
		for (int32 i = 0; i < stream->extent_header.NumEntries(); i++) {
			ext2_extent_entry &entry = stream->extent_entries[i];
			if (entry.PhysicalBlock() <= block
				&& (entry.PhysicalBlock() + entry.Length()) > block) {
				panic("_CheckBlock() entry %ld %lld %lld %d\n", i, block,
					entry.PhysicalBlock(), entry.Length());
				return B_BAD_VALUE;
			}
		}
		return B_OK;
	}

	CachedBlock cached(fVolume);
	for (int32 i = 0; i < stream->extent_header.NumEntries(); i++) {
		ext2_extent_index &index = stream->extent_index[i];
		if (index.PhysicalBlock() == block) {
			panic("_CheckBlock() index %ld %lld\n", i, block);
			return B_BAD_VALUE;
		}
		ext2_extent_stream *child = (ext2_extent_stream *)cached.SetTo(
			index.PhysicalBlock());
		if (child->extent_header.Depth() != (stream->extent_header.Depth() - 1)
			|| _CheckBlock(child, block) != B_OK)
			return B_BAD_VALUE;
	}
	return B_OK;
}
