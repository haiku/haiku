/*
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//! Transaction and logging


#include "Journal.h"

#include "Debug.h"
#include "Inode.h"


struct run_array {
	int32		count;
	int32		max_runs;
	block_run	runs[0];

	void Init(int32 blockSize);
	void Insert(block_run& run);

	int32 CountRuns() const { return BFS_ENDIAN_TO_HOST_INT32(count); }
	int32 MaxRuns() const { return BFS_ENDIAN_TO_HOST_INT32(max_runs) - 1; }
		// that -1 accounts for an off-by-one error in Be's BFS implementation
	const block_run& RunAt(int32 i) const { return runs[i]; }

	static int32 MaxRuns(int32 blockSize);

private:
	static int _Compare(block_run& a, block_run& b);
	int32 _FindInsertionIndex(block_run& run);
};

class RunArrays {
public:
							RunArrays(Journal* journal);
							~RunArrays();

			status_t		Insert(off_t blockNumber);

			run_array*		ArrayAt(int32 i) { return fArrays.Array()[i]; }
			int32			CountArrays() const { return fArrays.CountItems(); }

			uint32			CountBlocks() const { return fBlockCount; }
			uint32			LogEntryLength() const
								{ return CountBlocks() + CountArrays(); }

			int32			MaxArrayLength();

private:
			status_t		_AddArray();
			bool			_ContainsRun(block_run& run);
			bool			_AddRun(block_run& run);

			Journal*		fJournal;
			uint32			fBlockCount;
			Stack<run_array*> fArrays;
			run_array*		fLastArray;
};

class LogEntry : public DoublyLinkedListLinkImpl<LogEntry> {
public:
							LogEntry(Journal* journal, uint32 logStart,
								uint32 length);
							~LogEntry();

			uint32			Start() const { return fStart; }
			uint32			Length() const { return fLength; }

#ifdef BFS_DEBUGGER_COMMANDS
			void			SetTransactionID(int32 id) { fTransactionID = id; }
			int32			TransactionID() const { return fTransactionID; }
#endif

			Journal*		GetJournal() { return fJournal; }

private:
			Journal*		fJournal;
			uint32			fStart;
			uint32			fLength;
#ifdef BFS_DEBUGGER_COMMANDS
			int32			fTransactionID;
#endif
};


#if BFS_TRACING && !defined(BFS_SHELL) && !defined(_BOOT_MODE)
namespace BFSJournalTracing {

class LogEntry : public AbstractTraceEntry {
public:
	LogEntry(::LogEntry* entry, off_t logPosition, bool started)
		:
		fEntry(entry),
#ifdef BFS_DEBUGGER_COMMANDS
		fTransactionID(entry->TransactionID()),
#endif
		fStart(entry->Start()),
		fLength(entry->Length()),
		fLogPosition(logPosition),
		fStarted(started)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
#ifdef BFS_DEBUGGER_COMMANDS
		out.Print("bfs:j:%s entry %p id %ld, start %lu, length %lu, log %s "
			"%lu\n", fStarted ? "Started" : "Written", fEntry,
			fTransactionID, fStart, fLength,
			fStarted ? "end" : "start", fLogPosition);
#else
		out.Print("bfs:j:%s entry %p start %lu, length %lu, log %s %lu\n",
			fStarted ? "Started" : "Written", fEntry, fStart, fLength,
			fStarted ? "end" : "start", fLogPosition);
#endif
	}

private:
	::LogEntry*	fEntry;
#ifdef BFS_DEBUGGER_COMMANDS
	int32		fTransactionID;
#endif
	uint32		fStart;
	uint32		fLength;
	uint32		fLogPosition;
	bool		fStarted;
};

}	// namespace BFSJournalTracing

#	define T(x) new(std::nothrow) BFSJournalTracing::x;
#else
#	define T(x) ;
#endif


//	#pragma mark -


static void
add_to_iovec(iovec* vecs, int32& index, int32 max, const void* address,
	size_t size)
{
	if (index > 0 && (addr_t)vecs[index - 1].iov_base
			+ vecs[index - 1].iov_len == (addr_t)address) {
		// the iovec can be combined with the previous one
		vecs[index - 1].iov_len += size;
		return;
	}

	if (index == max)
		panic("no more space for iovecs!");

	// we need to start a new iovec
	vecs[index].iov_base = const_cast<void*>(address);
	vecs[index].iov_len = size;
	index++;
}


//	#pragma mark - LogEntry


LogEntry::LogEntry(Journal* journal, uint32 start, uint32 length)
	:
	fJournal(journal),
	fStart(start),
	fLength(length)
{
}


LogEntry::~LogEntry()
{
}


//	#pragma mark - run_array


/*!	The run_array's size equals the block size of the BFS volume, so we
	cannot use a (non-overridden) new.
	This makes a freshly allocated run_array ready to run.
*/
void
run_array::Init(int32 blockSize)
{
	memset(this, 0, blockSize);
	count = 0;
	max_runs = HOST_ENDIAN_TO_BFS_INT32(MaxRuns(blockSize));
}


