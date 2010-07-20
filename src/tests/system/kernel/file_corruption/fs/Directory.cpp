/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Directory.h"

#include <algorithm>
#include <new>

#include <AutoDeleter.h>

#include "Block.h"
#include "BlockAllocator.h"
#include "DebugSupport.h"
#include "Transaction.h"
#include "Volume.h"


class DirEntryBlock {
public:
								DirEntryBlock();
								DirEntryBlock(
									checksumfs_dir_entry_block* entryBlock,
									size_t entryBlockSize);

			void				SetTo(checksumfs_dir_entry_block* entryBlock,
									size_t entryBlockSize);

	inline	int32				EntryCount() const;
	inline	size_t				BytesUsedFor(int32 entryCount) const;
	inline	size_t				BytesUsed() const;
	inline	size_t				FreeSpace() const;

	inline	uint64				BlockIndexAt(int32 index) const;
			const char*			NameAt(int32 index, size_t& _nameLength) const;

			int32				FindInsertionIndex(const char* name,
									size_t nameLength, bool& _exactMatch) const;

			int32				FindSplitIndex(int32 index,
									size_t bytesNeeded) const;

			void				InsertEntry(int32 index, const char* name,
									size_t nameLength, uint64 blockIndex);
			void				ReplaceEntryName(int32 index, const char* name,
									size_t nameLength);
			void				RemoveEntry(int32 index);

			void				SplitBlock(int32 splitIndex,
									DirEntryBlock& other);

			bool				Check() const;

private:
			checksumfs_dir_entry_block* fBlock;
			size_t				fBlockSize;
};


class DirEntryTree {
public:
								DirEntryTree(Directory* directory);

			status_t			LookupEntry(const char* name,
									uint64& _blockIndex);
			status_t			LookupNextEntry(const char* name,
									char* foundName, size_t& _foundNameLength,
									uint64& _blockIndex);

			status_t			InsertEntry(const char* name, uint64 blockIndex,
									Transaction& transaction);
			status_t			RemoveEntry(const char* name,
									Transaction& transaction);

			status_t			FreeTree(Transaction& transaction);

			bool				IsEmpty() const;

			bool				Check();

private:
			struct LevelInfo {
				Block			block;
				DirEntryBlock	entryBlock;
				int32			index;
				bool			exactMatch;
			};

private:
			status_t			_InitReadOnly();
			status_t			_InitWritable(Transaction& transaction);
			status_t			_InitCommon();

			status_t			_UpdateOrInsertKey(LevelInfo* infos,
									int32 level, const char* name,
									size_t nameLength, uint64 blockIndex,
									bool insertKey, Transaction& transaction);

			status_t			_InsertEntryIncrementDepth(LevelInfo* infos,
									Transaction& transaction);
			status_t			_InsertEntrySplitBlock(int32 level,
									LevelInfo& info, size_t needed,
									Transaction& transaction, Block& newBlock,
									int32& _splitIndex);

			bool				_Check(int32 level, uint64 blockIndex,
									const char* key, size_t keyLength,
									DirEntryBlock& entryBlock);

	inline	uint16				_Depth() const	{ return fTree->depth; }

private:
			Directory*			fDirectory;
			Block				fRootBlock;
			checksumfs_dir_entry_tree* fTree;
			checksumfs_dir_entry_block* fRootEntryBlock;
			size_t				fRootEntryBlockSize;
};


// #pragma mark -


static int
compare_names(const char* a, size_t lengthA, const char* b, size_t lengthB)
{
	int cmp = strncmp(a, b, std::min(lengthA, lengthB));
	if (cmp != 0)
		return cmp;

	return (int)lengthA - (int)lengthB;
		// assumes we don't overflow 31 bits
}


// #pragma mark - DirEntryBlock


DirEntryBlock::DirEntryBlock()
	:
	fBlock(NULL),
	fBlockSize(0)
{
}


DirEntryBlock::DirEntryBlock(checksumfs_dir_entry_block* entryBlock,
	size_t entryBlockSize)
	:
	fBlock(entryBlock),
	fBlockSize(entryBlockSize)
{
}


void
DirEntryBlock::SetTo(checksumfs_dir_entry_block* entryBlock,
	size_t entryBlockSize)
{
	fBlock = entryBlock;
	fBlockSize = entryBlockSize;
}


int32
DirEntryBlock::EntryCount() const
{
	return fBlock->entryCount;
}


size_t
DirEntryBlock::BytesUsedFor(int32 entryCount) const
{
	if (entryCount == 0)
		return sizeof(*fBlock);
	return sizeof(*fBlock) + 10 * entryCount
		+ fBlock->nameEnds[entryCount - 1];
}


size_t
DirEntryBlock::BytesUsed() const
{
	return BytesUsedFor(EntryCount());
}


size_t
DirEntryBlock::FreeSpace() const
{
	return fBlockSize - BytesUsed();
}


uint64
DirEntryBlock::BlockIndexAt(int32 index) const
{
	uint64* blockIndices
		= (uint64*)((uint8*)fBlock + fBlockSize) - 1;
	return blockIndices[-index];
}


const char*
DirEntryBlock::NameAt(int32 index, size_t& _nameLength) const
{
	int32 entryCount = EntryCount();
	if (index < 0 || index >= entryCount)
		return NULL;

	uint32 nameOffset = index > 0 ? fBlock->nameEnds[index - 1] : 0;
	_nameLength = fBlock->nameEnds[index] - nameOffset;
	return (const char*)(fBlock->nameEnds + entryCount) + nameOffset;
}


int32
DirEntryBlock::FindInsertionIndex(const char* name, size_t nameLength,
	bool& _exactMatch) const
{
	int32 entryCount = EntryCount();
	if (entryCount == 0) {
		_exactMatch = false;
		return 0;
	}

	const char* entryNames = (char*)(fBlock->nameEnds + entryCount);
	uint32 nameOffset = 0;

	int32 index = 0;
	int cmp = -1;

	// TODO: Binary search!
	for (; index < entryCount; index++) {
		const char* entryName = entryNames + nameOffset;
		size_t entryNameLength = fBlock->nameEnds[index] - nameOffset;

		cmp = compare_names(entryName, entryNameLength, name, nameLength);
		if (cmp >= 0)
			break;

		nameOffset += entryNameLength;
	}

	_exactMatch = cmp == 0;
	return index;
}


