// DataContainer.cpp

#include "AllocationInfo.h"
#include "Attribute.h"	// for debugging only
#include "Block.h"
#include "DataContainer.h"
#include "Debug.h"
#include "Misc.h"
#include "Node.h"		// for debugging only
#include "Volume.h"

// constructor
DataContainer::DataContainer(Volume *volume)
	: fVolume(volume),
	  fSize(0)
{
}

// destructor
DataContainer::~DataContainer()
{
	Resize(0);
}

// InitCheck
status_t
DataContainer::InitCheck() const
{
	return (fVolume ? B_OK : B_ERROR);
}

// Resize
status_t
DataContainer::Resize(off_t newSize)
{
	status_t error = B_OK;
	if (newSize < 0)
		newSize = 0;
	if (newSize != fSize) {
		// Shrinking should never fail. Growing can fail, if we run out of
		// memory. Then we try to shrink back to the original size.
		off_t oldSize = fSize;
		error = _Resize(newSize);
		if (error == B_NO_MEMORY && newSize > fSize)
			_Resize(oldSize);
	}
	return error;
}

// ReadAt
status_t
DataContainer::ReadAt(off_t offset, void *_buffer, size_t size,
					  size_t *bytesRead)
{
	uint8 *buffer = (uint8*)_buffer;
	status_t error = (buffer && offset >= 0 && bytesRead ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// read not more than we have to offer
		offset = min(offset, fSize);
		size = min(size, size_t(fSize - offset));
		// iterate through the blocks, reading as long as there's something
		// left to read
		size_t blockSize = fVolume->GetBlockSize();
		*bytesRead = 0;
		while (size > 0) {
			size_t inBlockOffset = offset % blockSize;
			size_t toRead = min(size, size_t(blockSize - inBlockOffset));
			void *blockData = _GetBlockDataAt(offset / blockSize,
											  inBlockOffset, toRead);
D(
if (!blockData) {
	Node *node = NULL;
	if (Attribute *attribute = dynamic_cast<Attribute*>(this)) {
		FATAL(("attribute `%s' of\n", attribute->GetName()));
		node = attribute->GetNode();
	} else {
		node = dynamic_cast<Node*>(this);
	}
	if (node)
//		FATAL(("node `%s'\n", node->GetName()));
		FATAL(("container size: %Ld, offset: %Ld, buffer size: %lu\n",
		   fSize, offset, size));
		return B_ERROR;
}
);
			memcpy(buffer, blockData, toRead);
			buffer += toRead;
			size -= toRead;
			offset += toRead;
			*bytesRead += toRead;
		}
	}
	return error;
}

// WriteAt
status_t
DataContainer::WriteAt(off_t offset, const void *_buffer, size_t size,
					   size_t *bytesWritten)
{
//PRINT(("DataContainer::WriteAt(%Ld, %p, %lu, %p), fSize: %Ld\n", offset, _buffer, size, bytesWritten, fSize));
	const uint8 *buffer = (const uint8*)_buffer;
	status_t error = (buffer && offset >= 0 && bytesWritten
					  ? B_OK : B_BAD_VALUE);
	// resize the container, if necessary
	if (error == B_OK) {
		off_t newSize = offset + size;
		off_t oldSize = fSize;
		if (newSize > fSize) {
			error = Resize(newSize);
			// pad with zero, if necessary
			if (error == B_OK && offset > oldSize)
				_ClearArea(offset, oldSize - offset);
		}
	}
	if (error == B_OK) {
		// iterate through the blocks, writing as long as there's something
		// left to write
		size_t blockSize = fVolume->GetBlockSize();
		*bytesWritten = 0;
		while (size > 0) {
			size_t inBlockOffset = offset % blockSize;
			size_t toWrite = min(size, size_t(blockSize - inBlockOffset));
			void *blockData = _GetBlockDataAt(offset / blockSize,
											  inBlockOffset, toWrite);
D(if (!blockData) return B_ERROR;);
			memcpy(blockData, buffer, toWrite);
			buffer += toWrite;
			size -= toWrite;
			offset += toWrite;
			*bytesWritten += toWrite;
		}
	}
//PRINT(("DataContainer::WriteAt() done: %lx, fSize: %Ld\n", error, fSize));
	return error;
}

