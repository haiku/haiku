/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de
 * This file may be used under the terms of the MIT License.
 */

//!	Transaction and logging

#include "Journal.h"
#include "Inode.h"

#include <Drivers.h>
#include <util/kernel_cpp.h>


struct run_array {
	int32		count;
	int32		max_runs;
	block_run	runs[0];

	int32 CountRuns() const { return BFS_ENDIAN_TO_HOST_INT32(count); }
	int32 MaxRuns() const { return BFS_ENDIAN_TO_HOST_INT32(max_runs); }
	const block_run &RunAt(int32 i) const { return runs[i]; }

	static int32 MaxRuns(int32 blockSize)
		{ return (blockSize - sizeof(run_array)) / sizeof(block_run); }
};

class LogEntry : public DoublyLinkedListLinkImpl<LogEntry> {
	public:
		LogEntry(Journal *journal, uint32 logStart);
		~LogEntry();

		status_t InitCheck() const { return fArray != NULL ? B_OK : B_NO_MEMORY; }

		uint32 Start() const { return fStart; }
		uint32 Length() const { return fLength; }

		Journal *GetJournal() { return fJournal; }

		bool InsertBlock(off_t blockNumber);
		bool NotifyBlocks(int32 count);

		run_array *Array() const { return fArray; }
		int32 CountRuns() const { return fArray->CountRuns(); }
		int32 MaxRuns() const { return fArray->MaxRuns() - 1; }
			// the -1 is an off-by-one error in Be's BFS implementation
		const block_run &RunAt(int32 i) const { return fArray->RunAt(i); }

	private:
		Journal		*fJournal;
		uint32		fStart;
		uint32		fLength;
		uint32		fCachedBlocks;
		run_array	*fArray;
};


//	#pragma mark -


LogEntry::LogEntry(Journal *journal, uint32 start)
	:
	fJournal(journal),
	fStart(start),
	fLength(1),
	fCachedBlocks(0)
{
	int32 blockSize = fJournal->GetVolume()->BlockSize();
	fArray = (run_array *)malloc(blockSize);
	if (fArray == NULL)
		return;

	memset(fArray, 0, blockSize);
	fArray->max_runs = HOST_ENDIAN_TO_BFS_INT32(run_array::MaxRuns(blockSize));
}


LogEntry::~LogEntry()
{
	free(fArray);
}


/**	Adds the specified block into the array.
 */

bool
LogEntry::InsertBlock(off_t blockNumber)
{
	// Be's BFS log replay routine can only deal with block_runs of size 1
	// A pity, isn't it? Too sad we have to be compatible.

	if (CountRuns() >= MaxRuns())
		return false;

	block_run run = fJournal->GetVolume()->ToBlockRun(blockNumber);

	fArray->runs[CountRuns()] = run;
	fArray->count = HOST_ENDIAN_TO_BFS_INT32(CountRuns() + 1);
	fLength++;
	fCachedBlocks++;
	return true;
}


bool
LogEntry::NotifyBlocks(int32 count)
{
	fCachedBlocks -= count;
	return fCachedBlocks == 0;
}


//	#pragma mark -


Journal::Journal(Volume *volume)
	:
	fVolume(volume),
	fLock("bfs journal"),
	fOwner(NULL),
	fArray(volume->BlockSize()),
	fLogSize(volume->Log().Length()),
	fMaxTransactionSize(fLogSize / 4 - 5),
	fUsed(0),
	fTransactionsInEntry(0)
{
	if (fMaxTransactionSize > fLogSize / 2)
		fMaxTransactionSize = fLogSize / 2 - 5;
}


Journal::~Journal()
{
	FlushLogAndBlocks();
}


status_t
Journal::InitCheck()
{
	if (fVolume->LogStart() != fVolume->LogEnd()) {
		if (fVolume->SuperBlock().flags != SUPER_BLOCK_DISK_DIRTY)
			FATAL(("log_start and log_end differ, but disk is marked clean - trying to replay log...\n"));

		return ReplayLog();
	}

	return B_OK;
}


status_t
Journal::_CheckRunArray(const run_array *array)
{
	int32 maxRuns = run_array::MaxRuns(fVolume->BlockSize());
	if (array->MaxRuns() != maxRuns
		|| array->CountRuns() > maxRuns
		|| array->CountRuns() <= 0) {
		FATAL(("Log entry has broken header!\n"));
		return B_ERROR;
	}

	for (int32 i = 0; i < array->CountRuns(); i++) {
		if (fVolume->ValidateBlockRun(array->RunAt(i)) != B_OK)
			return B_ERROR;
	}

	PRINT(("Log entry has %ld entries (%Ld)\n", array->CountRuns()));
	return B_OK;
}


