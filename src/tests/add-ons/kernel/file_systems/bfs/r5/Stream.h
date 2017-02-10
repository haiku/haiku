/* Stream - inode stream access functions
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "Inode.h"


// The classes in the namespace "Access" provide different type of access
// to the inode's data stream.
// Uncached accesses the underlaying device directly, Cached uses the
// standard cache, while Logged directs write accesses through the log.
//
// The classes interface is similar to the one of the CachedBlock class,
// but adds two other (static) functions for reading/writing several
// blocks at once.
// We don't use a real pure virtual interface as the class base, but we
// provide the same mechanism using templates.

namespace Access {

class Uncached {
	public:
		Uncached(Volume *volume);
		Uncached(Volume *volume, off_t block, bool empty = false);
		Uncached(Volume *volume, block_run run, bool empty = false);
		~Uncached();

		void Unset();
		uint8 *SetTo(off_t block, bool empty = false);
		uint8 *SetTo(block_run run, bool empty = false);
		status_t WriteBack(Transaction *transaction);

		uint8 *Block() const { return fBlock; }
		off_t BlockNumber() const { return fBlockNumber; }
		uint32 BlockSize() const { return fVolume->BlockSize(); }
		uint32 BlockShift() const { return fVolume->BlockShift(); }

		static status_t Read(Volume *volume, block_run run, uint8 *buffer);
		static status_t Write(Transaction *transaction, Volume *volume, block_run run, const uint8 *buffer);

	private:
		Volume	*fVolume;
		off_t	fBlockNumber;
		uint8	*fBlock;
};

class Cached : public CachedBlock {
	public:
		Cached(Volume *volume);
		Cached(Volume *volume, off_t block, bool empty = false);
		Cached(Volume *volume, block_run run, bool empty = false);

		status_t WriteBack(Transaction *transaction);
		static status_t Read(Volume *volume, block_run run, uint8 *buffer);
		static status_t Write(Transaction *transaction, Volume *volume, block_run run, const uint8 *buffer);
};

class Logged : public CachedBlock {
	public:
		Logged(Volume *volume);
		Logged(Volume *volume,off_t block, bool empty = false);
		Logged(Volume *volume, block_run run, bool empty = false);

		static status_t Read(Volume *volume, block_run run, uint8 *buffer);
		static status_t Write(Transaction *transaction, Volume *volume, block_run run, const uint8 *buffer);
};


Uncached::Uncached(Volume *volume)
	:
	fVolume(volume),
	fBlock(NULL)
{
}


Uncached::Uncached(Volume *volume,off_t block, bool empty)
	:
	fVolume(volume),
	fBlock(NULL)
{
	SetTo(block,empty);
}


Uncached::Uncached(Volume *volume,block_run run,bool empty)
	:
	fVolume(volume),
	fBlock(NULL)
{
	SetTo(volume->ToBlock(run),empty);
}


Uncached::~Uncached()
{
	Unset();
}


void
Uncached::Unset()
{
	if (fBlock != NULL)
		fVolume->Pool().PutBuffer((void *)fBlock);
}


uint8 *
Uncached::SetTo(off_t block, bool empty)
{
	Unset();
	fBlockNumber = block;
	if (fVolume->Pool().GetBuffer((void **)&fBlock) < B_OK)
		return NULL;

	if (empty)
		memset(fBlock, 0, BlockSize());
	else
		read_pos(fVolume->Device(), fBlockNumber << BlockShift(), fBlock, BlockSize());

	return fBlock;
}


uint8 *
Uncached::SetTo(block_run run, bool empty)
{
	return SetTo(fVolume->ToBlock(run), empty);
}


status_t
Uncached::WriteBack(Transaction *transaction)
{
	if (fBlock == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	return write_pos(fVolume->Device(), fBlockNumber << BlockShift(), fBlock, BlockSize());
}


status_t
Uncached::Read(Volume *volume, block_run run, uint8 *buffer)
{
	return read_pos(volume->Device(), volume->ToBlock(run) << volume->BlockShift(), buffer, run.Length() << volume->BlockShift());
}


status_t
Uncached::Write(Transaction *transaction, Volume *volume, block_run run, const uint8 *buffer)
{
	return write_pos(volume->Device(), volume->ToBlock(run) << volume->BlockShift(), buffer, run.Length() << volume->BlockShift());
}


//	#pragma mark -


Cached::Cached(Volume *volume)
	: CachedBlock(volume)
{
}


Cached::Cached(Volume *volume,off_t block,bool empty)
	: CachedBlock(volume, block, empty)
{
}


Cached::Cached(Volume *volume,block_run run,bool empty)
	: CachedBlock(volume, run, empty)
{
}


status_t
Cached::WriteBack(Transaction *transaction)
{
	if (transaction == NULL || fBlock == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	return fVolume->WriteBlocks(fBlockNumber, fBlock, 1);
}


status_t
Cached::Read(Volume *volume, block_run run, uint8 *buffer)
{
	return cached_read(volume->Device(), volume->ToBlock(run), buffer, run.Length(), volume->BlockSize());
}


status_t
Cached::Write(Transaction *transaction, Volume *volume, block_run run, const uint8 *buffer)
{
	return volume->WriteBlocks(volume->ToBlock(run), buffer, run.Length());
}


//	#pragma mark -


Logged::Logged(Volume *volume)
	: CachedBlock(volume)
{
}


Logged::Logged(Volume *volume, off_t block, bool empty)
	: CachedBlock(volume, block, empty)
{
}


Logged::Logged(Volume *volume, block_run run, bool empty)
	: CachedBlock(volume, run, empty)
{
}


status_t
Logged::Read(Volume *volume, block_run run, uint8 *buffer)
{
	return cached_read(volume->Device(), volume->ToBlock(run), buffer, run.Length(), volume->BlockSize());
}


status_t
Logged::Write(Transaction *transaction, Volume *volume, block_run run, const uint8 *buffer)
{
	return transaction->WriteBlocks(volume->ToBlock(run), buffer, run.Length());
}

};	// namespace Access


//	#pragma mark -


// The Stream template class allows to have only one straight-forward
// implementation of the FindBlockRun(), ReadAt(), and WriteAt() methods.
// They will access the disk through the given cache class only, which
// means either uncached, cached, or logged (see above).

template<class Cache>
class Stream : public Inode {
	private:
		// The constructor only exists to make the compiler happy - it
		// is never called in the code itself
		Stream() : Inode(NULL, -1) {}

	public:
		status_t FindBlockRun(off_t pos, block_run &run, off_t &offset);
		status_t ReadAt(off_t pos, uint8 *buffer, size_t *length);
		status_t WriteAt(Transaction *transaction, off_t pos, const uint8 *buffer, size_t *length);
};


/** see Inode::FindBlockRun() for the documentation of this method */

