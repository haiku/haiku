/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "Journal.h"

#include <new>
#include <string.h>
#include <unistd.h>

#include <fs_cache.h>

#include "CachedBlock.h"
#include "CRCTable.h"
#include "HashRevokeManager.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mext2:\33[0m " x)
#define WARN(x...) dprintf("\33[34mext2:\33[0m " x)


class LogEntry : public DoublyLinkedListLinkImpl<LogEntry> {
public:
							LogEntry(Journal* journal, uint32 logStart,
								uint32 length);
							~LogEntry();

			uint32			Start() const { return fStart; }
			uint32			CommitID() const { return fCommitID; }

			Journal*		GetJournal() { return fJournal; }

private:
			Journal*		fJournal;
			uint32			fStart;
			uint32			fCommitID;
};


LogEntry::LogEntry(Journal* journal, uint32 logStart, uint32 commitID)
	:
	fJournal(journal),
	fStart(logStart),
	fCommitID(commitID)
{
}


LogEntry::~LogEntry()
{
}


void
JournalHeader::MakeDescriptor(uint32 sequence)
{
	this->magic = B_HOST_TO_BENDIAN_INT32(JOURNAL_MAGIC);
	this->sequence = B_HOST_TO_BENDIAN_INT32(sequence);
	this->block_type = B_HOST_TO_BENDIAN_INT32(JOURNAL_DESCRIPTOR_BLOCK);
}


void
JournalHeader::MakeCommit(uint32 sequence)
{
	this->magic = B_HOST_TO_BENDIAN_INT32(JOURNAL_MAGIC);
	this->sequence = B_HOST_TO_BENDIAN_INT32(sequence);
	this->block_type = B_HOST_TO_BENDIAN_INT32(JOURNAL_COMMIT_BLOCK);
}


Journal::Journal(Volume* fsVolume, Volume* jVolume)
	:
	fJournalVolume(jVolume),
	fJournalBlockCache(jVolume->BlockCache()),
	fFilesystemVolume(fsVolume),
	fFilesystemBlockCache(fsVolume->BlockCache()),
	fRevokeManager(NULL),
	fInitStatus(B_OK),
	fBlockSize(sizeof(JournalSuperBlock)),
	fFirstCommitID(0),
	fFirstCacheCommitID(0),
	fFirstLogBlock(1),
	fLogSize(0),
	fVersion(0),
	fLogStart(0),
	fLogEnd(0),
	fFreeBlocks(0),
	fMaxTransactionSize(0),
	fCurrentCommitID(0),
	fHasSubTransaction(false),
	fSeparateSubTransactions(false),
	fUnwrittenTransactions(0),
	fTransactionID(0)
{
	recursive_lock_init(&fLock, "ext2 journal");
	mutex_init(&fLogEntriesLock, "ext2 journal log entries");

	HashRevokeManager* revokeManager = new(std::nothrow) HashRevokeManager(
		fsVolume->Has64bitFeature());
	TRACE("Journal::Journal(): Allocated a hash revoke manager at %p\n",
		revokeManager);

	if (revokeManager == NULL)
		fInitStatus = B_NO_MEMORY;
	else {
		fInitStatus = revokeManager->Init();

		if (fInitStatus == B_OK) {
			fRevokeManager = revokeManager;
			fInitStatus = _LoadSuperBlock();
		} else
			delete revokeManager;
	}
}


Journal::Journal()
	:
	fJournalVolume(NULL),
	fJournalBlockCache(NULL),
	fFilesystemVolume(NULL),
	fFilesystemBlockCache(NULL),
	fOwner(NULL),
	fRevokeManager(NULL),
	fInitStatus(B_OK),
	fBlockSize(sizeof(JournalSuperBlock)),
	fFirstCommitID(0),
	fFirstCacheCommitID(0),
	fFirstLogBlock(1),
	fLogSize(0),
	fVersion(0),
	fIsStarted(false),
	fLogStart(0),
	fLogEnd(0),
	fFreeBlocks(0),
	fMaxTransactionSize(0),
	fCurrentCommitID(0),
	fHasSubTransaction(false),
	fSeparateSubTransactions(false),
	fUnwrittenTransactions(0),
	fTransactionID(0),
	fChecksumEnabled(false),
	fChecksumV3Enabled(false),
	fFeature64bits(false)
{
	recursive_lock_init(&fLock, "ext2 journal");
	mutex_init(&fLogEntriesLock, "ext2 journal log entries");
}


Journal::~Journal()
{
	TRACE("Journal destructor.\n");

	TRACE("Journal::~Journal(): Attempting to delete revoke manager at %p\n",
		fRevokeManager);
	delete fRevokeManager;

	recursive_lock_destroy(&fLock);
	mutex_destroy(&fLogEntriesLock);
}


status_t
Journal::InitCheck()
{
	return fInitStatus;
}


status_t
Journal::Uninit()
{
	if (!fIsStarted)
		return B_OK;

	status_t status = FlushLogAndBlocks();

	if (status == B_OK) {
		// Mark journal as clean
		fLogStart = 0;
		status = _SaveSuperBlock();
	}

	fIsStarted = false;

	return status;
}


/*virtual*/ status_t
Journal::StartLog()
{
	fLogStart = fFirstLogBlock;
	fLogEnd = fFirstLogBlock;
	fFreeBlocks = 0;
	fIsStarted = true;

	fCurrentCommitID = fFirstCommitID;

	return _SaveSuperBlock();
}


status_t
Journal::RestartLog()
{
	fFirstCommitID = 1;

	return B_OK;
}