/*!	Inserts the block_run into the array. You will have to make sure the
	array is large enough to contain the entry before calling this function.
*/
void
run_array::Insert(block_run& run)
{
	int32 index = _FindInsertionIndex(run);
	if (index == -1) {
		// add to the end
		runs[CountRuns()] = run;
	} else {
		// insert at index
		memmove(&runs[index + 1], &runs[index],
			(CountRuns() - index) * sizeof(off_t));
		runs[index] = run;
	}

	count = HOST_ENDIAN_TO_BFS_INT32(CountRuns() + 1);
}


/*static*/ int32
run_array::MaxRuns(int32 blockSize)
{
	// For whatever reason, BFS restricts the maximum array size
	uint32 maxCount = (blockSize - sizeof(run_array)) / sizeof(block_run);
	if (maxCount < 128)
		return maxCount;

	return 127;
}


/*static*/ int
run_array::_Compare(block_run& a, block_run& b)
{
	int cmp = a.AllocationGroup() - b.AllocationGroup();
	if (cmp == 0)
		return a.Start() - b.Start();

	return cmp;
}


int32
run_array::_FindInsertionIndex(block_run& run)
{
	int32 min = 0, max = CountRuns() - 1;
	int32 i = 0;
	if (max >= 8) {
		while (min <= max) {
			i = (min + max) / 2;

			int cmp = _Compare(runs[i], run);
			if (cmp < 0)
				min = i + 1;
			else if (cmp > 0)
				max = i - 1;
			else
				return -1;
		}

		if (_Compare(runs[i], run) < 0)
			i++;
	} else {
		for (; i <= max; i++) {
			if (_Compare(runs[i], run) > 0)
				break;
		}
		if (i == count)
			return -1;
	}

	return i;
}


//	#pragma mark - RunArrays


RunArrays::RunArrays(Journal* journal)
	:
	fJournal(journal),
	fBlockCount(0),
	fArrays(),
	fLastArray(NULL)
{
}


RunArrays::~RunArrays()
{
	run_array* array;
	while (fArrays.Pop(&array))
		free(array);
}


bool
RunArrays::_ContainsRun(block_run& run)
{
	for (int32 i = 0; i < CountArrays(); i++) {
		run_array* array = ArrayAt(i);

		for (int32 j = 0; j < array->CountRuns(); j++) {
			block_run& arrayRun = array->runs[j];
			if (run.AllocationGroup() != arrayRun.AllocationGroup())
				continue;

			if (run.Start() >= arrayRun.Start()
				&& run.Start() + run.Length()
					<= arrayRun.Start() + arrayRun.Length())
				return true;
		}
	}

	return false;
}


/*!	Adds the specified block_run into the array.
	Note: it doesn't support overlapping - it must only be used
	with block_runs of length 1!
*/
bool
RunArrays::_AddRun(block_run& run)
{
	ASSERT(run.length == 1);

	// Be's BFS log replay routine can only deal with block_runs of size 1
	// A pity, isn't it? Too sad we have to be compatible.

	if (fLastArray == NULL || fLastArray->CountRuns() == fLastArray->MaxRuns())
		return false;

	fLastArray->Insert(run);
	fBlockCount++;
	return true;
}