/*!	Finds a good split index for an insertion of \a bytesNeeded bytes at
	index \a index.
*/
int32
DirEntryBlock::FindSplitIndex(int32 index, size_t bytesNeeded) const
{
	size_t splitSize = (BytesUsed() + bytesNeeded) / 2;

	int32 entryCount = EntryCount();
	for (int32 i = 0; i < entryCount; i++) {
		size_t bytesUsed = BytesUsedFor(i + 1);
		if (i == index)
			bytesUsed += bytesNeeded;
		if (bytesUsed > splitSize)
			return i;
	}

	// This should never happen.
	return entryCount;
}


void
DirEntryBlock::InsertEntry(int32 index, const char* name, size_t nameLength,
	uint64 blockIndex)
{
	uint64* blockIndices = (uint64*)((uint8*)fBlock + fBlockSize) - 1;
	int32 entryCount = fBlock->entryCount;
	char* entryNames = (char*)(fBlock->nameEnds + entryCount);

	uint32 nameOffset = index == 0 ? 0 : fBlock->nameEnds[index - 1];
	uint32 lastNameEnd = entryCount == 0
		? 0 : fBlock->nameEnds[entryCount - 1];

	if (index < entryCount) {
		// make room in the block indices array
		memmove(&blockIndices[-entryCount], &blockIndices[1 - entryCount],
			8 * (entryCount - index));

		// make room in the name array -- we also move 2 bytes more for the
		// new entry in the nameEnds array
		memmove(entryNames + nameOffset + nameLength + 2,
			entryNames + nameOffset, lastNameEnd - nameOffset);

		// move the names < index by 2 bytes
		if (index > 0)
			memmove(entryNames + 2, entryNames, nameOffset);

		// move and update the nameEnds entries > index
		for (int32 i = entryCount; i > index; i--)
			fBlock->nameEnds[i] = fBlock->nameEnds[i - 1] + nameLength;
	} else if (entryCount > 0) {
		// the nameEnds array grows -- move all names
		memmove(entryNames + 2, entryNames, lastNameEnd);
	}

	// we have made room -- insert the entry
	entryNames += 2;
	memcpy(entryNames + nameOffset, name, nameLength);
	fBlock->nameEnds[index] = nameOffset + nameLength;
	blockIndices[-index] = blockIndex;
	fBlock->entryCount++;
ASSERT(Check());
}


void
DirEntryBlock::ReplaceEntryName(int32 index, const char* name,
	size_t nameLength)
{
	int32 entryCount = fBlock->entryCount;
	char* entryNames = (char*)(fBlock->nameEnds + entryCount);

	ASSERT(index >= 0 && index < entryCount);

	uint32 nameOffset = index == 0 ? 0 : fBlock->nameEnds[index - 1];
	uint32 oldNameLength = fBlock->nameEnds[index] - nameOffset;
	uint32 lastNameEnd = fBlock->nameEnds[entryCount - 1];

	if (oldNameLength != nameLength) {
		int32 lengthDiff = (int32)nameLength - (int32)oldNameLength;

		ASSERT(lengthDiff <= (int32)FreeSpace());

		// move names after the changing name
		if (index + 1 < entryCount) {
			memmove(entryNames + nameOffset + nameLength,
				entryNames + nameOffset + oldNameLength,
				lastNameEnd - nameOffset - oldNameLength);
		}

		// update the name ends
		for (int32 i = index; i < entryCount; i++)
			fBlock->nameEnds[i] = (int32)fBlock->nameEnds[i] + lengthDiff;
	}

	// copy the name
	memcpy(entryNames + nameOffset, name, nameLength);
ASSERT(Check());
}


void
DirEntryBlock::RemoveEntry(int32 index)
{
	ASSERT(index >= 0 && index < EntryCount());

	int32 entryCount = EntryCount();
	if (entryCount == 1) {
		// simple case -- removing the last entry
		fBlock->entryCount = 0;
		return;
	}

	uint64* blockIndices = (uint64*)((uint8*)fBlock + fBlockSize) - 1;
	char* entryNames = (char*)(fBlock->nameEnds + entryCount);

	uint32 nameOffset = index == 0 ? 0 : fBlock->nameEnds[index - 1];
	uint32 nameEnd = fBlock->nameEnds[index];
	uint32 lastNameEnd = entryCount == 0
		? 0 : fBlock->nameEnds[entryCount - 1];

	if (index < entryCount - 1) {
		uint32 nameLength = nameEnd - nameOffset;

		// remove the element from the block indices array
		memmove(&blockIndices[-entryCount + 2], &blockIndices[-entryCount + 1],
			8 * (entryCount - index - 1));

		// move and update the nameEnds entries > index
		for (int32 i = index + 1; i < entryCount; i++)
			fBlock->nameEnds[i - 1] = fBlock->nameEnds[i] - nameLength;

		// move the names < index by 2 bytes
		if (index > 0)
			memmove(entryNames - 2, entryNames, nameOffset);

		// move the names > index
		memmove(entryNames - 2 + nameOffset, entryNames + nameEnd,
			lastNameEnd - nameEnd);
	} else {
		// the nameEnds array shrinks -- move all names
		memmove(entryNames - 2, entryNames, nameOffset);
	}

	// we have removed the entry
	fBlock->entryCount--;
ASSERT(Check());
}


/*!	Moves all entries beyond \a splitIndex (inclusively) to the empty block
	\a other.
*/
void
DirEntryBlock::SplitBlock(int32 splitIndex, DirEntryBlock& other)
{
	ASSERT(other.EntryCount() == 0);
	ASSERT(splitIndex <= EntryCount());

	int32 entryCount = EntryCount();
	if (splitIndex == entryCount)
		return;
	int32 otherEntryCount = entryCount - splitIndex;

	// copy block indices
	uint64* blockIndices = (uint64*)((uint8*)fBlock + fBlockSize);
	uint64* otherBlockIndices
		= (uint64*)((uint8*)other.fBlock + other.fBlockSize);
		// note: both point after the last entry, unlike in other methods
	memcpy(otherBlockIndices - otherEntryCount, blockIndices - entryCount,
		8 * otherEntryCount);

	// copy the name end offsets
	uint32 namesOffset = splitIndex > 0
		? fBlock->nameEnds[splitIndex - 1] : 0;
	for (int32 i = splitIndex; i < entryCount; i++) {
		other.fBlock->nameEnds[i - splitIndex] = fBlock->nameEnds[i]
			- namesOffset;
	}

	// copy the names
	char* entryNames = (char*)(fBlock->nameEnds + entryCount);
	char* otherEntryNames
		= (char*)(other.fBlock->nameEnds + otherEntryCount);
	memcpy(otherEntryNames, entryNames + namesOffset,
		fBlock->nameEnds[entryCount - 1] - namesOffset);

	// the name ends array shrinks -- move the names
	if (splitIndex > 0) {
		char* newEntryNames = (char*)(fBlock->nameEnds + splitIndex);
		memmove(newEntryNames, entryNames, namesOffset);
	}

	// update the entry counts
	fBlock->entryCount = splitIndex;
	other.fBlock->entryCount = otherEntryCount;
ASSERT(Check());
ASSERT(other.Check());
}