template<class Cache>
status_t
Stream<Cache>::FindBlockRun(off_t pos, block_run &run, off_t &offset)
{
	data_stream *data = &Node()->data;

	// find matching block run

	if (data->MaxDirectRange() > 0 && pos >= data->MaxDirectRange()) {
		if (data->MaxDoubleIndirectRange() > 0 && pos >= data->MaxIndirectRange()) {
			// access to double indirect blocks

			Cache cached(fVolume);

			off_t start = pos - data->MaxIndirectRange();
			int32 indirectSize = (1L << (INDIRECT_BLOCKS_SHIFT + cached.BlockShift()))
				* (fVolume->BlockSize() / sizeof(block_run));
			int32 directSize = NUM_ARRAY_BLOCKS << cached.BlockShift();
			int32 index = start / indirectSize;
			int32 runsPerBlock = cached.BlockSize() / sizeof(block_run);

			block_run *indirect = (block_run *)cached.SetTo(
					fVolume->ToBlock(data->double_indirect) + index / runsPerBlock);
			if (indirect == NULL)
				RETURN_ERROR(B_ERROR);

			//printf("\tstart = %Ld, indirectSize = %ld, directSize = %ld, index = %ld\n",start,indirectSize,directSize,index);
			//printf("\tlook for indirect block at %ld,%d\n",indirect[index].allocation_group,indirect[index].start);

			int32 current = (start % indirectSize) / directSize;

			indirect = (block_run *)cached.SetTo(
					fVolume->ToBlock(indirect[index % runsPerBlock]) + current / runsPerBlock);
			if (indirect == NULL)
				RETURN_ERROR(B_ERROR);

			run = indirect[current % runsPerBlock];
			offset = data->MaxIndirectRange() + (index * indirectSize) + (current * directSize);
			//printf("\tfCurrent = %ld, fRunFileOffset = %Ld, fRunBlockEnd = %Ld, fRun = %ld,%d\n",fCurrent,fRunFileOffset,fRunBlockEnd,fRun.allocation_group,fRun.start);
		} else {
			// access to indirect blocks

			int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);
			off_t runBlockEnd = data->MaxDirectRange();

			Cache cached(fVolume);
			off_t block = fVolume->ToBlock(data->indirect);

			for (int32 i = 0; i < data->indirect.Length(); i++) {
				block_run *indirect = (block_run *)cached.SetTo(block + i);
				if (indirect == NULL)
					RETURN_ERROR(B_IO_ERROR);

				int32 current = -1;
				while (++current < runsPerBlock) {
					if (indirect[current].IsZero())
						break;

					runBlockEnd += indirect[current].Length() << cached.BlockShift();
					if (runBlockEnd > pos) {
						run = indirect[current];
						offset = runBlockEnd - (run.Length() << cached.BlockShift());
						//printf("reading from indirect block: %ld,%d\n",fRun.allocation_group,fRun.start);
						//printf("### indirect-run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.Length(),fRunFileOffset);
						return fVolume->ValidateBlockRun(run);
					}
				}
			}
			RETURN_ERROR(B_ERROR);
		}
	} else {
		// access from direct blocks

		off_t runBlockEnd = 0LL;
		int32 current = -1;

		while (++current < NUM_DIRECT_BLOCKS) {
			if (data->direct[current].IsZero())
				break;

			runBlockEnd += data->direct[current].Length() << fVolume->BlockShift();
			if (runBlockEnd > pos) {
				run = data->direct[current];
				offset = runBlockEnd - (run.Length() << fVolume->BlockShift());
				//printf("### run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.Length(),fRunFileOffset);
				return fVolume->ValidateBlockRun(run);
			}
		}
		//PRINT(("FindBlockRun() failed in direct range: size = %Ld, pos = %Ld\n",data->size,pos));
		return B_ENTRY_NOT_FOUND;
	}
	return fVolume->ValidateBlockRun(run);
}