/**	Replays an entry in the log.
 *	\a _start points to the entry in the log, and will be bumped to the next
 *	one if replaying succeeded.
 */

status_t
Journal::_ReplayRunArray(int32 *_start)
{
	PRINT(("ReplayRunArray(start = %ld)\n", *_start));

	off_t logOffset = fVolume->ToBlock(fVolume->Log());
	off_t blockNumber = *_start % fLogSize;
	int32 blockSize = fVolume->BlockSize();
	int32 count = 1;

	CachedBlock cachedArray(fVolume);

	const run_array *array = (const run_array *)cachedArray.SetTo(logOffset + blockNumber);
	if (array == NULL)
		return B_IO_ERROR;

	if (_CheckRunArray(array) < B_OK)
		return B_BAD_DATA;

	blockNumber = (blockNumber + 1) % fLogSize;

	CachedBlock cached(fVolume);
	for (int32 index = 0; index < array->CountRuns(); index++) {
		const block_run &run = array->RunAt(index);
		PRINT(("replay block run %lu:%u:%u in log at %Ld!\n", run.AllocationGroup(),
			run.Start(), run.Length(), blockNumber));

		off_t offset = fVolume->ToOffset(run);
		for (int32 i = 0; i < run.Length(); i++) {
			const uint8 *data = cached.SetTo(logOffset + blockNumber);
			if (data == NULL)
				RETURN_ERROR(B_IO_ERROR);

			ssize_t written = write_pos(fVolume->Device(),
				offset + (i * blockSize), data, blockSize);
			if (written != blockSize)
				RETURN_ERROR(B_IO_ERROR);

			blockNumber = (blockNumber + 1) % fLogSize;
			count++;
		}
	}

	*_start += count;
	return B_OK;
}


/**	Replays all log entries - this will put the disk into a
 *	consistent and clean state, if it was not correctly unmounted
 *	before.
 *	This method is called by Journal::InitCheck() if the log start
 *	and end pointer don't match.
 */

status_t
Journal::ReplayLog()
{
	INFORM(("Replay log, disk was not correctly unmounted...\n"));

	int32 start = fVolume->LogStart();
	int32 lastStart = -1;
	while (true) {
		// stop if the log is completely flushed
		if (start == fVolume->LogEnd())
			break;

		if (start == lastStart) {
			// strange, flushing the log hasn't changed the log_start pointer
			return B_ERROR;
		}
		lastStart = start;

		status_t status = _ReplayRunArray(&start);
		if (status < B_OK) {
			FATAL(("replaying log entry from %ld failed: %s\n", start, strerror(status)));
			return B_ERROR;
		}
		start = start % fLogSize;
	}
	
	PRINT(("replaying worked fine!\n"));
	fVolume->SuperBlock().log_start = HOST_ENDIAN_TO_BFS_INT64(fVolume->LogEnd());
	fVolume->LogStart() = fVolume->LogEnd();
	fVolume->SuperBlock().flags = HOST_ENDIAN_TO_BFS_INT32(SUPER_BLOCK_DISK_CLEAN);

	return fVolume->WriteSuperBlock();
}


/**	This is a callback function that is called by the cache, whenever
 *	a block is flushed to disk that was updated as part of a transaction.
 *	This is necessary to keep track of completed transactions, to be
 *	able to update the log start pointer.
 */

void
Journal::blockNotify(off_t blockNumber, size_t numBlocks, void *arg)
{
	LogEntry *logEntry = (LogEntry *)arg;

	if (!logEntry->NotifyBlocks(numBlocks)) {
		// nothing to do yet...
		return;
	}

	Journal *journal = logEntry->GetJournal();
	disk_super_block &superBlock = journal->fVolume->SuperBlock();
	bool update = false;

	// Set log_start pointer if possible...

	journal->fEntriesLock.Lock();

	if (logEntry == journal->fEntries.First()) {
		LogEntry *next = journal->fEntries.GetNext(logEntry);
		if (next != NULL) {
			int32 length = next->Start() - logEntry->Start();
			superBlock.log_start = HOST_ENDIAN_TO_BFS_INT64((superBlock.LogStart() + length) % journal->fLogSize);
		} else
			superBlock.log_start = HOST_ENDIAN_TO_BFS_INT64(journal->fVolume->LogEnd());

		update = true;
	}

	journal->fUsed -= logEntry->Length();
	journal->fEntries.Remove(logEntry);
	journal->fEntriesLock.Unlock();

	free(logEntry);

	// update the super block, and change the disk's state, if necessary

	if (update) {
		journal->fVolume->LogStart() = superBlock.log_start;

		if (superBlock.log_start == superBlock.log_end)
			superBlock.flags = SUPER_BLOCK_DISK_CLEAN;

		status_t status = journal->fVolume->WriteSuperBlock();
		if (status != B_OK)
			FATAL(("blockNotify: could not write back super block: %s\n", strerror(status)));
	}
}