/*virtual*/ status_t
Journal::Lock(Transaction* owner, bool separateSubTransactions)
{
	TRACE("Journal::Lock()\n");
	status_t status = recursive_lock_lock(&fLock);
	if (status != B_OK)
		return status;

	TRACE("Journal::Lock(): Aquired lock\n");

	if (!fSeparateSubTransactions && recursive_lock_get_recursion(&fLock) > 1) {
		// reuse current transaction
		TRACE("Journal::Lock(): Reusing current transaction\n");
		return B_OK;
	}

	if (separateSubTransactions)
		fSeparateSubTransactions = true;

	if (owner != NULL)
		owner->SetParent(fOwner);

	fOwner = owner;

	if (fOwner != NULL) {
		if (fUnwrittenTransactions > 0) {
			// start a sub transaction
			TRACE("Journal::Lock(): Starting sub transaction\n");
			cache_start_sub_transaction(fFilesystemBlockCache, fTransactionID);
			fHasSubTransaction = true;
		} else {
			TRACE("Journal::Lock(): Starting new transaction\n");
			fTransactionID = cache_start_transaction(fFilesystemBlockCache);
		}

		if (fTransactionID < B_OK) {
			recursive_lock_unlock(&fLock);
			return fTransactionID;
		}

		cache_add_transaction_listener(fFilesystemBlockCache, fTransactionID,
			TRANSACTION_IDLE, _TransactionIdle, this);
	}

	return B_OK;
}


/*virtual*/ status_t
Journal::Unlock(Transaction* owner, bool success)
{
	TRACE("Journal::Unlock(): Lock recursion: %" B_PRId32 "\n",
		recursive_lock_get_recursion(&fLock));
	if (fSeparateSubTransactions
		|| recursive_lock_get_recursion(&fLock) == 1) {
		// we only end the transaction if we unlock it
		if (owner != NULL) {
			TRACE("Journal::Unlock(): Calling _TransactionDone\n");
			status_t status = _TransactionDone(success);
			if (status != B_OK)
				return status;

			TRACE("Journal::Unlock(): Returned from _TransactionDone\n");
			bool separateSubTransactions = fSeparateSubTransactions;
			fSeparateSubTransactions = true;
			TRACE("Journal::Unlock(): Notifying listeners for: %p\n", owner);
			owner->NotifyListeners(success);
			TRACE("Journal::Unlock(): Done notifying listeners\n");
			fSeparateSubTransactions = separateSubTransactions;

			fOwner = owner->Parent();
		} else
			fOwner = NULL;

		if (fSeparateSubTransactions
			&& recursive_lock_get_recursion(&fLock) == 1)
			fSeparateSubTransactions = false;
	} else
		owner->MoveListenersTo(fOwner);

	TRACE("Journal::Unlock(): Unlocking the lock\n");

	recursive_lock_unlock(&fLock);
	return B_OK;
}


status_t
Journal::MapBlock(off_t logical, fsblock_t& physical)
{
	TRACE("Journal::MapBlock()\n");
	physical = logical;

	return B_OK;
}


inline uint32
Journal::FreeLogBlocks() const
{
	TRACE("Journal::FreeLogBlocks(): start: %" B_PRIu32 ", end: %" B_PRIu32
		", size: %" B_PRIu32 "\n", fLogStart, fLogEnd, fLogSize);
	return fLogStart <= fLogEnd
		? fLogSize - fLogEnd + fLogStart - 1
		: fLogStart - fLogEnd;
}


status_t
Journal::FlushLogAndBlocks()
{
	return _FlushLog(true, true);
}


int32
Journal::TransactionID() const
{
	return fTransactionID;
}


status_t
Journal::_WritePartialTransactionToLog(JournalHeader* descriptorBlock,
	bool detached, uint8** _escapedData, uint32 &logBlock, off_t& blockNumber,
	long& cookie, ArrayDeleter<uint8>& escapedDataDeleter, uint32& blockCount,
	bool& finished)
{
	TRACE("Journal::_WritePartialTransactionToLog()\n");

	uint32 descriptorBlockPos = logBlock;
	uint8* escapedData = *_escapedData;

	JournalBlockTag* tag = (JournalBlockTag*)descriptorBlock->data;
	JournalBlockTag* lastTag = (JournalBlockTag*)((uint8*)descriptorBlock
		+ fBlockSize - sizeof(JournalHeader));

	finished = false;
	status_t status = B_OK;

	while (tag < lastTag && status == B_OK) {
		tag->SetBlockNumber(blockNumber);
		tag->SetFlags(0);

		CachedBlock data(fFilesystemVolume);
		const JournalHeader* blockData = (JournalHeader*)data.SetTo(
			blockNumber);
		if (blockData == NULL) {
			panic("Got a NULL pointer while iterating through transaction "
				"blocks.\n");
			return B_ERROR;
		}

		void* finalData;

		if (blockData->CheckMagic()) {
			// The journaled block starts with the magic value
			// We must remove it to prevent confusion
			TRACE("Journal::_WritePartialTransactionToLog(): Block starts with "
				"magic number. Escaping it\n");
			tag->SetEscapedFlag();

			if (escapedData == NULL) {
				TRACE("Journal::_WritePartialTransactionToLog(): Allocating "
					"space for escaped block (%" B_PRIu32 ")\n", fBlockSize);
				escapedData = new(std::nothrow) uint8[fBlockSize];
				if (escapedData == NULL) {
					TRACE("Journal::_WritePartialTransactionToLof(): Failed to "
						"allocate buffer for escaped data block\n");
					return B_NO_MEMORY;
				}
				escapedDataDeleter.SetTo(escapedData);
				*_escapedData = escapedData;

				((int32*)escapedData)[0] = 0; // Remove magic
			}

			memcpy(escapedData + 4, blockData->data, fBlockSize - 4);
			finalData = escapedData;
		} else
			finalData = (void*)blockData;

		// TODO: use iovecs?

		logBlock = _WrapAroundLog(logBlock + 1);

		fsblock_t physicalBlock;
		status = MapBlock(logBlock, physicalBlock);
		if (status != B_OK)
			return status;

		off_t logOffset = physicalBlock * fBlockSize;

		TRACE("Journal::_WritePartialTransactionToLog(): Writing from memory: "
			"%p, to disk: %" B_PRIdOFF "\n", finalData, logOffset);
		size_t written = write_pos(fJournalVolume->Device(), logOffset,
			finalData, fBlockSize);
		if (written != fBlockSize) {
			TRACE("Failed to write journal block.\n");
			return B_IO_ERROR;
		}

		TRACE("Journal::_WritePartialTransactionToLog(): Wrote a journal block "
			"at: %" B_PRIu32 "\n", logBlock);

		blockCount++;
		tag++;

		status = cache_next_block_in_transaction(fFilesystemBlockCache,
			fTransactionID, detached, &cookie, &blockNumber, NULL, NULL);
	}

	finished = status != B_OK;

	// Write descriptor block
	--tag;
	tag->SetLastTagFlag();

	fsblock_t physicalBlock;
	status = MapBlock(descriptorBlockPos, physicalBlock);
	if (status != B_OK)
		return status;

	off_t descriptorBlockOffset = physicalBlock * fBlockSize;

	TRACE("Journal::_WritePartialTransactionToLog(): Writing to: %" B_PRIdOFF
		"\n", descriptorBlockOffset);
	size_t written = write_pos(fJournalVolume->Device(),
		descriptorBlockOffset, descriptorBlock, fBlockSize);
	if (written != fBlockSize) {
		TRACE("Failed to write journal descriptor block.\n");
		return B_IO_ERROR;
	}

	blockCount++;
	logBlock = _WrapAroundLog(logBlock + 1);

	return B_OK;
}