template<class Cache>
status_t
Stream<Cache>::ReadAt(off_t pos, uint8 *buffer, size_t *_length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0)
		return B_BAD_VALUE;
	if (pos >= Node()->data.Size() || length == 0) {
		*_length = 0;
		return B_NO_ERROR;
	}

	if (pos + length > Node()->data.Size())
		length = Node()->data.Size() - pos;

	block_run run;
	off_t offset;
	if (FindBlockRun(pos, run, offset) < B_OK) {
		*_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	uint32 bytesRead = 0;
	uint32 blockSize = fVolume->BlockSize();
	uint32 blockShift = fVolume->BlockShift();
	uint8 *block;

	// the first block_run we read could not be aligned to the block_size boundary
	// (read partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + ((pos - offset) >> blockShift));
		run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - ((pos - offset) >> blockShift));

		Cache cached(fVolume,run);
		if ((block = cached.Block()) == NULL) {
			*_length = 0;
			RETURN_ERROR(B_BAD_VALUE);
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
			RETURN_ERROR(B_BAD_VALUE);
		}
	}

	// the first block_run is already filled in at this point
	// read the following complete blocks using cached_read(),
	// the last partial block is read using the generic Cache class

	bool partial = false;

	while (length > 0) {
		// offset is the offset to the current pos in the block_run
		run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + ((pos - offset) >> blockShift));
		run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - ((pos - offset) >> blockShift));

		if (uint32(run.Length() << blockShift) > length) {
			if (length < blockSize) {
				Cache cached(fVolume, run);
				if ((block = cached.Block()) == NULL) {
					*_length = bytesRead;
					RETURN_ERROR(B_BAD_VALUE);
				}
				memcpy(buffer + bytesRead, block, length);
				bytesRead += length;
				break;
			}
			run.length = HOST_ENDIAN_TO_BFS_INT16(length >> blockShift);
			partial = true;
		}

		if (Cache::Read(fVolume, run, buffer + bytesRead) < B_OK) {
			*_length = bytesRead;
			RETURN_ERROR(B_BAD_VALUE);
		}

		int32 bytes = run.Length() << blockShift;
#ifdef DEBUG
		if ((uint32)bytes > length)
			DEBUGGER(("bytes greater than length"));
#endif
		length -= bytes;
		bytesRead += bytes;
		if (length == 0)
			break;

		pos += bytes;

		if (partial) {
			// if the last block was read only partially, point block_run
			// to the remaining part
			run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + run.Length());
			run.length = HOST_ENDIAN_TO_BFS_INT16(1);
			offset = pos;
		} else if (FindBlockRun(pos, run, offset) < B_OK) {
			*_length = bytesRead;
			RETURN_ERROR(B_BAD_VALUE);
		}
	}

	*_length = bytesRead;
	return B_OK;
}