status_t
RunArrays::_AddArray()
{
	int32 blockSize = fJournal->GetVolume()->BlockSize();

	run_array* array = (run_array*)malloc(blockSize);
	if (array == NULL)
		return B_NO_MEMORY;

	if (fArrays.Push(array) != B_OK) {
		free(array);
		return B_NO_MEMORY;
	}

	array->Init(blockSize);
	fLastArray = array;
	return B_OK;
}


status_t
RunArrays::Insert(off_t blockNumber)
{
	Volume* volume = fJournal->GetVolume();
	block_run run = volume->ToBlockRun(blockNumber);

	if (fLastArray != NULL) {
		// check if the block is already in the array
		if (_ContainsRun(run))
			return B_OK;
	}

	// insert block into array

	if (!_AddRun(run)) {
		// array is full
		if (_AddArray() != B_OK || !_AddRun(run))
			return B_NO_MEMORY;
	}

	return B_OK;
}


int32
RunArrays::MaxArrayLength()
{
	int32 max = 0;
	for (int32 i = 0; i < CountArrays(); i++) {
		if (ArrayAt(i)->CountRuns() > max)
			max = ArrayAt(i)->CountRuns();
	}

	return max;
}


//	#pragma mark - Journal


Journal::Journal(Volume* volume)
	:
	fVolume(volume),
	fOwner(NULL),
	fLogSize(volume->Log().Length()),
	fMaxTransactionSize(fLogSize / 2 - 5),
	fUsed(0),
	fUnwrittenTransactions(0),
	fHasSubtransaction(false),
	fSeparateSubTransactions(false)
{
	recursive_lock_init(&fLock, "bfs journal");
	mutex_init(&fEntriesLock, "bfs journal entries");
}


Journal::~Journal()
{
	FlushLogAndBlocks();

	recursive_lock_destroy(&fLock);
	mutex_destroy(&fEntriesLock);
}


status_t
Journal::InitCheck()
{
	return B_OK;
}


/*!	\brief Does a very basic consistency check of the run array.
	It will check the maximum run count as well as if all of the runs fall
	within a the volume.
*/
status_t
Journal::_CheckRunArray(const run_array* array)
{
	int32 maxRuns = run_array::MaxRuns(fVolume->BlockSize()) - 1;
		// the -1 works around an off-by-one bug in Be's BFS implementation,
		// same as in run_array::MaxRuns()
	if (array->MaxRuns() != maxRuns
		|| array->CountRuns() > maxRuns
		|| array->CountRuns() <= 0) {
		dprintf("run count: %d, array max: %d, max runs: %d\n",
			(int)array->CountRuns(), (int)array->MaxRuns(), (int)maxRuns);
		FATAL(("Log entry has broken header!\n"));
		return B_ERROR;
	}

	for (int32 i = 0; i < array->CountRuns(); i++) {
		if (fVolume->ValidateBlockRun(array->RunAt(i)) != B_OK)
			return B_ERROR;
	}

	PRINT(("Log entry has %ld entries\n", array->CountRuns()));
	return B_OK;
}