bool
DirEntryBlock::Check() const
{
	int32 entryCount = EntryCount();
	if (entryCount == 0)
		return true;

	// Check size: Both name ends and block index arrays must fit and we need
	// at least one byte per name.
	size_t size = sizeof(*fBlock) + entryCount * 10;
	if (size + entryCount > fBlockSize) {
		ERROR("Invalid dir entry block: entry count %d requires minimum size "
			"of %" B_PRIuSIZE " + %d bytes, but block size is %" B_PRIuSIZE
			"\n", (int)entryCount, size, (int)entryCount, fBlockSize);
		return false;
	}

	// check the name ends and block indices arrays and the names
	const char* entryNames = (char*)(fBlock->nameEnds + entryCount);
	const uint64* blockIndices = (uint64*)((uint8*)fBlock + fBlockSize) - 1;
	const char* previousName = NULL;
	uint16 previousNameLength = 0;
	uint16 previousEnd = 0;

	for (int32 i = 0; i < entryCount; i++) {
		// check name end
		uint16 nameEnd = fBlock->nameEnds[i];
		if (nameEnd <= previousEnd || nameEnd > fBlockSize - size) {
			ERROR("Invalid dir entry block: name end offset of entry %" B_PRId32
				": %u, previous: %u\n", i, nameEnd, previousEnd);
			return false;
		}

		// check name length
		uint16 nameLength = nameEnd - previousEnd;
		if (nameLength > kCheckSumFSNameLength) {
			ERROR("Invalid dir entry block: name of entry %" B_PRId32 " too "
				"long: %u\n", i, nameLength);
			return false;
		}

		// verify that the name doesn't contain a null char
		const char* name = entryNames + previousEnd;
		if (strnlen(name, nameLength) != nameLength) {
			ERROR("Invalid dir entry block: name of entry %" B_PRId32
				" contains a null char\n", i);
			return false;
		}

		// compare the name with the previous name
		if (i > 0) {
			int cmp = compare_names(previousName, previousNameLength, name,
				nameLength);
			if (cmp == 0) {
				ERROR("Invalid dir entry block: entries %" B_PRId32 "/%"
					B_PRId32 " have the same name: \"%.*s\"\n", i - 1, i,
					(int)nameLength, name);
				return false;
			} else if (cmp > 0) {
				ERROR("Invalid dir entry block: entries %" B_PRId32 "/%"
					B_PRId32 " out of order: \"%.*s\" > \"%.*s\"\n", i - 1, i,
					(int)previousNameLength, previousName, (int)nameLength,
					name);
				return false;
			}
		}

		// check the block index
		if (blockIndices[-i] < kCheckSumFSSuperBlockOffset / B_PAGE_SIZE) {
			ERROR("Invalid dir entry block: entry %" B_PRId32
				" has invalid block index: %" B_PRIu64, i, blockIndices[-i]);
			return false;
		}

		previousName = name;
		previousNameLength = nameLength;
		previousEnd = nameEnd;
	}

	return true;
}


// #pragma mark - DirEntryTree


DirEntryTree::DirEntryTree(Directory* directory)
	:
	fDirectory(directory)
{
}


status_t
DirEntryTree::LookupEntry(const char* name, uint64& _blockIndex)
{
	FUNCTION("name: \"%s\"\n", name);

	status_t error = _InitReadOnly();
	if (error != B_OK)
		RETURN_ERROR(error);

	size_t nameLength = strlen(name);
	if (nameLength > kCheckSumFSNameLength)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	uint32 depth = _Depth();

	DirEntryBlock entryBlock(fRootEntryBlock, fRootEntryBlockSize);
ASSERT(entryBlock.Check());

	Block block;

	for (uint32 level = 0; level <= depth; level++) {
		if (entryBlock.EntryCount() == 0)
			RETURN_ERROR(level == 0 ? B_ENTRY_NOT_FOUND : B_BAD_DATA);

		bool exactMatch;
		int32 index = entryBlock.FindInsertionIndex(name, nameLength,
			exactMatch);

		// If we haven't found an exact match, the index points to the first
		// entry that is greater or after the last entry.
		if (!exactMatch) {
			if (index == 0) {
				// The first entry is already greater, so the branch doesn't
				// contain the entry we're looking for.
				RETURN_ERROR(B_ENTRY_NOT_FOUND);
			}

			index--;
		}

		PRINT("  level %" B_PRId32 " -> index: %" B_PRId32 " %sexact\n", level,
			index, exactMatch ? "" : " not ");

		uint64 blockIndex = entryBlock.BlockIndexAt(index);

		if (level == depth) {
			// final level -- here we should have an exact match
			if (!exactMatch)
				RETURN_ERROR(B_ENTRY_NOT_FOUND);

			_blockIndex = blockIndex;
			return B_OK;
		}

		// not the final level -- load the block and descend to the next
		// level
		if (!block.GetReadable(fDirectory->GetVolume(), blockIndex))
			RETURN_ERROR(B_ERROR);

		entryBlock.SetTo((checksumfs_dir_entry_block*)block.Data(),
			B_PAGE_SIZE);
ASSERT(entryBlock.Check());
	}

	// cannot get here, but keep the compiler happy
	RETURN_ERROR(B_ENTRY_NOT_FOUND);
}