// GetFirstDataBlock
void
DataContainer::GetFirstDataBlock(const uint8 **data, size_t *length)
{
	if (data && length) {
		if (_IsBlockMode()) {
			BlockReference *block = _GetBlockList()->ItemAt(0);
			*data = (const uint8*)block->GetData();
			*length = min(fSize, fVolume->GetBlockSize());
		} else {
			*data = fSmallBuffer;
			*length = fSize;
		}
	}
}

// GetAllocationInfo
void
DataContainer::GetAllocationInfo(AllocationInfo &info)
{
	if (_IsBlockMode()) {
		BlockList *blocks = _GetBlockList();
		info.AddListAllocation(blocks->GetCapacity(), sizeof(BlockReference*));
		int32 blockCount = blocks->CountItems();
		for (int32 i = 0; i < blockCount; i++)
			info.AddBlockAllocation(blocks->ItemAt(i)->GetBlock()->GetSize());
	} else {
		// ...
	}
}

// _RequiresBlockMode
inline
bool
DataContainer::_RequiresBlockMode(size_t size)
{
	return (size > kSmallDataContainerSize);
}

// _IsBlockMode
inline
bool
DataContainer::_IsBlockMode() const
{
	return (fSize > kSmallDataContainerSize);
}

// _Resize
status_t
DataContainer::_Resize(off_t newSize)
{
//PRINT(("DataContainer::_Resize(%Ld), fSize: %Ld\n", newSize, fSize));
	status_t error = B_OK;
	if (newSize != fSize) {
		size_t blockSize = fVolume->GetBlockSize();
		int32 blockCount = _CountBlocks();
		int32 newBlockCount = (newSize + blockSize - 1) / blockSize;
		if (newBlockCount == blockCount) {
			// only the last block needs to be resized
			if (_IsBlockMode() && _RequiresBlockMode(newSize)) {
				// keep block mode
				error = _ResizeLastBlock((newSize - 1) % blockSize + 1);
			} else if (!_IsBlockMode() && !_RequiresBlockMode(newSize)) {
				// keep small buffer mode
				fSize = newSize;
			} else if (fSize < newSize) {
				// switch to block mode
				_SwitchToBlockMode(newSize);
			} else {
				// switch to small buffer mode
				_SwitchToSmallBufferMode(newSize);
			}
		} else if (newBlockCount < blockCount) {
			// shrink
			if (_IsBlockMode()) {
				// remove the last blocks
				BlockList *blocks = _GetBlockList();
				for (int32 i = blockCount - 1; i >= newBlockCount; i--) {
					BlockReference *block = blocks->ItemAt(i);
					blocks->RemoveItem(i);
					fVolume->FreeBlock(block);
					fSize = (fSize - 1) / blockSize * blockSize;
				}
				// resize the last block to the correct size, respectively
				// switch to small buffer mode
				if (_RequiresBlockMode(newSize))
					error = _ResizeLastBlock((newSize - 1) % blockSize + 1);
				else
					_SwitchToSmallBufferMode(newSize);
			} else {
				// small buffer mode: just set the new size
				fSize = newSize;
			}
		} else {
			// grow
			if (_RequiresBlockMode(newSize)) {
				// resize the first block to the correct size, respectively
				// switch to block mode
				if (_IsBlockMode())
					error = _ResizeLastBlock(blockSize);
				else {
					error = _SwitchToBlockMode(min((size_t)newSize,
												   blockSize));
				}
				// add new blocks
				BlockList *blocks = _GetBlockList();
				while (error == B_OK && fSize < newSize) {
					size_t newBlockSize = min(size_t(newSize - fSize),
											  blockSize);
					BlockReference *block = NULL;
					error = fVolume->AllocateBlock(newBlockSize, &block);
					if (error == B_OK) {
						if (blocks->AddItem(block))
							fSize += newBlockSize;
						else {
							SET_ERROR(error, B_NO_MEMORY);
							fVolume->FreeBlock(block);
						}
					}
				}
			} else {
				// no need to switch to block mode: just set the new size
				fSize = newSize;
			}
		}
	}
//PRINT(("DataContainer::_Resize() done: %lx, fSize: %Ld\n", error, fSize));
	return error;
}

// _GetBlockList
inline
DataContainer::BlockList *
DataContainer::_GetBlockList()
{
	return (BlockList*)fBlocks;
}

// _GetBlockList
inline
const DataContainer::BlockList *
DataContainer::_GetBlockList() const
{
	return (BlockList*)fBlocks;
}

// _CountBlocks
inline
int32
DataContainer::_CountBlocks() const
{
	if (_IsBlockMode())
		return _GetBlockList()->CountItems();
	else if (fSize == 0)	// small buffer mode, empty buffer
		return 0;
	return 1;	// small buffer mode, non-empty buffer
}