status_t
Journal::WriteLogEntry()
{
	fTransactionsInEntry = 0;
	fHasChangedBlocks = false;

	sorted_array *array = fArray.Array();
	if (array == NULL || array->count == 0)
		return B_OK;

	// Make sure there is enough space in the log.
	// If that fails for whatever reason, panic!
	force_cache_flush(fVolume->Device(), false);
	int32 tries = fLogSize / 2 + 1;
	while (TransactionSize() > FreeLogBlocks() && tries-- > 0)
		force_cache_flush(fVolume->Device(), true);

	if (tries <= 0) {
		fVolume->Panic();
		return B_BAD_DATA;
	}

	int32 blockShift = fVolume->BlockShift();
	off_t logOffset = fVolume->ToBlock(fVolume->Log()) << blockShift;
	off_t logStart = fVolume->LogEnd();
	off_t logPosition = logStart % fLogSize;

	// Create log entries for the transaction

	LogEntry *logEntry = NULL, *firstEntry = NULL, *lastAdded = NULL;

	for (int32 i = 0; i < array->CountItems(); i++) {
	retry:
		if (logEntry == NULL) {
			logEntry = new LogEntry(this, logStart);
			if (logEntry == NULL)
				return B_NO_MEMORY;
			if (logEntry->InitCheck() != B_OK) {
				delete logEntry;
				return B_NO_MEMORY;
			}
			if (firstEntry == NULL)
				firstEntry = logEntry;

			logStart++;
		}

		if (!logEntry->InsertBlock(array->ValueAt(i))) {
			// log entry is full - start a new one
			fEntriesLock.Lock();
			fEntries.Add(logEntry);
			fEntriesLock.Unlock();
			lastAdded = logEntry;

			logEntry = NULL;
			goto retry;
		}

		logStart++;
	}

	if (firstEntry == NULL)
		return B_OK;

	if (logEntry != lastAdded) {
		fEntriesLock.Lock();
		fEntries.Add(logEntry);
		fEntriesLock.Unlock();
	}

	// Write log entries to disk

	CachedBlock cached(fVolume);
	fEntriesLock.Lock();

	for (logEntry = firstEntry; logEntry != NULL; logEntry = fEntries.GetNext(logEntry)) {
		// first write the log entry array
		write_pos(fVolume->Device(), logOffset + (logPosition << blockShift),
			logEntry->Array(), fVolume->BlockSize());

		logPosition = (logPosition + 1) % fLogSize;

		for (int32 i = 0; i < logEntry->CountRuns(); i++) {
			uint8 *block = cached.SetTo(logEntry->RunAt(i));
			if (block == NULL)
				return B_IO_ERROR;

			// write blocks
			write_pos(fVolume->Device(), logOffset + (logPosition << blockShift),
				block, fVolume->BlockSize());

			logPosition = (logPosition + 1) % fLogSize;
		}
	}

	fEntriesLock.Unlock();

	fUsed += array->CountItems();

	// Update the log end pointer in the super block
	fVolume->SuperBlock().flags = HOST_ENDIAN_TO_BFS_INT32(SUPER_BLOCK_DISK_DIRTY);
	fVolume->SuperBlock().log_end = HOST_ENDIAN_TO_BFS_INT64(logPosition);
	fVolume->LogEnd() = logPosition;

	status_t status = fVolume->WriteSuperBlock();

	// We need to flush the drives own cache here to ensure
	// disk consistency.
	// If that call fails, we can't do anything about it anyway
	ioctl(fVolume->Device(), B_FLUSH_DRIVE_CACHE);

	fEntriesLock.Lock();
	logStart = firstEntry->Start();

	for (logEntry = firstEntry; logEntry != NULL; logEntry = fEntries.GetNext(logEntry)) {
		// Note: this only works this way as we only have block_runs of length 1
		// We're reusing the fArray array, as we don't need it anymore, and
		// it's guaranteed to be large enough for us, too
		for (int32 i = 0; i < logEntry->CountRuns(); i++) {
			array->values[i] = fVolume->ToBlock(logEntry->RunAt(i));
		}

		set_blocks_info(fVolume->Device(), &array->values[0],
			logEntry->CountRuns(), blockNotify, logEntry);
	}

	fEntriesLock.Unlock();

	fArray.MakeEmpty();

	// If the log goes to the next round (the log is written as a
	// circular buffer), all blocks will be flushed out which is
	// possible because we don't have any locked blocks at this
	// point.
	if (logPosition < logStart)
		fVolume->FlushDevice();

	return status;
}