status_t
Journal::_WriteTransactionToLog()
{
	TRACE("Journal::_WriteTransactionToLog()\n");
	// Transaction enters the Flush state
	bool detached = false;
	TRACE("Journal::_WriteTransactionToLog(): Attempting to get transaction "
		"size\n");
	size_t size = _FullTransactionSize();
	TRACE("Journal::_WriteTransactionToLog(): transaction size: %" B_PRIuSIZE
		"\n", size);

	if (size > fMaxTransactionSize) {
		TRACE("Journal::_WriteTransactionToLog(): not enough free space "
			"for the transaction. Attempting to free some space.\n");
		size = _MainTransactionSize();
		TRACE("Journal::_WriteTransactionToLog(): main transaction size: %"
			B_PRIuSIZE "\n", size);

		if (fHasSubTransaction && size < fMaxTransactionSize) {
			TRACE("Journal::_WriteTransactionToLog(): transaction doesn't fit, "
				"but it can be separated\n");
			detached = true;
		} else {
			// Error: transaction can't fit in log
			panic("transaction too large (size: %" B_PRIuSIZE ", max size: %"
				B_PRIu32 ", log size: %" B_PRIu32 ")\n", size,
				fMaxTransactionSize, fLogSize);
			return B_BUFFER_OVERFLOW;
		}
	}

	TRACE("Journal::_WriteTransactionToLog(): free log blocks: %" B_PRIu32
		"\n", FreeLogBlocks());
	if (size > FreeLogBlocks()) {
		TRACE("Journal::_WriteTransactionToLog(): Syncing block cache\n");
		cache_sync_transaction(fFilesystemBlockCache, fTransactionID);

		if (size > FreeLogBlocks()) {
			panic("Transaction fits, but sync didn't result in enough"
				"free space.\n\tGot %" B_PRIu32 " when at least %" B_PRIuSIZE
				" was expected.", FreeLogBlocks(), size);
		}
	}

	TRACE("Journal::_WriteTransactionToLog(): finished managing space for "
		"the transaction\n");

	fHasSubTransaction = false;
	if (!fIsStarted)
		StartLog();

	// Prepare Descriptor block
	TRACE("Journal::_WriteTransactionToLog(): attempting to allocate space for "
		"the descriptor block, block size %" B_PRIu32 "\n", fBlockSize);
	JournalHeader* descriptorBlock =
		(JournalHeader*)new(std::nothrow) uint8[fBlockSize];
	if (descriptorBlock == NULL) {
		TRACE("Journal::_WriteTransactionToLog(): Failed to allocate a buffer "
			"for the descriptor block\n");
		return B_NO_MEMORY;
	}
	ArrayDeleter<uint8> descriptorBlockDeleter((uint8*)descriptorBlock);

	descriptorBlock->MakeDescriptor(fCurrentCommitID);

	// Prepare Commit block
	TRACE("Journal::_WriteTransactionToLog(): attempting to allocate space for "
		"the commit block, block size %" B_PRIu32 "\n", fBlockSize);
	JournalHeader* commitBlock =
		(JournalHeader*)new(std::nothrow) uint8[fBlockSize];
	if (commitBlock == NULL) {
		TRACE("Journal::_WriteTransactionToLog(): Failed to allocate a buffer "
			"for the commit block\n");
		return B_NO_MEMORY;
	}
	ArrayDeleter<uint8> commitBlockDeleter((uint8*)commitBlock);

	commitBlock->MakeCommit(fCurrentCommitID + 1);
	memset(commitBlock->data, 0, fBlockSize - sizeof(JournalHeader));
		// TODO: This probably isn't necessary

	uint8* escapedData = NULL;
	ArrayDeleter<uint8> escapedDataDeleter;

	off_t blockNumber;
	long cookie = 0;

	status_t status = cache_next_block_in_transaction(fFilesystemBlockCache,
		fTransactionID, detached, &cookie, &blockNumber, NULL, NULL);
	if (status != B_OK) {
		TRACE("Journal::_WriteTransactionToLog(): Transaction has no blocks to "
			"write\n");
		return B_OK;
	}

	uint32 blockCount = 0;

	uint32 logBlock = _WrapAroundLog(fLogEnd);

	bool finished = false;

	status = _WritePartialTransactionToLog(descriptorBlock, detached,
		&escapedData, logBlock, blockNumber, cookie, escapedDataDeleter,
		blockCount, finished);
	if (!finished && status != B_OK)
		return status;

	uint32 commitBlockPos = logBlock;

	while (!finished) {
		descriptorBlock->IncrementSequence();

		status = _WritePartialTransactionToLog(descriptorBlock, detached,
			&escapedData, logBlock, blockNumber, cookie, escapedDataDeleter,
			blockCount, finished);
		if (!finished && status != B_OK)
			return status;

		// It is okay to write the commit blocks of the partial transactions
		// as long as the commit block of the first partial transaction isn't
		// written. When it recovery reaches where the first commit should be
		// and doesn't find it, it considers it found the end of the log.

		fsblock_t physicalBlock;
		status = MapBlock(logBlock, physicalBlock);
		if (status != B_OK)
			return status;

		off_t logOffset = physicalBlock * fBlockSize;

		TRACE("Journal::_WriteTransactionToLog(): Writting commit block to "
			"%" B_PRIdOFF "\n", logOffset);
		off_t written = write_pos(fJournalVolume->Device(), logOffset,
			commitBlock, fBlockSize);
		if (written != fBlockSize) {
			TRACE("Failed to write journal commit block.\n");
			return B_IO_ERROR;
		}

		commitBlock->IncrementSequence();
		blockCount++;

		logBlock = _WrapAroundLog(logBlock + 1);
	}

	// Transaction will enter the Commit state
	fsblock_t physicalBlock;
	status = MapBlock(commitBlockPos, physicalBlock);
	if (status != B_OK)
		return status;

	off_t logOffset = physicalBlock * fBlockSize;

	TRACE("Journal::_WriteTransactionToLog(): Writing to: %" B_PRIdOFF "\n",
		logOffset);
	off_t written = write_pos(fJournalVolume->Device(), logOffset, commitBlock,
		fBlockSize);
	if (written != fBlockSize) {
		TRACE("Failed to write journal commit block.\n");
		return B_IO_ERROR;
	}

	blockCount++;
	fLogEnd = _WrapAroundLog(fLogEnd + blockCount);

	status = _SaveSuperBlock();

	// Transaction will enter Finished state
	LogEntry *logEntry = new LogEntry(this, fLogEnd, fCurrentCommitID++);
	TRACE("Journal::_WriteTransactionToLog(): Allocating log entry at %p\n",
		logEntry);
	if (logEntry == NULL) {
		panic("no memory to allocate log entries!");
		return B_NO_MEMORY;
	}

	mutex_lock(&fLogEntriesLock);
	fLogEntries.Add(logEntry);
	mutex_unlock(&fLogEntriesLock);

	if (detached) {
		fTransactionID = cache_detach_sub_transaction(fFilesystemBlockCache,
			fTransactionID, _TransactionWritten, logEntry);
		fUnwrittenTransactions = 1;

		if (status == B_OK && _FullTransactionSize() > fLogSize) {
			// If the transaction is too large after writing, there is no way to
			// recover, so let this transaction fail.
			ERROR("transaction too large (%" B_PRIuSIZE " blocks, log size %"
				B_PRIu32 ")!\n", _FullTransactionSize(), fLogSize);
			return B_BUFFER_OVERFLOW;
		}
	} else {
		cache_end_transaction(fFilesystemBlockCache, fTransactionID,
			_TransactionWritten, logEntry);
		fUnwrittenTransactions = 0;
	}

	return B_OK;
}