status_t
DirEntryTree::LookupNextEntry(const char* name, char* foundName,
	size_t& _foundNameLength, uint64& _blockIndex)
{
	FUNCTION("name: \"%s\"\n", name);

	status_t error = _InitReadOnly();
	if (error != B_OK)
		RETURN_ERROR(error);

	size_t nameLength = strlen(name);
	if (nameLength > kCheckSumFSNameLength)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	int32 depth = _Depth();

	LevelInfo* infos = new(std::nothrow) LevelInfo[
		kCheckSumFSMaxDirEntryTreeDepth + 1];
	if (infos == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ArrayDeleter<LevelInfo> infosDeleter(infos);

	infos[0].entryBlock.SetTo(fRootEntryBlock, fRootEntryBlockSize);
ASSERT(infos[0].entryBlock.Check());

	// descend the tree
	for (int32 level = 0; level <= depth; level++) {
		LevelInfo& info = infos[level];

		if (info.entryBlock.EntryCount() == 0) {
			if (level == 0) {
				// directory is empty
				return B_ENTRY_NOT_FOUND;
			}

			RETURN_ERROR(B_BAD_DATA);
		}

		info.index = info.entryBlock.FindInsertionIndex(name, nameLength,
			info.exactMatch);

		PRINT("  level %" B_PRId32 " -> index: %" B_PRId32 " %sexact\n", level,
			info.index, info.exactMatch ? "" : " not ");

		if (level == depth)
			break;

		// If we haven't found an exact match, the index points to the first
		// entry that is greater or after the last entry.
		if (!info.exactMatch && info.index > 0)
			info.index--;

		uint64 nextBlockIndex = info.entryBlock.BlockIndexAt(info.index);

		// not the final level -- load the block and descend to the next
		// level
		LevelInfo& nextInfo = infos[level + 1];
		if (!nextInfo.block.GetReadable(fDirectory->GetVolume(),
				nextBlockIndex)) {
			RETURN_ERROR(B_ERROR);
		}

		nextInfo.entryBlock.SetTo(
			(checksumfs_dir_entry_block*)nextInfo.block.Data(),
			B_PAGE_SIZE);
ASSERT(nextInfo.entryBlock.Check());
	}

	if (infos[depth].exactMatch)
		infos[depth].index++;

	if (infos[depth].index >= infos[depth].entryBlock.EntryCount()) {
		// We're at the end of the last block -- we need to track back to find a
		// greater branch.
		PRINT("  searching for greater branch\n");

		int32 level;
		for (level = depth - 1; level >= 0; level--) {
			LevelInfo& info = infos[level];
			if (++info.index < info.entryBlock.EntryCount()) {
				PRINT("  found greater branch: level: %" B_PRId32 " -> index: %"
					B_PRId32 "\n", level, info.index);
				break;
			}
		}

		if (level < 0)
			return B_ENTRY_NOT_FOUND;

		// We've found a greater branch -- get the first entry in that branch.
		for (level++; level <= depth; level++) {
			LevelInfo& previousInfo = infos[level - 1];
			LevelInfo& info = infos[level];

			uint64 nextBlockIndex = previousInfo.entryBlock.BlockIndexAt(
				previousInfo.index);

			// load the block
			if (!info.block.GetReadable(fDirectory->GetVolume(),
					nextBlockIndex)) {
				RETURN_ERROR(B_ERROR);
			}

			info.entryBlock.SetTo(
				(checksumfs_dir_entry_block*)info.block.Data(), B_PAGE_SIZE);
ASSERT(info.entryBlock.Check());

			info.index = 0;
			if (info.entryBlock.EntryCount() == 0)
				RETURN_ERROR(B_BAD_DATA);
		}
	}

	// get and check the name
	LevelInfo& info = infos[depth];

	name = info.entryBlock.NameAt(info.index, nameLength);
	if (nameLength > kCheckSumFSNameLength
		|| strnlen(name, nameLength) != nameLength) {
		RETURN_ERROR(B_BAD_DATA);
	}

	// set the return values
	memcpy(foundName, name, nameLength);
	foundName[nameLength] = '\0';
	_foundNameLength = nameLength;
	_blockIndex = info.entryBlock.BlockIndexAt(info.index);

	PRINT("  found entry: \"%s\" -> %" B_PRIu64 "\n", foundName, _blockIndex);

	return B_OK;
}


status_t
DirEntryTree::InsertEntry(const char* name, uint64 blockIndex,
	Transaction& transaction)
{
	FUNCTION("name: \"%s\", blockIndex: %" B_PRIu64 "\n", name, blockIndex);

	status_t error = _InitWritable(transaction);
	if (error != B_OK)
		RETURN_ERROR(error);

	size_t nameLength = strlen(name);
	if (nameLength == 0)
		RETURN_ERROR(B_BAD_VALUE);
	if (nameLength > kCheckSumFSNameLength)
		RETURN_ERROR(B_NAME_TOO_LONG);

	int32 depth = _Depth();

	LevelInfo* infos = new(std::nothrow) LevelInfo[
		kCheckSumFSMaxDirEntryTreeDepth + 1];
	if (infos == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ArrayDeleter<LevelInfo> infosDeleter(infos);

	infos[0].entryBlock.SetTo(fRootEntryBlock, fRootEntryBlockSize);

	for (int32 level = 0; level <= depth; level++) {
		LevelInfo& info = infos[level];

		if (info.entryBlock.EntryCount() == 0) {
			if (level == 0) {
				PRINT("  directory is empty\n");
				// directory is empty
				info.index = 0;
				break;
			}

			RETURN_ERROR(B_BAD_DATA);
		}

		info.index = info.entryBlock.FindInsertionIndex(name, nameLength,
			info.exactMatch);

		PRINT("  level %" B_PRId32 ", block %" B_PRIu64 " -> index: %" B_PRId32
			" %sexact\n", level,
			level == 0 ? fDirectory->BlockIndex() : info.block.Index(),
			info.index, info.exactMatch ? "" : " not ");

		// Finding an exact match -- even in the non-final level -- means
		// that there's an entry with that name.
		if (info.exactMatch)
			RETURN_ERROR(B_FILE_EXISTS);

		if (level == depth) {
			// final level -- here we need to insert the entry
			break;
		}

		// Since we haven't found an exact match, the index points to the
		// first entry that is greater or after the last entry.
		info.index--;

		uint64 nextBlockIndex = info.entryBlock.BlockIndexAt(info.index);

		// not the final level -- load the block and descend to the next
		// level
		LevelInfo& nextInfo = infos[level + 1];
		if (!nextInfo.block.GetReadable(fDirectory->GetVolume(),
				nextBlockIndex)) {
			RETURN_ERROR(B_ERROR);
		}

		nextInfo.entryBlock.SetTo(
			(checksumfs_dir_entry_block*)nextInfo.block.Data(),
			B_PAGE_SIZE);
ASSERT(nextInfo.entryBlock.Check());
	}

	// We've found the insertion point. Insert the key and iterate backwards
	// to perform the potentially necessary updates. Insertion at index 0 of
	// the block changes the block's key, requiring an update in the parent
	// block. Insertion or key update can cause the block to be split (if
	// there's not enough space left in it), requiring an insertion in the
	// parent block. So we start with a pending insertion in the leaf block
	// and work our way upwards, performing key updates and insertions as
	// necessary.

	return _UpdateOrInsertKey(infos, depth, name, nameLength, blockIndex, true,
		transaction);
}


status_t
DirEntryTree::RemoveEntry(const char* name, Transaction& transaction)
{
	FUNCTION("name: \"%s\"\n", name);

	status_t error = _InitWritable(transaction);
	if (error != B_OK)
		RETURN_ERROR(error);

	size_t nameLength = strlen(name);
	if (nameLength == 0)
		RETURN_ERROR(B_BAD_VALUE);
	if (nameLength > kCheckSumFSNameLength)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	int32 depth = _Depth();

	LevelInfo* infos = new(std::nothrow) LevelInfo[
		kCheckSumFSMaxDirEntryTreeDepth + 1];
	if (infos == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ArrayDeleter<LevelInfo> infosDeleter(infos);

	infos[0].entryBlock.SetTo(fRootEntryBlock, fRootEntryBlockSize);

	for (int32 level = 0; level <= depth; level++) {
		LevelInfo& info = infos[level];

		if (info.entryBlock.EntryCount() == 0) {
			if (level == 0) {
				// directory is empty
				PRINT("  directory is empty\n");
				RETURN_ERROR(B_ENTRY_NOT_FOUND);
			}

			RETURN_ERROR(B_BAD_DATA);
		}

		info.index = info.entryBlock.FindInsertionIndex(name, nameLength,
			info.exactMatch);

		PRINT("  level %" B_PRId32 ", block %" B_PRIu64 " -> index: %" B_PRId32
			" %sexact\n", level,
			level == 0 ? fDirectory->BlockIndex() : info.block.Index(),
			info.index, info.exactMatch ? "" : " not ");

		if (level == depth) {
			// final level -- here the entry should be found
			if (!info.exactMatch)
				RETURN_ERROR(B_ENTRY_NOT_FOUND);
			break;
		}

		// If we haven't found an exact match, the index points to the first
		// entry that is greater or after the last entry.
		if (!info.exactMatch) {
			if (info.index == 0) {
				// The first entry is already greater, so the branch doesn't
				// contain the entry we're looking for.
				RETURN_ERROR(B_ENTRY_NOT_FOUND);
			}

			info.index--;
		}

		uint64 nextBlockIndex = info.entryBlock.BlockIndexAt(info.index);

		// not the final level -- load the block and descend to the next
		// level
		LevelInfo& nextInfo = infos[level + 1];
		if (!nextInfo.block.GetReadable(fDirectory->GetVolume(),
				nextBlockIndex)) {
			RETURN_ERROR(B_ERROR);
		}

		nextInfo.entryBlock.SetTo(
			(checksumfs_dir_entry_block*)nextInfo.block.Data(),
			B_PAGE_SIZE);
ASSERT(nextInfo.entryBlock.Check());
	}

	// We've found the entry. Insert the key and iterate backwards to perform
	// the potentially necessary updates. Removal at index 0 of the block
	// changes the block's key, requiring an update in the parent block.
	// Removal of the last entry will require removal of the block from its
	// parent. Key update can cause the block to be split (if there's not
	// enough space left in it), requiring an insertion in the parent block.
	// We start with a pending removal in the leaf block and work our way
	// upwards as long as the blocks become empty. As soon as a key update is
	// required, we delegate the remaining to the update/insert backwards loop.

	for (int32 level = depth; level >= 0; level--) {
		LevelInfo& info = infos[level];

		// make the block writable
		if (level > 0) {
			error = info.block.MakeWritable(transaction);
			if (error != B_OK)
				RETURN_ERROR(error);
		}

		PRINT("  level: %" B_PRId32 ", index: %" B_PRId32 ": removing key "
			"\"%.*s\" (%" B_PRIuSIZE ")\n", level, info.index, (int)nameLength,
			name, nameLength);

		if (info.entryBlock.EntryCount() == 1) {
			// That's the last key in the block. Unless that's the root level,
			// we remove the block completely.
			PRINT("  -> block is empty\n");
			if (level == 0) {
				info.entryBlock.RemoveEntry(info.index);
				return B_OK;
			}

			error = fDirectory->GetVolume()->GetBlockAllocator()->Free(
				info.block.Index(), 1, transaction);
			if (error != B_OK)
				RETURN_ERROR(error);
			fDirectory->SetSize(fDirectory->Size() - B_PAGE_SIZE);

			// remove the key (the same one) from the parent block
			continue;
		}

		// There are more entries, so just remove the entry in question. If it
		// is not the first one, we're done, otherwise we have to update the
		// block's key in the parent block.
		info.entryBlock.RemoveEntry(info.index);

		if (info.index > 0 || level == 0)
			return B_OK;

		name = info.entryBlock.NameAt(0, nameLength);
		return _UpdateOrInsertKey(infos, level - 1, name, nameLength, 0, false,
			transaction);
	}

	return B_OK;
}


status_t
DirEntryTree::FreeTree(Transaction& transaction)
{
	status_t error = _InitReadOnly();
	if (error != B_OK)
		RETURN_ERROR(error);

	int32 depth = _Depth();
	if (depth == 0)
		return B_OK;

	LevelInfo* infos = new(std::nothrow) LevelInfo[
		kCheckSumFSMaxDirEntryTreeDepth + 1];
	if (infos == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ArrayDeleter<LevelInfo> infosDeleter(infos);

	infos[0].entryBlock.SetTo(fRootEntryBlock, fRootEntryBlockSize);
	infos[0].index = 0;

	// Iterate through the tree in post order. We don't touch the content of
	// any block, we only free the blocks.
	int32 level = 0;
	while (true) {
		LevelInfo& info = infos[level];

		if (level == depth || info.index >= info.entryBlock.EntryCount()) {
			// we're through with the block
			if (level == 0)
				break;

			// free it
			error = fDirectory->GetVolume()->GetBlockAllocator()->Free(
				info.block.Index(), 1, transaction);

			// continue with the next sibling branch
			infos[--level].index++;
		}

		// descend to next level
		uint64 nextBlockIndex = info.entryBlock.BlockIndexAt(info.index);

		LevelInfo& nextInfo = infos[++level];
		if (!nextInfo.block.GetReadable(fDirectory->GetVolume(),
				nextBlockIndex)) {
			RETURN_ERROR(B_ERROR);
		}

		nextInfo.entryBlock.SetTo(
			(checksumfs_dir_entry_block*)nextInfo.block.Data(),
			B_PAGE_SIZE);
	}

	return B_OK;
}


bool
DirEntryTree::IsEmpty() const
{
	DirEntryBlock entryBlock(fRootEntryBlock, fRootEntryBlockSize);
	return entryBlock.EntryCount() == 0;
}


bool
DirEntryTree::Check()
{
	if (_InitReadOnly() != B_OK) {
		ERROR("DirEntryTree::Check(): Init failed!\n");
		return false;
	}

	DirEntryBlock entryBlock(fRootEntryBlock, fRootEntryBlockSize);
	return _Check(0, fDirectory->BlockIndex(), NULL, 0, entryBlock);
}


status_t
DirEntryTree::_InitReadOnly()
{
	if (!fRootBlock.GetReadable(fDirectory->GetVolume(),
			fDirectory->BlockIndex())) {
		RETURN_ERROR(B_ERROR);
	}

	return _InitCommon();
}


status_t
DirEntryTree::_InitWritable(Transaction& transaction)
{
	if (!fRootBlock.GetWritable(fDirectory->GetVolume(),
			fDirectory->BlockIndex(), transaction)) {
		RETURN_ERROR(B_ERROR);
	}

	return _InitCommon();
}


status_t
DirEntryTree::_InitCommon()
{
	fTree = (checksumfs_dir_entry_tree*)
		((uint8*)fRootBlock.Data() + sizeof(checksumfs_node));

	fRootEntryBlock = (checksumfs_dir_entry_block*)(fTree + 1);
	fRootEntryBlockSize = B_PAGE_SIZE
		- ((addr_t)fRootEntryBlock - (addr_t)fRootBlock.Data());

	if (_Depth() > kCheckSumFSMaxDirEntryTreeDepth)
		RETURN_ERROR(B_BAD_DATA);

	return B_OK;
}


status_t
DirEntryTree::_UpdateOrInsertKey(LevelInfo* infos, int32 level,
	const char* name, size_t nameLength, uint64 blockIndex, bool insertKey,
	Transaction& transaction)
{
	FUNCTION("level: %" B_PRId32 ": %s name: \"%.*s\" (%" B_PRIuSIZE "), "
		"blockIndex: %" B_PRIu64 "\n", level, insertKey ? "insert" : "update",
		(int)nameLength, name, nameLength, blockIndex);

	// Some temporary blocks: newBlock is used when a block is split. The
	// other three are used when a key update respectively insertion in the
	// parent block becomes necessary. We only need them, since the name
	// we update/insert is potentially from a block and instead of cloning
	// the name, we simple postpone putting the block until we don't need
	// the name anymore.
	Block newBlock;
	Block tempBlockUpdate;
	Block tempBlockUpdateInsert;
	Block tempBlockInsert;

	int32 depth = _Depth();
	status_t error;

	bool updateNextKey = !insertKey;
	bool insertNextKey = insertKey;
	const char* nameToUpdate = name;
	size_t nameToUpdateLength = nameLength;
	const char* nextNameToInsert = name;
	size_t nextNameToInsertLength = nameLength;
	uint64 nextBlockIndexToInsert = blockIndex;

	for (; level >= 0; level--) {
		LevelInfo& info = infos[level];

		bool updateThisKey = updateNextKey;
		bool insertThisKey = insertNextKey;

		if (!updateThisKey && !insertThisKey)
			return B_OK;

		updateNextKey = false;
		insertNextKey = false;

		blockIndex = nextBlockIndexToInsert;
		name = nextNameToInsert;
		nameLength = nextNameToInsertLength;

		// make the block writable
		if (level > 0) {
			error = info.block.MakeWritable(transaction);
			if (error != B_OK)
				RETURN_ERROR(error);
		}

		if (updateThisKey) {
			PRINT("  level: %" B_PRId32 ", index: %" B_PRId32 ": updating key "
				"to \"%.*s\" (%" B_PRIuSIZE ")\n", level, info.index,
				(int)nameToUpdateLength, nameToUpdate, nameToUpdateLength);

			size_t oldNameLength;
			info.entryBlock.NameAt(info.index, oldNameLength);
			size_t spaceNeeded = oldNameLength < nameToUpdateLength
				? nameToUpdateLength - oldNameLength : 0;

			if (spaceNeeded <= info.entryBlock.FreeSpace()) {
				info.entryBlock.ReplaceEntryName(info.index, nameToUpdate,
					nameToUpdateLength);
				if (info.index == 0) {
					// we updated at index 0, so we need to update this
					// block's key in the parent block
					updateNextKey = true;
					nameToUpdate = info.entryBlock.NameAt(0,
						nameToUpdateLength);

					// make sure the new block is kept until we no longer
					// use the name in the next iteration
					tempBlockUpdate.TransferFrom(info.block);
				}
			} else if (level == 0) {
				// We need to split the root block -- clone it first.
				error = _InsertEntryIncrementDepth(infos, transaction);
				if (error != B_OK)
					RETURN_ERROR(error);

				level = 2;
					// _InsertEntryIncrementDepth() moved the root block
					// content to level 1, where we want to continue.
				updateNextKey = true;
				insertNextKey = insertThisKey;
				continue;
			} else {
				// We need to split this non-root block.
				int32 splitIndex;
				error = _InsertEntrySplitBlock(level, info, spaceNeeded,
					transaction, newBlock, splitIndex);
				if (error != B_OK)
					RETURN_ERROR(error);

				nextBlockIndexToInsert = newBlock.Index();

				DirEntryBlock newEntryBlock(
					(checksumfs_dir_entry_block*)newBlock.Data(),
					B_PAGE_SIZE);
ASSERT(newEntryBlock.Check());

				if (info.index < splitIndex) {
					ASSERT(info.entryBlock.FreeSpace() >= spaceNeeded);

					info.entryBlock.ReplaceEntryName(info.index,
						nameToUpdate, nameToUpdateLength);
					if (info.index == 0) {
						// we updated at index 0, so we need to update this
						// block's key in the parent block
						updateNextKey = true;
						nameToUpdate = info.entryBlock.NameAt(0,
							nameToUpdateLength);

						// make sure the new block is kept until we no
						// longer use the name in the next iteration
						tempBlockUpdate.TransferFrom(info.block);
					}
				} else {
					ASSERT(newEntryBlock.FreeSpace() >= spaceNeeded);

					// we need to transfer the block to the info, in case we
					// also need to insert a key below
					info.block.TransferFrom(newBlock);
					info.entryBlock.SetTo(
						(checksumfs_dir_entry_block*)info.block.Data(),
						B_PAGE_SIZE);
ASSERT(info.entryBlock.Check());

					info.index -= splitIndex;

					info.entryBlock.ReplaceEntryName(info.index, nameToUpdate,
						nameToUpdateLength);
				}

				// the newly created block needs to be inserted in the
				// parent
				insertNextKey = true;
				nextNameToInsert = newEntryBlock.NameAt(0,
					nextNameToInsertLength);

				// make sure the new block is kept until we no longer use
				// the name in the next iteration (might already have been
				// transferred to entry.block)
				tempBlockUpdateInsert.TransferFrom(newBlock);
			}
		}

		if (insertThisKey) {
			// insert after the block we descended
			if (level < depth)
				info.index++;

			PRINT("  level: %" B_PRId32 ", index: %" B_PRId32 ": inserting key "
				"\"%.*s\" (%" B_PRIuSIZE "), blockIndex: %" B_PRIu64 "\n",
				level, info.index, (int)nameLength, name, nameLength,
				blockIndex);

			if (info.entryBlock.FreeSpace() >= nameLength + 10) {
				info.entryBlock.InsertEntry(info.index, name,
					nameLength, blockIndex);
				if (info.index == 0) {
					// we inserted at index 0, so we need to update this
					// block's key in the parent block
					updateNextKey = true;
					nameToUpdate = info.entryBlock.NameAt(0,
						nameToUpdateLength);

					// make sure the new block is kept until we no longer
					// use the name in the next iteration
					tempBlockUpdate.TransferFrom(info.block);
				}
				continue;
			}

			// Not enough space left in the block -- we need to split it.
			ASSERT(!insertNextKey);

			// for level == 0 we need to clone the block first
			if (level == 0) {
				error = _InsertEntryIncrementDepth(infos, transaction);
				if (error != B_OK)
					RETURN_ERROR(error);

				level = 2;
					// _InsertEntryIncrementDepth() moved the root block
					// content to level 1, where we want to continue.
				updateNextKey = false;
				insertNextKey = true;
				continue;
			}

			int32 splitIndex;
			error = _InsertEntrySplitBlock(level, info, nameLength + 10,
				transaction, newBlock, splitIndex);
			if (error != B_OK)
				RETURN_ERROR(error);

			DirEntryBlock newEntryBlock(
				(checksumfs_dir_entry_block*)newBlock.Data(),
				B_PAGE_SIZE);
ASSERT(newEntryBlock.Check());

			if (info.index < splitIndex) {
				ASSERT(info.entryBlock.FreeSpace() >= nameLength + 10);

				info.entryBlock.InsertEntry(info.index, name,
					nameLength, blockIndex);
				if (info.index == 0) {
					// we inserted at index 0, so we need to update this
					// block's key in the parent block
					updateNextKey = true;
					nameToUpdate = info.entryBlock.NameAt(0,
						nameToUpdateLength);

					// make sure the new block is kept until we no longer
					// use the name in the next iteration
					tempBlockUpdate.TransferFrom(info.block);
				}
			} else {
				ASSERT(newEntryBlock.FreeSpace() >= nameLength + 10);

				info.index -= splitIndex;

				newEntryBlock.InsertEntry(info.index, name, nameLength,
					blockIndex);
			}

			// the newly created block needs to be inserted in the parent
			insertNextKey = true;
			nextNameToInsert = newEntryBlock.NameAt(0, nextNameToInsertLength);
			nextBlockIndexToInsert = newBlock.Index();

			// make sure the new block is kept until we no longer use
			// the name in the next iteration
			tempBlockInsert.TransferFrom(newBlock);
		}
	}

	return B_OK;
}


status_t
DirEntryTree::_InsertEntryIncrementDepth(LevelInfo* infos,
	Transaction& transaction)
{
	FUNCTION("depth: %u -> %u\n", _Depth(), _Depth() + 1);

	if (_Depth() >= kCheckSumFSMaxDirEntryTreeDepth)
		RETURN_ERROR(B_DEVICE_FULL);

	// allocate a new block
	AllocatedBlock allocatedBlock(
		fDirectory->GetVolume()->GetBlockAllocator(), transaction);

	status_t error = allocatedBlock.Allocate(fDirectory->BlockIndex());
	if (error != B_OK)
		RETURN_ERROR(error);
	fDirectory->SetSize(fDirectory->Size() + B_PAGE_SIZE);

	LevelInfo& newInfo = infos[1];
	if (!newInfo.block.GetZero(fDirectory->GetVolume(),
			allocatedBlock.Index(), transaction)) {
		RETURN_ERROR(B_ERROR);
	}

	allocatedBlock.Detach();

	newInfo.entryBlock.SetTo(
		(checksumfs_dir_entry_block*)newInfo.block.Data(), B_PAGE_SIZE);
ASSERT(newInfo.entryBlock.Check());

	// move the old root block contents to the new block
	LevelInfo& rootInfo = infos[0];
	rootInfo.entryBlock.SplitBlock(0, newInfo.entryBlock);

	// add an entry for the new block to the root block
	size_t nameLength;
	const char* name = newInfo.entryBlock.NameAt(0, nameLength);
	rootInfo.entryBlock.InsertEntry(0, name, nameLength,
		newInfo.block.Index());

	PRINT("  -> new block: %" B_PRIu64 "\n", newInfo.block.Index());

	newInfo.index = rootInfo.index;
	rootInfo.index = 0;
	fTree->depth++;

	return B_OK;
}


status_t
DirEntryTree::_InsertEntrySplitBlock(int32 level, LevelInfo& info,
	size_t needed, Transaction& transaction, Block& newBlock,
	int32& _splitIndex)
{
	int32 splitIndex = info.entryBlock.FindSplitIndex(info.index,
		needed);

	FUNCTION("level: %" B_PRId32 ", size needed: %" B_PRIuSIZE ", split index: "
		"%" B_PRId32 "/%" B_PRId32 "\n", level, needed, splitIndex,
		info.entryBlock.EntryCount());

	// allocate a new block
	AllocatedBlock allocatedBlock(
		fDirectory->GetVolume()->GetBlockAllocator(), transaction);

	status_t error = allocatedBlock.Allocate(fDirectory->BlockIndex());
	if (error != B_OK)
		RETURN_ERROR(error);
	fDirectory->SetSize(fDirectory->Size() + B_PAGE_SIZE);

	if (!newBlock.GetZero(fDirectory->GetVolume(), allocatedBlock.Index(),
			transaction)) {
		RETURN_ERROR(B_ERROR);
	}

	allocatedBlock.Detach();

	// split the old block
	DirEntryBlock newEntryBlock(
		(checksumfs_dir_entry_block*)newBlock.Data(), B_PAGE_SIZE);
ASSERT(newEntryBlock.Check());
	info.entryBlock.SplitBlock(splitIndex, newEntryBlock);

	PRINT("  -> new block: %" B_PRIu64 "\n", newBlock.Index());

	_splitIndex = splitIndex;
	return B_OK;
}


bool
DirEntryTree::_Check(int32 level, uint64 blockIndex, const char* key,
	size_t keyLength, DirEntryBlock& entryBlock)
{
	// check block for validity
	if (!entryBlock.Check()) {
		ERROR("DirEntryTree::Check(): level %" B_PRIu32 ": block %"
			B_PRIu64 " not valid!\n", level, blockIndex);
		return false;
	}

	// The root block is allowed to be empty. For all other blocks that is an
	// error.
	uint32 entryCount = entryBlock.EntryCount();
	if (entryCount == 0) {
		if (level == 0)
			return true;

		ERROR("DirEntryTree::Check(): level %" B_PRIu32 ": block %"
			B_PRIu64 " empty!\n", level, blockIndex);
		return false;
	}

	// Verify that the block's first entry matches the key with which the
	// parent block refers to it.
	if (level > 0) {
		size_t nameLength;
		const char* name = entryBlock.NameAt(0, nameLength);
		if (nameLength != keyLength || strncmp(name, key, keyLength) != 0) {
			ERROR("DirEntryTree::Check(): level %" B_PRIu32 ": block %"
				B_PRIu64 " key mismatch: is \"%.*s\", should be \"%.*s\"\n",
				level, blockIndex, (int)keyLength, key, (int)nameLength, name);
			return false;
		}
	}

	if (level == _Depth())
		return true;

	// not the final level -- recurse
	for (uint32 i = 0; i < entryCount; i++) {
		size_t nameLength;
		const char* name = entryBlock.NameAt(i, nameLength);
		uint64 childBlockIndex = entryBlock.BlockIndexAt(i);

		Block childBlock;
		if (!childBlock.GetReadable(fDirectory->GetVolume(), childBlockIndex)) {
			ERROR("DirEntryTree::Check(): level %" B_PRIu32 ": block %"
				B_PRIu64 " failed to get child block %" B_PRIu64 " (entry %"
				B_PRIu32 ")\n", level, blockIndex, childBlockIndex, i);
		}

		DirEntryBlock childEntryBlock(
			(checksumfs_dir_entry_block*)childBlock.Data(), B_PAGE_SIZE);

		if (!_Check(level + 1, childBlockIndex, name, nameLength,
				childEntryBlock)) {
			return false;
		}
	}

	return true;
}


// #pragma mark - Directory


Directory::Directory(Volume* volume, uint64 blockIndex,
	const checksumfs_node& nodeData)
	:
	Node(volume, blockIndex, nodeData)
{
}


Directory::Directory(Volume* volume, mode_t mode)
	:
	Node(volume, mode)
{
}


Directory::~Directory()
{
}


void
Directory::DeletingNode()
{
	Node::DeletingNode();

	// iterate through the directory and remove references to all entries' nodes
	char* name = (char*)malloc(kCheckSumFSNameLength + 1);
	if (name != NULL) {
		name[0] = '\0';

		DirEntryTree entryTree(this);
		size_t nameLength;
		uint64 blockIndex;
		while (entryTree.LookupNextEntry(name, name, nameLength,
				blockIndex) == B_OK) {
			Node* node;
			if (GetVolume()->GetNode(blockIndex, node) == B_OK) {
				Transaction transaction(GetVolume());
				if (transaction.StartAndAddNode(node) == B_OK) {
					node->SetHardLinks(node->HardLinks() - 1);
					if (node->HardLinks() == 0)
						GetVolume()->RemoveNode(node);

					if (transaction.Commit() != B_OK) {
						ERROR("Failed to commit transaction for dereferencing "
							"entry node of deleted directory at %" B_PRIu64
							"\n", BlockIndex());
					}
				} else {
					ERROR("Failed to start transaction for dereferencing "
						"entry node of deleted directory at %" B_PRIu64 "\n",
						BlockIndex());
				}

				GetVolume()->PutNode(node);
			} else {
				ERROR("Failed to get node %" B_PRIu64 " referenced by deleted "
					"directory at %" B_PRIu64 "\n", blockIndex, BlockIndex());
			}
		}

		free(name);
	}

	// free the directory entry block tree
	Transaction transaction(GetVolume());
	if (transaction.Start() != B_OK) {
		ERROR("Failed to start transaction for freeing entry tree of deleted "
			"directory at %" B_PRIu64 "\n", BlockIndex());
		return;
	}

	DirEntryTree entryTree(this);
	if (entryTree.FreeTree(transaction) != B_OK) {
		ERROR("Failed to freeing entry tree of deleted directory at %" B_PRIu64
			"\n", BlockIndex());
		return;
	}

	if (transaction.Commit() != B_OK) {
		ERROR("Failed to commit transaction for freeing entry tree of deleted "
			"directory at %" B_PRIu64 "\n", BlockIndex());
	}
}


status_t
Directory::LookupEntry(const char* name, uint64& _blockIndex)
{
	DirEntryTree entryTree(this);
	return entryTree.LookupEntry(name, _blockIndex);
}


status_t
Directory::LookupNextEntry(const char* name, char* foundName,
	size_t& _foundNameLength, uint64& _blockIndex)
{
	DirEntryTree entryTree(this);
	return entryTree.LookupNextEntry(name, foundName, _foundNameLength,
		_blockIndex);
}


status_t
Directory::InsertEntry(const char* name, uint64 blockIndex,
	Transaction& transaction)
{
	DirEntryTree entryTree(this);

	status_t error = entryTree.InsertEntry(name, blockIndex, transaction);
	if (error == B_OK)
		ASSERT(entryTree.Check());
	return error;
}


status_t
Directory::RemoveEntry(const char* name, Transaction& transaction,
	bool* _lastEntryRemoved)
{
	DirEntryTree entryTree(this);

	status_t error = entryTree.RemoveEntry(name, transaction);
	if (error != B_OK)
		return error;

	ASSERT(entryTree.Check());

	if (_lastEntryRemoved != NULL)
		*_lastEntryRemoved = entryTree.IsEmpty();

	return B_OK;
}