template<class Cache>
status_t
Stream<Cache>::WriteAt(Transaction *transaction, off_t pos, const uint8 *buffer, size_t *_length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0)
		return B_BAD_VALUE;

	if (pos + length > Size()) {
		off_t oldSize = Size();

		// uncached files can't be resized (Inode::SetFileSize() also
		// doesn't allow this, but this way we don't have to start a
		// transaction to find out).
		if (Flags() & INODE_NO_CACHE)
			return B_BAD_VALUE;

		// the transaction doesn't have to be started already
		// ToDo: what's that INODE_NO_TRANSACTION flag good for again?
		if ((Flags() & INODE_NO_TRANSACTION) == 0
			&& !transaction->IsStarted())
			transaction->Start(fVolume, BlockNumber());

		// let's grow the data stream to the size needed
		status_t status = SetFileSize(transaction, pos + length);
		if (status < B_OK) {
			*_length = 0;
			RETURN_ERROR(status);
		}
		// If the position of the write was beyond the file size, we
		// have to fill the gap between that position and the old file
		// size with zeros.
		FillGapWithZeros(oldSize, pos);
	}

	// If we don't want to write anything, we can now return (we may
	// just have changed the file size using the position parameter)
	if (length == 0)
		return B_OK;

	block_run run;
	off_t offset;
	if (FindBlockRun(pos, run, offset) < B_OK) {
		*_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	bool logStream = (Flags() & INODE_LOGGED) == INODE_LOGGED;
	if (logStream
		&& !transaction->IsStarted())
		transaction->Start(fVolume, BlockNumber());

	uint32 bytesWritten = 0;
	uint32 blockSize = fVolume->BlockSize();
	uint32 blockShift = fVolume->BlockShift();
	uint8 *block;

	// the first block_run we write could not be aligned to the block_size boundary
	// (write partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + ((pos - offset) >> blockShift));
		run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - ((pos - offset) >> blockShift));

		Cache cached(fVolume, run);
		if ((block = cached.Block()) == NULL) {
			*_length = 0;
			RETURN_ERROR(B_BAD_VALUE);
		}

		bytesWritten = blockSize - (pos % blockSize);
		if (length < bytesWritten)
			bytesWritten = length;

		memcpy(block + (pos % blockSize),buffer,bytesWritten);

		cached.WriteBack(transaction);

		pos += bytesWritten;
		
		length -= bytesWritten;
		if (length == 0) {
			*_length = bytesWritten;
			return B_OK;
		}

		if (FindBlockRun(pos, run, offset) < B_OK) {
			*_length = bytesWritten;
			RETURN_ERROR(B_BAD_VALUE);
		}
	}

	// the first block_run is already filled in at this point
	// write the following complete blocks using Volume::WriteBlocks(),
	// the last partial block is written using the generic Cache class

	bool partial = false;

	while (length > 0) {
		// offset is the offset to the current pos in the block_run
		run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + ((pos - offset) >> blockShift));
		run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - ((pos - offset) >> blockShift));

		if (uint32(run.Length() << blockShift) > length) {
			if (length < blockSize) {
				Cache cached(fVolume,run);
				if ((block = cached.Block()) == NULL) {
					*_length = bytesWritten;
					RETURN_ERROR(B_BAD_VALUE);
				}
				memcpy(block, buffer + bytesWritten, length);

				cached.WriteBack(transaction);

				bytesWritten += length;
				break;
			}
			run.length = HOST_ENDIAN_TO_BFS_INT16(length >> blockShift);
			partial = true;
		}

		if (Cache::Write(transaction, fVolume, run, buffer + bytesWritten) < B_OK) {
			*_length = bytesWritten;
			RETURN_ERROR(B_BAD_VALUE);
		}

		int32 bytes = run.Length() << blockShift;
		length -= bytes;
		bytesWritten += bytes;
		if (length == 0)
			break;

		pos += bytes;

		if (partial) {
			// if the last block was written only partially, point block_run
			// to the remaining part
			run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + run.Length());
			run.length = HOST_ENDIAN_TO_BFS_INT16(1);
			offset = pos;
		} else if (FindBlockRun(pos, run, offset) < B_OK) {
			*_length = bytesWritten;
			RETURN_ERROR(B_BAD_VALUE);
		}
	}

	*_length = bytesWritten;

	return B_OK;
}