/*!	Replays an entry in the log.
	\a _start points to the entry in the log, and will be bumped to the next
	one if replaying succeeded.
*/
status_t
Journal::_ReplayRunArray(int32* _start)
{
	PRINT(("ReplayRunArray(start = %ld)\n", *_start));

	off_t logOffset = fVolume->ToBlock(fVolume->Log());
	off_t firstBlockNumber = *_start % fLogSize;

	CachedBlock cachedArray(fVolume);

	const run_array* array = (const run_array*)cachedArray.SetTo(logOffset
		+ firstBlockNumber);
	if (array == NULL)
		return B_IO_ERROR;

	if (_CheckRunArray(array) < B_OK)
		return B_BAD_DATA;

	// First pass: check integrity of the blocks in the run array

	CachedBlock cached(fVolume);

	firstBlockNumber = (firstBlockNumber + 1) % fLogSize;
	off_t blockNumber = firstBlockNumber;
	int32 blockSize = fVolume->BlockSize();

	for (int32 index = 0; index < array->CountRuns(); index++) {
		const block_run& run = array->RunAt(index);

		off_t offset = fVolume->ToOffset(run);
		for (int32 i = 0; i < run.Length(); i++) {
			const uint8* data = cached.SetTo(logOffset + blockNumber);
			if (data == NULL)
				RETURN_ERROR(B_IO_ERROR);

			// TODO: eventually check other well known offsets, like the
			// root and index dirs
			if (offset == 0) {
				// This log entry writes over the super block - check if
				// it's valid!
				if (Volume::CheckSuperBlock(data) != B_OK) {
					FATAL(("Log contains invalid super block!\n"));
					RETURN_ERROR(B_BAD_DATA);
				}
			}

			blockNumber = (blockNumber + 1) % fLogSize;
			offset += blockSize;
		}
	}

	// Second pass: write back its blocks

	blockNumber = firstBlockNumber;
	int32 count = 1;

	for (int32 index = 0; index < array->CountRuns(); index++) {
		const block_run& run = array->RunAt(index);
		INFORM(("replay block run %u:%u:%u in log at %" B_PRIdOFF "!\n",
			(int)run.AllocationGroup(), run.Start(), run.Length(), blockNumber));

		off_t offset = fVolume->ToOffset(run);
		for (int32 i = 0; i < run.Length(); i++) {
			const uint8* data = cached.SetTo(logOffset + blockNumber);
			if (data == NULL)
				RETURN_ERROR(B_IO_ERROR);

			ssize_t written = write_pos(fVolume->Device(), offset, data,
				blockSize);
			if (written != blockSize)
				RETURN_ERROR(B_IO_ERROR);

			blockNumber = (blockNumber + 1) % fLogSize;
			offset += blockSize;
			count++;
		}
	}

	*_start += count;
	return B_OK;
}


/*!	Replays all log entries - this will put the disk into a
	consistent and clean state, if it was not correctly unmounted
	before.
	This method is called by Journal::InitCheck() if the log start
	and end pointer don't match.
*/
status_t
Journal::ReplayLog()
{
	// TODO: this logic won't work whenever the size of the pending transaction
	//	equals the size of the log (happens with the original BFS only)
	if (fVolume->LogStart() == fVolume->LogEnd())
		return B_OK;

	INFORM(("Replay log, disk was not correctly unmounted...\n"));

	if (fVolume->SuperBlock().flags != SUPER_BLOCK_DISK_DIRTY) {
		INFORM(("log_start and log_end differ, but disk is marked clean - "
			"trying to replay log...\n"));
	}

	if (fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

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
		if (status != B_OK) {
			FATAL(("replaying log entry from %d failed: %s\n", (int)start,
				strerror(status)));
			return B_ERROR;
		}
		start = start % fLogSize;
	}

	PRINT(("replaying worked fine!\n"));
	fVolume->SuperBlock().log_start = HOST_ENDIAN_TO_BFS_INT64(
		fVolume->LogEnd());
	fVolume->LogStart() = HOST_ENDIAN_TO_BFS_INT64(fVolume->LogEnd());
	fVolume->SuperBlock().flags = SUPER_BLOCK_DISK_CLEAN;

	return fVolume->WriteSuperBlock();
}


size_t
Journal::CurrentTransactionSize() const
{
	if (_HasSubTransaction()) {
		return cache_blocks_in_sub_transaction(fVolume->BlockCache(),
			fTransactionID);
	}

	return cache_blocks_in_main_transaction(fVolume->BlockCache(),
		fTransactionID);
}


bool
Journal::CurrentTransactionTooLarge() const
{
	return CurrentTransactionSize() > fLogSize;
}