status_t
Journal::_SaveSuperBlock()
{
	TRACE("Journal::_SaveSuperBlock()\n");
	fsblock_t physicalBlock;
	status_t status = MapBlock(0, physicalBlock);
	if (status != B_OK)
		return status;

	off_t superblockPos = physicalBlock * fBlockSize;

	JournalSuperBlock superblock;
	size_t bytesRead = read_pos(fJournalVolume->Device(), superblockPos,
		&superblock, sizeof(superblock));

	if (bytesRead != sizeof(superblock))
		return B_IO_ERROR;

	superblock.SetFirstCommitID(fFirstCommitID);
	superblock.SetLogStart(fLogStart);

	if (fChecksumEnabled)
		superblock.SetChecksum(_Checksum(&superblock));

	TRACE("Journal::SaveSuperBlock(): Write to %" B_PRIdOFF "\n",
		superblockPos);
	size_t bytesWritten = write_pos(fJournalVolume->Device(), superblockPos,
		&superblock, sizeof(superblock));

	if (bytesWritten != sizeof(superblock))
		return B_IO_ERROR;

	TRACE("Journal::_SaveSuperBlock(): Done\n");

	return B_OK;
}


status_t
Journal::_LoadSuperBlock()
{
	STATIC_ASSERT(sizeof(struct JournalHeader) == 12);
	STATIC_ASSERT(sizeof(struct JournalSuperBlock) == 1024);

	TRACE("Journal::_LoadSuperBlock()\n");
	fsblock_t superblockPos;

	status_t status = MapBlock(0, superblockPos);
	if (status != B_OK)
		return status;

	TRACE("Journal::_LoadSuperBlock(): superblock physical block: %" B_PRIu64
		"\n", superblockPos);

	JournalSuperBlock superblock;
	size_t bytesRead = read_pos(fJournalVolume->Device(), superblockPos
		* fJournalVolume->BlockSize(), &superblock, sizeof(superblock));

	if (bytesRead != sizeof(superblock)) {
		ERROR("Journal::_LoadSuperBlock(): failed to read superblock\n");
		return B_IO_ERROR;
	}

	if (!superblock.header.CheckMagic()) {
		ERROR("Journal::_LoadSuperBlock(): Invalid superblock magic %" B_PRIx32
			"\n", superblock.header.Magic());
		return B_BAD_VALUE;
	}

	if (superblock.header.BlockType() == JOURNAL_SUPERBLOCK_V1) {
		TRACE("Journal::_LoadSuperBlock(): Journal superblock version 1\n");
		fVersion = 1;
	} else if (superblock.header.BlockType() == JOURNAL_SUPERBLOCK_V2) {
		TRACE("Journal::_LoadSuperBlock(): Journal superblock version 2\n");
		fVersion = 2;
	} else {
		ERROR("Journal::_LoadSuperBlock(): Invalid superblock version\n");
		return B_BAD_VALUE;
	}

	if (fVersion >= 2) {
		TRACE("Journal::_LoadSuperBlock(): incompatible features %" B_PRIx32
			", read-only features %" B_PRIx32 "\n",
			superblock.IncompatibleFeatures(),
			superblock.ReadOnlyCompatibleFeatures());

		status = _CheckFeatures(&superblock);

		if (status != B_OK)
			return status;

		if (fChecksumEnabled) {
			if (superblock.Checksum() != _Checksum(&superblock)) {
				ERROR("Journal::_LoadSuperBlock(): Invalid checksum\n");
				return B_BAD_DATA;
			}
			fChecksumSeed = calculate_crc32c(0xffffffff, (uint8*)superblock.uuid,
				sizeof(superblock.uuid));
		}
	}

	fBlockSize = superblock.BlockSize();
	fFirstCommitID = superblock.FirstCommitID();
	fFirstLogBlock = superblock.FirstLogBlock();
	fLogStart = superblock.LogStart();
	fLogSize = superblock.NumBlocks();

	uint32 descriptorTags = (fBlockSize - sizeof(JournalHeader))
		/ sizeof(JournalBlockTag);
		// Maximum tags per descriptor block
	uint32 maxDescriptors = (fLogSize - 1) / (descriptorTags + 2);
		// Maximum number of full journal transactions
	fMaxTransactionSize = maxDescriptors * descriptorTags;
	fMaxTransactionSize += (fLogSize - 1) - fMaxTransactionSize - 2;
		// Maximum size of a "logical" transaction
		// TODO: Why is "superblock.MaxTransactionBlocks();" zero?
	//fFirstCacheCommitID = fFirstCommitID - fTransactionID /*+ 1*/;

	TRACE("Journal::_LoadSuperBlock(): block size: %" B_PRIu32 ", first commit"
		" id: %" B_PRIu32 ", first log block: %" B_PRIu32 ", log start: %"
		B_PRIu32 ", log size: %" B_PRIu32 ", max transaction size: %" B_PRIu32
		"\n", fBlockSize, fFirstCommitID, fFirstLogBlock, fLogStart,
		fLogSize, fMaxTransactionSize);

	return B_OK;
}