// _GetBlockDataAt
inline
void *
DataContainer::_GetBlockDataAt(int32 index, size_t offset, size_t DARG(size))
{
	if (_IsBlockMode()) {
		BlockReference *block = _GetBlockList()->ItemAt(index);
D(if (!fVolume->CheckBlock(block, offset + size)) return NULL;);
		return block->GetDataAt(offset);
	} else {
D(
if (offset + size > kSmallDataContainerSize) {
	FATAL(("DataContainer: Data access exceeds small buffer.\n"));
	PANIC("DataContainer: Data access exceeds small buffer.");
	return NULL;
}
);
		return fSmallBuffer + offset;
	}
}

// _ClearArea
void
DataContainer::_ClearArea(off_t offset, off_t size)
{
	// constrain the area to the data area
	offset = min(offset, fSize);
	size = min(size, fSize - offset);
	// iterate through the blocks, clearing as long as there's something
	// left to clear
	size_t blockSize = fVolume->GetBlockSize();
	while (size > 0) {
		size_t inBlockOffset = offset % blockSize;
		size_t toClear = min(size_t(size), blockSize - inBlockOffset);
		void *blockData = _GetBlockDataAt(offset / blockSize, inBlockOffset,
										  toClear);
D(if (!blockData) return;);
		memset(blockData, 0, toClear);
		size -= toClear;
		offset += toClear;
	}
}

// _ResizeLastBlock
status_t
DataContainer::_ResizeLastBlock(size_t newSize)
{
//PRINT(("DataContainer::_ResizeLastBlock(%lu), fSize: %Ld\n", newSize, fSize));
	int32 blockCount = _CountBlocks();
	status_t error = (fSize > 0 && blockCount > 0 && newSize > 0
					  ? B_OK : B_BAD_VALUE);
D(
if (!_IsBlockMode()) {
	FATAL(("Call of _ResizeLastBlock() in small buffer mode.\n"));
	PANIC("Call of _ResizeLastBlock() in small buffer mode.");
	return B_ERROR;
}
);
	if (error == B_OK) {
		size_t blockSize = fVolume->GetBlockSize();
		size_t oldSize = (fSize - 1) % blockSize + 1;
		if (newSize != oldSize) {
			BlockList *blocks = _GetBlockList();
			BlockReference *block = blocks->ItemAt(blockCount - 1);
			BlockReference *newBlock = fVolume->ResizeBlock(block, newSize);
			if (newBlock) {
				if (newBlock != block)
					blocks->ReplaceItem(blockCount - 1, newBlock);
				fSize += off_t(newSize) - oldSize;
			} else
				SET_ERROR(error, B_NO_MEMORY);
		}
	}
//PRINT(("DataContainer::_ResizeLastBlock() done: %lx, fSize: %Ld\n", error, fSize));
	return error;
}

// _SwitchToBlockMode
status_t
DataContainer::_SwitchToBlockMode(size_t newBlockSize)
{
	// allocate a new block
	BlockReference *block = NULL;
	status_t error = fVolume->AllocateBlock(newBlockSize, &block);
	if (error == B_OK) {
		// copy the data from the small buffer into the block
		if (fSize > 0)
			memcpy(block->GetData(), fSmallBuffer, fSize);
		// construct the block list and add the block
		new (fBlocks) BlockList(10);
		BlockList *blocks = _GetBlockList();
		if (blocks->AddItem(block)) {
			fSize = newBlockSize;
		} else {
			// error: destroy the block list and free the block
			SET_ERROR(error, B_NO_MEMORY);
			blocks->~BlockList();
			if (fSize > 0)
				memcpy(fSmallBuffer, block->GetData(), fSize);
			fVolume->FreeBlock(block);
		}
	}
	return error;
}

// _SwitchToSmallBufferMode
void
DataContainer::_SwitchToSmallBufferMode(size_t newSize)
{
	// remove the first (and only) block
	BlockList *blocks = _GetBlockList();
	BlockReference *block = blocks->ItemAt(0);
	blocks->RemoveItem(0L);
	// destroy the block list and copy the data into the small buffer
	blocks->~BlockList();
	if (newSize > 0)
		memcpy(fSmallBuffer, block->GetData(), newSize);
	// free the block and set the new size
	fVolume->FreeBlock(block);
	fSize = newSize;
}