/*!	This is a callback function that is called by the cache, whenever
	all blocks of a transaction have been flushed to disk.
	This lets us keep track of completed transactions, and update
	the log start pointer as needed. Note, the transactions may not be
	completed in the order they were written.
*/
/*static*/ void
Journal::_TransactionWritten(int32 transactionID, int32 event, void* _logEntry)
{
	LogEntry* logEntry = (LogEntry*)_logEntry;

	PRINT(("Log entry %p has been finished, transaction ID = %ld\n", logEntry,
		transactionID));

	Journal* journal = logEntry->GetJournal();
	disk_super_block& superBlock = journal->fVolume->SuperBlock();
	bool update = false;

	// Set log_start pointer if possible...

	mutex_lock(&journal->fEntriesLock);

	if (logEntry == journal->fEntries.First()) {
		LogEntry* next = journal->fEntries.GetNext(logEntry);
		if (next != NULL) {
			superBlock.log_start = HOST_ENDIAN_TO_BFS_INT64(next->Start()
				% journal->fLogSize);
		} else {
			superBlock.log_start = HOST_ENDIAN_TO_BFS_INT64(
				journal->fVolume->LogEnd());
		}

		update = true;
	}

	T(LogEntry(logEntry, superBlock.LogStart(), false));

	journal->fUsed -= logEntry->Length();
	journal->fEntries.Remove(logEntry);
	mutex_unlock(&journal->fEntriesLock);

	delete logEntry;

	// update the super block, and change the disk's state, if necessary

	if (update) {
		if (superBlock.log_start == superBlock.log_end)
			superBlock.flags = SUPER_BLOCK_DISK_CLEAN;

		status_t status = journal->fVolume->WriteSuperBlock();
		if (status != B_OK) {
			FATAL(("_TransactionWritten: could not write back super block: %s\n",
				strerror(status)));
		}

		journal->fVolume->LogStart() = superBlock.LogStart();
	}
}


/*!	Listens to TRANSACTION_IDLE events, and flushes the log when that happens */
/*static*/ void
Journal::_TransactionIdle(int32 transactionID, int32 event, void* _journal)
{
	// The current transaction seems to be idle - flush it. We can't do this
	// in this thread, as flushing the log can produce new transaction events.
	thread_id id = spawn_kernel_thread(&Journal::_FlushLog, "bfs log flusher",
		B_NORMAL_PRIORITY, _journal);
	if (id > 0)
		resume_thread(id);
}


/*static*/ status_t
Journal::_FlushLog(void* _journal)
{
	Journal* journal = (Journal*)_journal;
	return journal->_FlushLog(false, false);
}