status_t
Journal::_CheckFeatures(JournalSuperBlock* superblock)
{
	uint32 readonly = superblock->ReadOnlyCompatibleFeatures();
	uint32 incompatible = superblock->IncompatibleFeatures();
	bool hasReadonly = (readonly & ~JOURNAL_KNOWN_READ_ONLY_COMPATIBLE_FEATURES)
		!= 0;
	bool hasIncompatible = (incompatible
		& ~JOURNAL_KNOWN_INCOMPATIBLE_FEATURES) != 0;
	if (hasReadonly || hasIncompatible ) {
		ERROR("Journal::_CheckFeatures(): Unsupported features: %" B_PRIx32
			" %" B_PRIx32 "\n", readonly, incompatible);
		return B_UNSUPPORTED;
	}

	bool hasCsumV2 =
		(superblock->IncompatibleFeatures() & JOURNAL_FEATURE_INCOMPATIBLE_CSUM_V2) != 0;
	bool hasCsumV3 =
		(superblock->IncompatibleFeatures() & JOURNAL_FEATURE_INCOMPATIBLE_CSUM_V3) != 0;
	if (hasCsumV2 && hasCsumV3) {
		return B_BAD_VALUE;
	}

	fChecksumEnabled = hasCsumV2 && hasCsumV3;
	fChecksumV3Enabled = hasCsumV3;
	fFeature64bits =
		(superblock->IncompatibleFeatures() & JOURNAL_FEATURE_INCOMPATIBLE_64BIT) != 0;
	return B_OK;
}


uint32
Journal::_Checksum(JournalSuperBlock* superblock)
{
	uint32 oldChecksum = superblock->checksum;
	superblock->checksum = 0;
	uint32 checksum = calculate_crc32c(0xffffffff, (uint8*)superblock,
		sizeof(JournalSuperBlock));
	superblock->checksum = oldChecksum;
	return checksum;
}


bool
Journal::_Checksum(uint8* block, bool set)
{
	JournalBlockTail *tail = (JournalBlockTail*)(block + fBlockSize
		- sizeof(JournalBlockTail));
	uint32 oldChecksum = tail->checksum;
	tail->checksum = 0;
	uint32 checksum = calculate_crc32c(0xffffffff, block, fBlockSize);
	if (set) {
		tail->checksum = checksum;
	} else {
		tail->checksum = oldChecksum;
	}
	return checksum == oldChecksum;
}