status_t 
Journal::FlushLogAndBlocks()
{
	status_t status = Lock((Transaction *)this);
	if (status != B_OK)
		return status;

	// write the current log entry to disk
	
	if (TransactionSize() != 0) {
		status = WriteLogEntry();
		if (status < B_OK)
			FATAL(("writing current log entry failed: %s\n", strerror(status)));
	}
	status = fVolume->FlushDevice();

	Unlock((Transaction *)this, true);
	return status;
}


status_t
Journal::Lock(Transaction *owner)
{
	if (owner == fOwner)
		return B_OK;

	status_t status = fLock.Lock();
	if (status == B_OK)
		fOwner = owner;

	// if the last transaction is older than 2 secs, start a new one
	if (fTransactionsInEntry != 0 && system_time() - fTimestamp > 2000000L)
		WriteLogEntry();

	return B_OK;
}


void
Journal::Unlock(Transaction *owner, bool success)
{
	if (owner != fOwner)
		return;

	TransactionDone(success);

	fTimestamp = system_time();
	fOwner = NULL;
	fLock.Unlock();
}


/** If there is a current transaction that the current thread has
 *	started, this function will give you access to it.
 */

Transaction *
Journal::CurrentTransaction()
{
	if (fLock.LockWithTimeout(0) != B_OK)
		return NULL;

	Transaction *owner = fOwner;
	fLock.Unlock();

	return owner;
}


status_t
Journal::TransactionDone(bool success)
{
	if (!success && fTransactionsInEntry == 0) {
		// we can safely abort the transaction
		sorted_array *array = fArray.Array();
		if (array != NULL) {
			// release the lock for all blocks in the array (we don't need
			// to be notified when they are actually written to disk)
			for (int32 i = 0; i < array->CountItems(); i++)
				release_block(fVolume->Device(), array->ValueAt(i));
		}

		return B_OK;
	}

	// Up to a maximum size, we will just batch several
	// transactions together to improve speed
	if (TransactionSize() < fMaxTransactionSize) {
		fTransactionsInEntry++;
		fHasChangedBlocks = false;

		return B_OK;
	}

	return WriteLogEntry();
}


status_t
Journal::LogBlocks(off_t blockNumber, const uint8 *buffer, size_t numBlocks)
{
	// ToDo: that's for now - we should change the log file size here
	if (TransactionSize() + numBlocks + 1 > fLogSize)
		return B_DEVICE_FULL;

	fHasChangedBlocks = true;
	int32 blockSize = fVolume->BlockSize();

	for (;numBlocks-- > 0; blockNumber++, buffer += blockSize) {
		if (fArray.Find(blockNumber) >= 0) {
			// The block is already in the log, so just update its data
			// Note, this is only necessary if this method is called with a buffer
			// different from the cached block buffer - which is unlikely but
			// we'll make sure this way (costs one cache lookup, though).
			status_t status = cached_write(fVolume->Device(), blockNumber, buffer, 1, blockSize);
			if (status < B_OK)
				return status;

			continue;
		}

		// Insert the block into the transaction's array, and write the changes
		// back into the locked cache buffer
		fArray.Insert(blockNumber);
		status_t status = cached_write_locked(fVolume->Device(), blockNumber, buffer, 1, blockSize);
		if (status < B_OK)
			return status;
	}

	// If necessary, flush the log, so that we have enough space for this transaction
	if (TransactionSize() > FreeLogBlocks())
		force_cache_flush(fVolume->Device(), true);

	return B_OK;
}


//	#pragma mark -


status_t 
Transaction::Start(Volume *volume, off_t refBlock)
{
	// has it already been started?
	if (fJournal != NULL)
		return B_OK;

	fJournal = volume->GetJournal(refBlock);
	if (fJournal != NULL && fJournal->Lock(this) == B_OK)
		return B_OK;

	fJournal = NULL;
	return B_ERROR;
}