/*!	Writes the blocks that are part of current transaction into the log,
	and ends the current transaction.
	If the current transaction is too large to fit into the log, it will
	try to detach an existing sub-transaction.
*/
status_t
Journal::_WriteTransactionToLog()
{
	// TODO: in case of a failure, we need a backup plan like writing all
	//	changed blocks back to disk immediately (hello disk corruption!)

	bool detached = false;

	if (_TransactionSize() > fLogSize) {
		// The current transaction won't fit into the log anymore, try to
		// detach the current sub-transaction
		if (_HasSubTransaction() && cache_blocks_in_main_transaction(
				fVolume->BlockCache(), fTransactionID) < (int32)fLogSize) {
			detached = true;
		} else {
			// We created a transaction larger than one we can write back to
			// disk - the only option we have (besides risking disk corruption
			// by writing it back anyway), is to let it fail.
			dprintf("transaction too large (%d blocks, log size %d)!\n",
				(int)_TransactionSize(), (int)fLogSize);
			return B_BUFFER_OVERFLOW;
		}
	}

	fHasSubtransaction = false;

	int32 blockShift = fVolume->BlockShift();
	off_t logOffset = fVolume->ToBlock(fVolume->Log()) << blockShift;
	off_t logStart = fVolume->LogEnd() % fLogSize;
	off_t logPosition = logStart;
	status_t status;

	// create run_array structures for all changed blocks

	RunArrays runArrays(this);

	off_t blockNumber;
	long cookie = 0;
	while (cache_next_block_in_transaction(fVolume->BlockCache(),
			fTransactionID, detached, &cookie, &blockNumber, NULL,
			NULL) == B_OK) {
		status = runArrays.Insert(blockNumber);
		if (status < B_OK) {
			FATAL(("filling log entry failed!"));
			return status;
		}
	}

	if (runArrays.CountBlocks() == 0) {
		// nothing has changed during this transaction
		if (detached) {
			fTransactionID = cache_detach_sub_transaction(fVolume->BlockCache(),
				fTransactionID, NULL, NULL);
			fUnwrittenTransactions = 1;
		} else {
			cache_end_transaction(fVolume->BlockCache(), fTransactionID, NULL,
				NULL);
			fUnwrittenTransactions = 0;
		}
		return B_OK;
	}

	// If necessary, flush the log, so that we have enough space for this
	// transaction
	if (runArrays.LogEntryLength() > FreeLogBlocks()) {
		cache_sync_transaction(fVolume->BlockCache(), fTransactionID);
		if (runArrays.LogEntryLength() > FreeLogBlocks()) {
			panic("no space in log after sync (%ld for %ld blocks)!",
				(long)FreeLogBlocks(), (long)runArrays.LogEntryLength());
		}
	}

	// Write log entries to disk

	int32 maxVecs = runArrays.MaxArrayLength() + 1;
		// one extra for the index block

	iovec* vecs = (iovec*)malloc(sizeof(iovec) * maxVecs);
	if (vecs == NULL) {
		// TODO: write back log entries directly?
		return B_NO_MEMORY;
	}

	for (int32 k = 0; k < runArrays.CountArrays(); k++) {
		run_array* array = runArrays.ArrayAt(k);
		int32 index = 0, count = 1;
		int32 wrap = fLogSize - logStart;

		add_to_iovec(vecs, index, maxVecs, (void*)array, fVolume->BlockSize());

		// add block runs

		for (int32 i = 0; i < array->CountRuns(); i++) {
			const block_run& run = array->RunAt(i);
			off_t blockNumber = fVolume->ToBlock(run);

			for (int32 j = 0; j < run.Length(); j++) {
				if (count >= wrap) {
					// We need to write back the first half of the entry
					// directly as the log wraps around
					if (writev_pos(fVolume->Device(), logOffset
						+ (logStart << blockShift), vecs, index) < 0)
						FATAL(("could not write log area!\n"));

					logPosition = logStart + count;
					logStart = 0;
					wrap = fLogSize;
					count = 0;
					index = 0;
				}

				// make blocks available in the cache
				const void* data = block_cache_get(fVolume->BlockCache(),
					blockNumber + j);
				if (data == NULL) {
					free(vecs);
					return B_IO_ERROR;
				}

				add_to_iovec(vecs, index, maxVecs, data, fVolume->BlockSize());
				count++;
			}
		}

		// write back the rest of the log entry
		if (count > 0) {
			logPosition = logStart + count;
			if (writev_pos(fVolume->Device(), logOffset
					+ (logStart << blockShift), vecs, index) < 0)
				FATAL(("could not write log area: %s!\n", strerror(errno)));
		}

		// release blocks again
		for (int32 i = 0; i < array->CountRuns(); i++) {
			const block_run& run = array->RunAt(i);
			off_t blockNumber = fVolume->ToBlock(run);

			for (int32 j = 0; j < run.Length(); j++) {
				block_cache_put(fVolume->BlockCache(), blockNumber + j);
			}
		}

		logStart = logPosition % fLogSize;
	}

	free(vecs);

	LogEntry* logEntry = new LogEntry(this, fVolume->LogEnd(),
		runArrays.LogEntryLength());
	if (logEntry == NULL) {
		FATAL(("no memory to allocate log entries!"));
		return B_NO_MEMORY;
	}

#ifdef BFS_DEBUGGER_COMMANDS
	logEntry->SetTransactionID(fTransactionID);
#endif

	// Update the log end pointer in the super block

	fVolume->SuperBlock().flags = SUPER_BLOCK_DISK_DIRTY;
	fVolume->SuperBlock().log_end = HOST_ENDIAN_TO_BFS_INT64(logPosition);

	status = fVolume->WriteSuperBlock();

	fVolume->LogEnd() = logPosition;
	T(LogEntry(logEntry, fVolume->LogEnd(), true));

	// We need to flush the drives own cache here to ensure
	// disk consistency.
	// If that call fails, we can't do anything about it anyway
	ioctl(fVolume->Device(), B_FLUSH_DRIVE_CACHE);

	// at this point, we can finally end the transaction - we're in
	// a guaranteed valid state

	mutex_lock(&fEntriesLock);
	fEntries.Add(logEntry);
	fUsed += logEntry->Length();
	mutex_unlock(&fEntriesLock);

	if (detached) {
		fTransactionID = cache_detach_sub_transaction(fVolume->BlockCache(),
			fTransactionID, _TransactionWritten, logEntry);
		fUnwrittenTransactions = 1;

		if (status == B_OK && _TransactionSize() > fLogSize) {
			// If the transaction is too large after writing, there is no way to
			// recover, so let this transaction fail.
			dprintf("transaction too large (%d blocks, log size %d)!\n",
				(int)_TransactionSize(), (int)fLogSize);
			return B_BUFFER_OVERFLOW;
		}
	} else {
		cache_end_transaction(fVolume->BlockCache(), fTransactionID,
			_TransactionWritten, logEntry);
		fUnwrittenTransactions = 0;
	}

	return status;
}