uint32
Journal::_CountTags(JournalHeader* descriptorBlock)
{
	uint32 count = 0;
	size_t tagSize = _TagSize();
	size_t size = fBlockSize;

	if (fChecksumEnabled)
		size -= sizeof(JournalBlockTail);

	JournalBlockTag* tags = (JournalBlockTag*)descriptorBlock->data;
		// Skip the header
	JournalBlockTag* lastTag = (JournalBlockTag*)
		(descriptorBlock + size - tagSize);

	while (tags < lastTag && (tags->Flags() & JOURNAL_FLAG_LAST_TAG) == 0) {
		if ((tags->Flags() & JOURNAL_FLAG_SAME_UUID) == 0)
			tags = (JournalBlockTag*)((uint8*)tags + 16); // Skip new UUID

		TRACE("Journal::_CountTags(): Tag block: %" B_PRIu32 "\n",
			tags->BlockNumber());

		tags = (JournalBlockTag*)((uint8*)tags + tagSize); // Go to next tag
		count++;
	}

	if ((tags->Flags() & JOURNAL_FLAG_LAST_TAG) != 0)
		count++;

	TRACE("Journal::_CountTags(): counted tags: %" B_PRIu32 "\n", count);

	return count;
}


size_t
Journal::_TagSize()
{
	if (fChecksumV3Enabled)
		return sizeof(JournalBlockTagV3);

	size_t size = sizeof(JournalBlockTag);
	if (fChecksumEnabled)
		size += sizeof(uint16);
	if (!fFeature64bits)
		size -= sizeof(uint32);
	return size;
}


/*virtual*/ status_t
Journal::Recover()
{
	TRACE("Journal::Recover()\n");
	if (fLogStart == 0) // Journal was cleanly unmounted
		return B_OK;

	TRACE("Journal::Recover(): Journal needs recovery\n");

	uint32 lastCommitID;

	status_t status = _RecoverPassScan(lastCommitID);
	if (status != B_OK)
		return status;

	status = _RecoverPassRevoke(lastCommitID);
	if (status != B_OK)
		return status;

	return _RecoverPassReplay(lastCommitID);
}


// First pass: Find the end of the log
status_t
Journal::_RecoverPassScan(uint32& lastCommitID)
{
	TRACE("Journal Recover: 1st Pass: Scan\n");

	CachedBlock cached(fJournalVolume);
	JournalHeader* header;
	uint32 nextCommitID = fFirstCommitID;
	uint32 nextBlock = fLogStart;
	fsblock_t nextBlockPos;

	status_t status = MapBlock(nextBlock, nextBlockPos);
	if (status != B_OK)
		return status;

	header = (JournalHeader*)cached.SetTo(nextBlockPos);

	while (header->CheckMagic() && header->Sequence() == nextCommitID) {
		uint32 blockType = header->BlockType();

		if (blockType == JOURNAL_DESCRIPTOR_BLOCK) {
			if (fChecksumEnabled && !_Checksum((uint8*)header, false)) {
				ERROR("Journal::_RecoverPassScan(): Invalid checksum\n");
				return B_BAD_DATA;
			}
			uint32 tags = _CountTags(header);
			nextBlock += tags;
			TRACE("Journal recover pass scan: Found a descriptor block with "
				"%" B_PRIu32 " tags\n", tags);
		} else if (blockType == JOURNAL_COMMIT_BLOCK) {
			nextCommitID++;
			TRACE("Journal recover pass scan: Found a commit block. Next "
				"commit ID: %" B_PRIu32 "\n", nextCommitID);
		} else if (blockType != JOURNAL_REVOKE_BLOCK) {
			TRACE("Journal recover pass scan: Reached an unrecognized block, "
				"assuming as log's end.\n");
			break;
		} else {
			TRACE("Journal recover pass scan: Found a revoke block, "
				"skipping it\n");
		}

		nextBlock = _WrapAroundLog(nextBlock + 1);

		status = MapBlock(nextBlock, nextBlockPos);
		if (status != B_OK)
			return status;

		header = (JournalHeader*)cached.SetTo(nextBlockPos);
	}

	TRACE("Journal Recovery pass scan: Last detected transaction ID: %"
		B_PRIu32 "\n", nextCommitID);

	lastCommitID = nextCommitID;
	return B_OK;
}


// Second pass: Collect all revoked blocks
status_t
Journal::_RecoverPassRevoke(uint32 lastCommitID)
{
	TRACE("Journal Recover: 2nd Pass: Revoke\n");

	CachedBlock cached(fJournalVolume);
	JournalHeader* header;
	uint32 nextCommitID = fFirstCommitID;
	uint32 nextBlock = fLogStart;
	fsblock_t nextBlockPos;

	status_t status = MapBlock(nextBlock, nextBlockPos);
	if (status != B_OK)
		return status;

	header = (JournalHeader*)cached.SetTo(nextBlockPos);

	while (nextCommitID < lastCommitID) {
		if (!header->CheckMagic() || header->Sequence() != nextCommitID) {
			// Somehow the log is different than the expexted
			return B_ERROR;
		}

		uint32 blockType = header->BlockType();

		if (blockType == JOURNAL_DESCRIPTOR_BLOCK)
			nextBlock += _CountTags(header);
		else if (blockType == JOURNAL_COMMIT_BLOCK)
			nextCommitID++;
		else if (blockType == JOURNAL_REVOKE_BLOCK) {
			TRACE("Journal::_RecoverPassRevoke(): Found a revoke block\n");
			status = fRevokeManager->ScanRevokeBlock(
				(JournalRevokeHeader*)header, nextCommitID);

			if (status != B_OK)
				return status;
		} else {
			WARN("Journal::_RecoverPassRevoke(): Found an unrecognized block\n");
			break;
		}

		nextBlock = _WrapAroundLog(nextBlock + 1);

		status = MapBlock(nextBlock, nextBlockPos);
		if (status != B_OK)
			return status;

		header = (JournalHeader*)cached.SetTo(nextBlockPos);
	}

	if (nextCommitID != lastCommitID) {
		// Possibly because of some sort of IO error
		TRACE("Journal::_RecoverPassRevoke(): Incompatible commit IDs\n");
		return B_ERROR;
	}

	TRACE("Journal recovery pass revoke: Revoked blocks: %" B_PRIu32 "\n",
		fRevokeManager->NumRevokes());

	return B_OK;
}