/*!	Flushes the current log entry to disk. If \a flushBlocks is \c true it will
	also write back all dirty blocks for this volume.
*/
status_t
Journal::_FlushLog(bool canWait, bool flushBlocks)
{
	status_t status = canWait ? recursive_lock_lock(&fLock)
		: recursive_lock_trylock(&fLock);
	if (status != B_OK)
		return status;

	if (recursive_lock_get_recursion(&fLock) > 1) {
		// whoa, FlushLogAndBlocks() was called from inside a transaction
		recursive_lock_unlock(&fLock);
		return B_OK;
	}

	// write the current log entry to disk

	if (fUnwrittenTransactions != 0 && _TransactionSize() != 0) {
		status = _WriteTransactionToLog();
		if (status < B_OK)
			FATAL(("writing current log entry failed: %s\n", strerror(status)));
	}

	if (flushBlocks)
		status = fVolume->FlushDevice();

	recursive_lock_unlock(&fLock);
	return status;
}


/*!	Flushes the current log entry to disk, and also writes back all dirty
	blocks for this volume (completing all open transactions).
*/
status_t
Journal::FlushLogAndBlocks()
{
	return _FlushLog(true, true);
}


status_t
Journal::Lock(Transaction* owner, bool separateSubTransactions)
{
	status_t status = recursive_lock_lock(&fLock);
	if (status != B_OK)
		return status;

	if (!fSeparateSubTransactions && recursive_lock_get_recursion(&fLock) > 1) {
		// we'll just use the current transaction again
		return B_OK;
	}

	if (separateSubTransactions)
		fSeparateSubTransactions = true;

	if (owner != NULL)
		owner->SetParent(fOwner);

	fOwner = owner;

	// TODO: we need a way to find out how big the current transaction is;
	//	we need to be able to either detach the latest sub transaction on
	//	demand, as well as having some kind of fall back plan in case the
	//	sub transaction itself grows bigger than the log.
	//	For that, it would be nice to have some call-back interface in the
	//	cache transaction API...

	if (fOwner != NULL) {
		if (fUnwrittenTransactions > 0) {
			// start a sub transaction
			cache_start_sub_transaction(fVolume->BlockCache(), fTransactionID);
			fHasSubtransaction = true;
		} else
			fTransactionID = cache_start_transaction(fVolume->BlockCache());

		if (fTransactionID < B_OK) {
			recursive_lock_unlock(&fLock);
			return fTransactionID;
		}

		cache_add_transaction_listener(fVolume->BlockCache(), fTransactionID,
			TRANSACTION_IDLE, _TransactionIdle, this);
	}
	return B_OK;
}


status_t
Journal::Unlock(Transaction* owner, bool success)
{
	if (fSeparateSubTransactions || recursive_lock_get_recursion(&fLock) == 1) {
		// we only end the transaction if we would really unlock it
		// TODO: what about failing transactions that do not unlock?
		// (they must make the parent fail, too)
		if (owner != NULL) {
			status_t status = _TransactionDone(success);
			if (status != B_OK)
				return status;

			// Unlocking the inodes might trigger new transactions, but we
			// cannot reuse the current one anymore, as this one is already
			// closed.
			bool separateSubTransactions = fSeparateSubTransactions;
			fSeparateSubTransactions = true;
			owner->NotifyListeners(success);
			fSeparateSubTransactions = separateSubTransactions;

			fOwner = owner->Parent();
		} else
			fOwner = NULL;

		fTimestamp = system_time();

		if (fSeparateSubTransactions
			&& recursive_lock_get_recursion(&fLock) == 1)
			fSeparateSubTransactions = false;
	} else
		owner->MoveListenersTo(fOwner);

	recursive_lock_unlock(&fLock);
	return B_OK;
}