// Third pass: Replay log
status_t
Journal::_RecoverPassReplay(uint32 lastCommitID)
{
	TRACE("Journal Recover: 3rd Pass: Replay\n");

	uint32 nextCommitID = fFirstCommitID;
	uint32 nextBlock = fLogStart;
	fsblock_t nextBlockPos;

	status_t status = MapBlock(nextBlock, nextBlockPos);
	if (status != B_OK)
		return status;

	CachedBlock cached(fJournalVolume);
	JournalHeader* header = (JournalHeader*)cached.SetTo(nextBlockPos);

	int count = 0;

	uint8* data = new(std::nothrow) uint8[fBlockSize];
	if (data == NULL) {
		TRACE("Journal::_RecoverPassReplay(): Failed to allocate memory for "
			"data\n");
		return B_NO_MEMORY;
	}

	ArrayDeleter<uint8> dataDeleter(data);

	while (nextCommitID < lastCommitID) {
		if (!header->CheckMagic() || header->Sequence() != nextCommitID) {
			// Somehow the log is different than the expected
			ERROR("Journal::_RecoverPassReplay(): Weird problem with block\n");
			return B_ERROR;
		}

		uint32 blockType = header->BlockType();

		if (blockType == JOURNAL_DESCRIPTOR_BLOCK) {
			JournalBlockTag* last_tag = (JournalBlockTag*)((uint8*)header
				+ fBlockSize - sizeof(JournalBlockTag));

			for (JournalBlockTag* tag = (JournalBlockTag*)header->data;
				tag <= last_tag; ++tag) {
				nextBlock = _WrapAroundLog(nextBlock + 1);

				status = MapBlock(nextBlock, nextBlockPos);
				if (status != B_OK)
					return status;

				if (!fRevokeManager->Lookup(tag->BlockNumber(),
						nextCommitID)) {
					// Block isn't revoked
					size_t read = read_pos(fJournalVolume->Device(),
						nextBlockPos * fBlockSize, data, fBlockSize);
					if (read != fBlockSize)
						return B_IO_ERROR;

					if ((tag->Flags() & JOURNAL_FLAG_ESCAPED) != 0) {
						// Block is escaped
						((int32*)data)[0]
							= B_HOST_TO_BENDIAN_INT32(JOURNAL_MAGIC);
					}

					TRACE("Journal::_RevoverPassReplay(): Write to %" B_PRIu32
						"\n", tag->BlockNumber() * fBlockSize);
					size_t written = write_pos(fFilesystemVolume->Device(),
						tag->BlockNumber() * fBlockSize, data, fBlockSize);

					if (written != fBlockSize)
						return B_IO_ERROR;

					++count;
				}

				if ((tag->Flags() & JOURNAL_FLAG_LAST_TAG) != 0)
					break;
				if ((tag->Flags() & JOURNAL_FLAG_SAME_UUID) == 0) {
					// TODO: Check new UUID with file system UUID
					tag += 2;
						// sizeof(JournalBlockTag) = 8
						// sizeof(UUID) = 16
				}
			}
		} else if (blockType == JOURNAL_COMMIT_BLOCK)
			nextCommitID++;
		else if (blockType != JOURNAL_REVOKE_BLOCK) {
			WARN("Journal::_RecoverPassReplay(): Found an unrecognized block\n");
			break;
		} // If blockType == JOURNAL_REVOKE_BLOCK we just skip it

		nextBlock = _WrapAroundLog(nextBlock + 1);

		status = MapBlock(nextBlock, nextBlockPos);
		if (status != B_OK)
			return status;

		header = (JournalHeader*)cached.SetTo(nextBlockPos);
	}

	if (nextCommitID != lastCommitID) {
		// Possibly because of some sort of IO error
		return B_ERROR;
	}

	TRACE("Journal recovery pass replay: Replayed blocks: %u\n", count);

	return B_OK;
}


status_t
Journal::_FlushLog(bool canWait, bool flushBlocks)
{
	TRACE("Journal::_FlushLog()\n");
	status_t status = canWait ? recursive_lock_lock(&fLock)
		: recursive_lock_trylock(&fLock);

	TRACE("Journal::_FlushLog(): Acquired fLock, recursion: %" B_PRId32 "\n",
		recursive_lock_get_recursion(&fLock));
	if (status != B_OK)
		return status;

	if (recursive_lock_get_recursion(&fLock) > 1) {
		// Called from inside a transaction
		recursive_lock_unlock(&fLock);
		TRACE("Journal::_FlushLog(): Called from a transaction. Leaving...\n");
		return B_OK;
	}

	if (fUnwrittenTransactions != 0 && _FullTransactionSize() != 0) {
		status = _WriteTransactionToLog();
		if (status < B_OK)
			panic("Failed flushing transaction: %s\n", strerror(status));
	}

	TRACE("Journal::_FlushLog(): Attempting to flush journal volume at %p\n",
		fJournalVolume);

	// TODO: Not sure this is correct. Need to review...
	// NOTE: Not correct. Causes double lock of a block cache mutex
	// TODO: Need some other way to synchronize the journal...
	/*status = fJournalVolume->FlushDevice();
	if (status != B_OK)
		return status;*/

	TRACE("Journal::_FlushLog(): Flushed journal volume\n");

	if (flushBlocks) {
		TRACE("Journal::_FlushLog(): Attempting to flush file system volume "
			"at %p\n", fFilesystemVolume);
		status = fFilesystemVolume->FlushDevice();
		if (status == B_OK)
			TRACE("Journal::_FlushLog(): Flushed file system volume\n");
	}

	TRACE("Journal::_FlushLog(): Finished. Releasing lock\n");

	recursive_lock_unlock(&fLock);

	TRACE("Journal::_FlushLog(): Done, final status: %s\n", strerror(status));
	return status;
}