uint32
Journal::_TransactionSize() const
{
	int32 count = cache_blocks_in_transaction(fVolume->BlockCache(),
		fTransactionID);
	if (count <= 0)
		return 0;

	// take the number of array blocks in this transaction into account
	uint32 maxRuns = run_array::MaxRuns(fVolume->BlockSize());
	uint32 arrayBlocks = (count + maxRuns - 1) / maxRuns;
	return count + arrayBlocks;
}


status_t
Journal::_TransactionDone(bool success)
{
	if (!success) {
		if (_HasSubTransaction()) {
			cache_abort_sub_transaction(fVolume->BlockCache(), fTransactionID);
			// We can continue to use the parent transaction afterwards
		} else {
			cache_abort_transaction(fVolume->BlockCache(), fTransactionID);
			fUnwrittenTransactions = 0;
		}

		return B_OK;
	}

	// Up to a maximum size, we will just batch several
	// transactions together to improve speed
	uint32 size = _TransactionSize();
	if (size < fMaxTransactionSize) {
		// Flush the log from time to time, so that we have enough space
		// for this transaction
		if (size > FreeLogBlocks())
			cache_sync_transaction(fVolume->BlockCache(), fTransactionID);

		fUnwrittenTransactions++;
		return B_OK;
	}

	return _WriteTransactionToLog();
}


//	#pragma mark - debugger commands


#ifdef BFS_DEBUGGER_COMMANDS


void
Journal::Dump()
{
	kprintf("Journal %p\n", this);
	kprintf("  log start:            %ld\n", fVolume->LogStart());
	kprintf("  log end:              %ld\n", fVolume->LogEnd());
	kprintf("  owner:                %p\n", fOwner);
	kprintf("  log size:             %lu\n", fLogSize);
	kprintf("  max transaction size: %lu\n", fMaxTransactionSize);
	kprintf("  used:                 %lu\n", fUsed);
	kprintf("  unwritten:            %ld\n", fUnwrittenTransactions);
	kprintf("  timestamp:            %lld\n", fTimestamp);
	kprintf("  transaction ID:       %ld\n", fTransactionID);
	kprintf("  has subtransaction:   %d\n", fHasSubtransaction);
	kprintf("  separate sub-trans.:  %d\n", fSeparateSubTransactions);
	kprintf("entries:\n");
	kprintf("  address        id  start length\n");

	LogEntryList::Iterator iterator = fEntries.GetIterator();

	while (iterator.HasNext()) {
		LogEntry* entry = iterator.Next();

		kprintf("  %p %6ld %6lu %6lu\n", entry, entry->TransactionID(),
			entry->Start(), entry->Length());
	}
}


int
dump_journal(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-volume>\n", argv[0]);
		return 0;
	}

	Volume* volume = (Volume*)parse_expression(argv[1]);
	Journal* journal = volume->GetJournal(0);

	journal->Dump();
	return 0;
}


#endif	// BFS_DEBUGGER_COMMANDS


//	#pragma mark - TransactionListener


TransactionListener::TransactionListener()
{
}


TransactionListener::~TransactionListener()
{
}


//	#pragma mark - Transaction


status_t
Transaction::Start(Volume* volume, off_t refBlock)
{
	// has it already been started?
	if (fJournal != NULL)
		return B_OK;

	fJournal = volume->GetJournal(refBlock);
	if (fJournal != NULL && fJournal->Lock(this, false) == B_OK)
		return B_OK;

	fJournal = NULL;
	return B_ERROR;
}


void
Transaction::AddListener(TransactionListener* listener)
{
	if (fJournal == NULL)
		panic("Transaction is not running!");

	fListeners.Add(listener);
}


void
Transaction::RemoveListener(TransactionListener* listener)
{
	if (fJournal == NULL)
		panic("Transaction is not running!");

	fListeners.Remove(listener);
	listener->RemovedFromTransaction();
}


void
Transaction::NotifyListeners(bool success)
{
	while (TransactionListener* listener = fListeners.RemoveHead()) {
		listener->TransactionDone(success);
		listener->RemovedFromTransaction();
	}
}


/*!	Move the inodes into the parent transaction. This is needed only to make
	sure they will still be reverted in case the transaction is aborted.
*/
void
Transaction::MoveListenersTo(Transaction* transaction)
{
	while (TransactionListener* listener = fListeners.RemoveHead()) {
		transaction->fListeners.Add(listener);
	}
}