inline uint32
Journal::_WrapAroundLog(uint32 block)
{
	TRACE("Journal::_WrapAroundLog()\n");
	if (block >= fLogSize)
		return block - fLogSize + fFirstLogBlock;
	else
		return block;
}


size_t
Journal::_CurrentTransactionSize() const
{
	TRACE("Journal::_CurrentTransactionSize(): transaction %" B_PRIu32 "\n",
		fTransactionID);

	size_t count;

	if (fHasSubTransaction) {
		count = cache_blocks_in_sub_transaction(fFilesystemBlockCache,
			fTransactionID);

		TRACE("\tSub transaction size: %" B_PRIuSIZE "\n", count);
	} else {
		count =  cache_blocks_in_transaction(fFilesystemBlockCache,
			fTransactionID);

		TRACE("\tTransaction size: %" B_PRIuSIZE "\n", count);
	}

	return count;
}


size_t
Journal::_FullTransactionSize() const
{
	TRACE("Journal::_FullTransactionSize(): transaction %" B_PRIu32 "\n",
		fTransactionID);
	TRACE("\tFile sytem block cache: %p\n", fFilesystemBlockCache);

	size_t count = cache_blocks_in_transaction(fFilesystemBlockCache,
		 fTransactionID);

	TRACE("\tFull transaction size: %" B_PRIuSIZE "\n", count);

	return count;
}


size_t
Journal::_MainTransactionSize() const
{
	TRACE("Journal::_MainTransactionSize(): transaction %" B_PRIu32 "\n",
		fTransactionID);

	size_t count =  cache_blocks_in_main_transaction(fFilesystemBlockCache,
		fTransactionID);

	TRACE("\tMain transaction size: %" B_PRIuSIZE "\n", count);

	return count;
}


status_t
Journal::_TransactionDone(bool success)
{
	if (!success) {
		if (fHasSubTransaction) {
			TRACE("Journal::_TransactionDone(): transaction %" B_PRIu32
				" failed, aborting subtransaction\n", fTransactionID);
			cache_abort_sub_transaction(fFilesystemBlockCache, fTransactionID);
			// parent is unaffected
		} else {
			TRACE("Journal::_TransactionDone(): transaction %" B_PRIu32
				" failed, aborting\n", fTransactionID);
			cache_abort_transaction(fFilesystemBlockCache, fTransactionID);
			fUnwrittenTransactions = 0;
		}

		TRACE("Journal::_TransactionDone(): returning B_OK\n");
		return B_OK;
	}

	// If possible, delay flushing the transaction
	uint32 size = _FullTransactionSize();
	TRACE("Journal::_TransactionDone(): full transaction size: %" B_PRIu32
		", max transaction size: %" B_PRIu32 ", free log blocks: %" B_PRIu32
		"\n", size, fMaxTransactionSize, FreeLogBlocks());
	if (fMaxTransactionSize > 0 && size < fMaxTransactionSize) {
		TRACE("Journal::_TransactionDone(): delaying flush of transaction "
			"%" B_PRIu32 "\n", fTransactionID);

		// Make sure the transaction fits in the log
		if (size < FreeLogBlocks())
			cache_sync_transaction(fFilesystemBlockCache, fTransactionID);

		fUnwrittenTransactions++;
		TRACE("Journal::_TransactionDone(): returning B_OK\n");
		return B_OK;
	}

	return _WriteTransactionToLog();
}


/*static*/ void
Journal::_TransactionWritten(int32 transactionID, int32 event, void* _logEntry)
{
	LogEntry* logEntry = (LogEntry*)_logEntry;

	TRACE("Journal::_TransactionWritten(): Transaction %" B_PRIu32
		" checkpointed\n", transactionID);

	Journal* journal = logEntry->GetJournal();

	TRACE("Journal::_TransactionWritten(): log entry: %p, journal: %p\n",
		logEntry, journal);
	TRACE("Journal::_TransactionWritten(): log entries: %p\n",
		&journal->fLogEntries);

	mutex_lock(&journal->fLogEntriesLock);

	TRACE("Journal::_TransactionWritten(): first log entry: %p\n",
		journal->fLogEntries.First());
	if (logEntry == journal->fLogEntries.First()) {
		TRACE("Journal::_TransactionWritten(): Moving start of log to %"
			B_PRIu32 "\n", logEntry->Start());
		journal->fLogStart = logEntry->Start();
		journal->fFirstCommitID = logEntry->CommitID();
		TRACE("Journal::_TransactionWritten(): Setting commit ID to %" B_PRIu32
			"\n", logEntry->CommitID());

		if (journal->_SaveSuperBlock() != B_OK)
			panic("ext2: Failed to write journal superblock\n");
	}

	TRACE("Journal::_TransactionWritten(): Removing log entry\n");
	journal->fLogEntries.Remove(logEntry);

	TRACE("Journal::_TransactionWritten(): Unlocking entries list\n");
	mutex_unlock(&journal->fLogEntriesLock);

	TRACE("Journal::_TransactionWritten(): Deleting log entry at %p\n", logEntry);
	delete logEntry;
}


/*static*/ void
Journal::_TransactionIdle(int32 transactionID, int32 event, void* _journal)
{
	Journal* journal = (Journal*)_journal;
	journal->_FlushLog(false, false);
}
