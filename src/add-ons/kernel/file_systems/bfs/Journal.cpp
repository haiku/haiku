/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */

//! Transaction and logging


#include "Journal.h"
#include "Inode.h"
#include "Debug.h"


struct run_array {
	int32		count;
	int32		max_runs;
	block_run	runs[0];

	void Init(int32 blockSize);
	void Insert(block_run &run);

	int32 CountRuns() const { return BFS_ENDIAN_TO_HOST_INT32(count); }
	int32 MaxRuns() const { return BFS_ENDIAN_TO_HOST_INT32(max_runs) - 1; }
		// that -1 accounts for an off-by-one error in Be's BFS implementation
	const block_run &RunAt(int32 i) const { return runs[i]; }

	static int32 MaxRuns(int32 blockSize)
		{ return (blockSize - sizeof(run_array)) / sizeof(block_run); }

private:
	static int _Compare(block_run &a, block_run &b);
	int32 _FindInsertionIndex(block_run &run);
};

class RunArrays {
	public:
		RunArrays(Journal *journal);
		~RunArrays();

		uint32 Length() const { return fLength; }

		status_t Insert(off_t blockNumber);

		run_array *ArrayAt(int32 i) { return fArrays.Array()[i]; }
		int32 CountArrays() const { return fArrays.CountItems(); }

		int32 MaxArrayLength();

	private:
		status_t _AddArray();
		bool _ContainsRun(block_run &run);
		bool _AddRun(block_run &run);

		Journal		*fJournal;
		uint32		fLength;
		Stack<run_array *> fArrays;
		run_array	*fLastArray;
};

class LogEntry : public DoublyLinkedListLinkImpl<LogEntry> {
	public:
		LogEntry(Journal *journal, uint32 logStart, uint32 length);
		~LogEntry();

		uint32 Start() const { return fStart; }
		uint32 Length() const { return fLength; }

		Journal *GetJournal() { return fJournal; }

	private:
		Journal		*fJournal;
		uint32		fStart;
		uint32		fLength;
};


//	#pragma mark -


static void
add_to_iovec(iovec *vecs, int32 &index, int32 max, const void *address,
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
	vecs[index].iov_base = const_cast<void *>(address);
	vecs[index].iov_len = size;
	index++;
}


//	#pragma mark - LogEntry


LogEntry::LogEntry(Journal *journal, uint32 start, uint32 length)
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
run_array::Insert(block_run &run)
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


/*static*/ int
run_array::_Compare(block_run &a, block_run &b)
{
	int cmp = a.AllocationGroup() - b.AllocationGroup();
	if (cmp == 0)
		return a.Start() - b.Start();

	return cmp;
}


int32
run_array::_FindInsertionIndex(block_run &run)
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


RunArrays::RunArrays(Journal *journal)
	:
	fJournal(journal),
	fLength(0),
	fArrays(),
	fLastArray(NULL)
{
}


RunArrays::~RunArrays()
{
	run_array *array;
	while (fArrays.Pop(&array))
		free(array);
}


bool
RunArrays::_ContainsRun(block_run &run)
{
	for (int32 i = 0; i < CountArrays(); i++) {
		run_array *array = ArrayAt(i);

		for (int32 j = 0; j < array->CountRuns(); j++) {
			block_run &arrayRun = array->runs[j];
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
RunArrays::_AddRun(block_run &run)
{
	ASSERT(run.length == 1);

	// Be's BFS log replay routine can only deal with block_runs of size 1
	// A pity, isn't it? Too sad we have to be compatible.

	if (fLastArray == NULL || fLastArray->CountRuns() == fLastArray->MaxRuns())
		return false;

	fLastArray->Insert(run);
	fLength++;
	return true;
}


status_t
RunArrays::_AddArray()
{
	int32 blockSize = fJournal->GetVolume()->BlockSize();

	run_array *array = (run_array *)malloc(blockSize);
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
	Volume *volume = fJournal->GetVolume();
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


Journal::Journal(Volume *volume)
	:
	fVolume(volume),
	fLock("bfs journal"),
	fOwner(NULL),
	fLogSize(volume->Log().Length()),
	fMaxTransactionSize(fLogSize / 2 - 5),
	fUsed(0),
	fUnwrittenTransactions(0),
	fHasSubtransaction(false)
{
}


Journal::~Journal()
{
	FlushLogAndBlocks();
}


status_t
Journal::InitCheck()
{
	// TODO: this logic won't work whenever the size of the pending transaction
	//	equals the size of the log (happens with the original BFS only)
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
Journal::_ReplayRunArray(int32 *_start)
{
	PRINT(("ReplayRunArray(start = %ld)\n", *_start));

	off_t logOffset = fVolume->ToBlock(fVolume->Log());
	off_t blockNumber = *_start % fLogSize;
	int32 blockSize = fVolume->BlockSize();
	int32 count = 1;

	CachedBlock cachedArray(fVolume);

	const run_array *array = (const run_array *)cachedArray.SetTo(logOffset
		+ blockNumber);
	if (array == NULL)
		return B_IO_ERROR;

	if (_CheckRunArray(array) < B_OK)
		return B_BAD_DATA;

	blockNumber = (blockNumber + 1) % fLogSize;

	CachedBlock cached(fVolume);
	for (int32 index = 0; index < array->CountRuns(); index++) {
		const block_run &run = array->RunAt(index);
		INFORM(("replay block run %u:%u:%u in log at %Ld!\n",
			(int)run.AllocationGroup(), run.Start(), run.Length(), blockNumber));

		off_t offset = fVolume->ToOffset(run);
		for (int32 i = 0; i < run.Length(); i++) {
			const uint8 *data = cached.SetTo(logOffset + blockNumber);
			if (data == NULL)
				RETURN_ERROR(B_IO_ERROR);

			// TODO: eventually check other well known offsets, like the
			// root and index dirs
			if (offset == 0) {
				// This log entry writes over the super block - check if
				// it's valid!
				if (Volume::CheckSuperBlock(data) != B_OK)
					RETURN_ERROR(B_BAD_DATA);
			}

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
			FATAL(("replaying log entry from %d failed: %s\n", (int)start, strerror(status)));
			return B_ERROR;
		}
		start = start % fLogSize;
	}
	
	PRINT(("replaying worked fine!\n"));
	fVolume->SuperBlock().log_start = fVolume->LogEnd();
	fVolume->LogStart() = fVolume->LogEnd();
	fVolume->SuperBlock().flags = SUPER_BLOCK_DISK_CLEAN;

	return fVolume->WriteSuperBlock();
}


/*!	This is a callback function that is called by the cache, whenever
	a block is flushed to disk that was updated as part of a transaction.
	This is necessary to keep track of completed transactions, to be
	able to update the log start pointer.
*/
void
Journal::_BlockNotify(int32 transactionID, int32 event, void *arg)
{
	LogEntry *logEntry = (LogEntry *)arg;

	if (event != TRANSACTION_WRITTEN)
		return;

	PRINT(("Log entry %p has been finished, transaction ID = %ld\n", logEntry, transactionID));

	Journal *journal = logEntry->GetJournal();
	disk_super_block &superBlock = journal->fVolume->SuperBlock();
	bool update = false;

	// Set log_start pointer if possible...

	journal->fEntriesLock.Lock();

	if (logEntry == journal->fEntries.First()) {
		LogEntry *next = journal->fEntries.GetNext(logEntry);
		if (next != NULL) {
			int32 length = next->Start() - logEntry->Start();
				// log entries inbetween could have been already released, so
				// we can't just use LogEntry::Length() here
			superBlock.log_start = superBlock.log_start + length;
		} else
			superBlock.log_start = journal->fVolume->LogEnd();

		superBlock.log_start %= journal->fLogSize;
		update = true;
	}

	journal->fUsed -= logEntry->Length();
	journal->fEntries.Remove(logEntry);
	journal->fEntriesLock.Unlock();

	delete logEntry;

	// update the super block, and change the disk's state, if necessary

	if (update) {
		journal->fVolume->LogStart() = superBlock.log_start;

		if (superBlock.log_start == superBlock.log_end)
			superBlock.flags = SUPER_BLOCK_DISK_CLEAN;

		status_t status = journal->fVolume->WriteSuperBlock();
		if (status != B_OK) {
			FATAL(("_BlockNotify: could not write back super block: %s\n",
				strerror(status)));
		}
	}
}


/*!	Writes the blocks that are part of current transaction into the log,
	and ends the current transaction.
	If the current transaction is too large to fit into the log, it will
	try to detach an existing sub-transaction.
*/
status_t
Journal::_WriteTransactionToLog()
{
	// ToDo: in case of a failure, we need a backup plan like writing all
	//	changed blocks back to disk immediately (hello disk corruption!)

	bool detached = false;

	if (_TransactionSize() > fLogSize) {
		// The current transaction won't fit into the log anymore, try to
		// detach the current sub-transaction
		if (_HasSubTransaction() && cache_blocks_in_main_transaction(
				fVolume->BlockCache(), fTransactionID) < (int32)fLogSize) {
			detached = true;
		} else {
			// TODO: what are our options here?
			// a) abort the transaction - bad, because all changes are lost
			// b) carry out the changes, but don't use the log - even worse,
			//    as it potentially creates a corrupted disk.
			dprintf("transaction too large (%d blocks, %d main, log size %d)!\n",
				(int)_TransactionSize(), (int)cache_blocks_in_main_transaction(
				fVolume->BlockCache(), fTransactionID), (int)fLogSize);
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

	if (runArrays.Length() == 0) {
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

	// Write log entries to disk

	int32 maxVecs = runArrays.MaxArrayLength() + 1;
		// one extra for the index block

	iovec *vecs = (iovec *)malloc(sizeof(iovec) * maxVecs);
	if (vecs == NULL) {
		// ToDo: write back log entries directly?
		return B_NO_MEMORY;
	}

	for (int32 k = 0; k < runArrays.CountArrays(); k++) {
		run_array *array = runArrays.ArrayAt(k);
		int32 index = 0, count = 1;
		int32 wrap = fLogSize - logStart;

		add_to_iovec(vecs, index, maxVecs, (void *)array, fVolume->BlockSize());

		// add block runs

		for (int32 i = 0; i < array->CountRuns(); i++) {
			const block_run &run = array->RunAt(i);
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
				const void *data = block_cache_get(fVolume->BlockCache(),
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
			const block_run &run = array->RunAt(i);
			off_t blockNumber = fVolume->ToBlock(run);

			for (int32 j = 0; j < run.Length(); j++) {
				block_cache_put(fVolume->BlockCache(), blockNumber + j);
			}
		}

		logStart = logPosition % fLogSize;
	}

	free(vecs);

	LogEntry *logEntry = new LogEntry(this, fVolume->LogEnd(),
		runArrays.Length());
	if (logEntry == NULL) {
		FATAL(("no memory to allocate log entries!"));
		return B_NO_MEMORY;
	}

	// Update the log end pointer in the super block

	fVolume->SuperBlock().flags = SUPER_BLOCK_DISK_DIRTY;
	fVolume->SuperBlock().log_end = logPosition;
	fVolume->LogEnd() = logPosition;

	status = fVolume->WriteSuperBlock();

	// We need to flush the drives own cache here to ensure
	// disk consistency.
	// If that call fails, we can't do anything about it anyway
	ioctl(fVolume->Device(), B_FLUSH_DRIVE_CACHE);

	// at this point, we can finally end the transaction - we're in
	// a guaranteed valid state

	fEntriesLock.Lock();
	fEntries.Add(logEntry);
	fUsed += logEntry->Length();
	fEntriesLock.Unlock();

	if (detached) {
		fTransactionID = cache_detach_sub_transaction(fVolume->BlockCache(),
			fTransactionID, _BlockNotify, logEntry);
		fUnwrittenTransactions = 1;
	} else {
		cache_end_transaction(fVolume->BlockCache(), fTransactionID,
			_BlockNotify, logEntry);
		fUnwrittenTransactions = 0;
	}

	return status;
}


status_t 
Journal::FlushLogAndBlocks()
{
	status_t status = fLock.Lock();
	if (status != B_OK)
		return status;

	if (fLock.OwnerCount() > 1) {
		// whoa, FlushLogAndBlocks() was called from inside a transaction
		fLock.Unlock();
		return B_OK;
	}

	// write the current log entry to disk

	if (fUnwrittenTransactions != 0 && _TransactionSize() != 0) {
		status = _WriteTransactionToLog();
		if (status < B_OK)
			FATAL(("writing current log entry failed: %s\n", strerror(status)));
	}

	status = fVolume->FlushDevice();

	fLock.Unlock();
	return status;
}


status_t
Journal::Lock(Transaction *owner)
{
	status_t status = fLock.Lock();
	if (status != B_OK)
		return status;

/*	ToDo:
	// if the last transaction is older than 2 secs, start a new one
	if (fTransactionsInEntry != 0 && system_time() - fTimestamp > 2000000L)
		WriteLogEntry();
*/

	if (fLock.OwnerCount() > 1) {
		// we'll just use the current transaction again
		return B_OK;
	}

	fOwner = owner;

	// ToDo: we need a way to find out how big the current transaction is;
	//	we need to be able to either detach the latest sub transaction on
	//	demand, as well as having some kind of fall back plan in case the
	//	sub transaction itself grows bigger than the log.
	//	For that, it would be nice to have some call-back interface in the
	//	cache transaction API...

	if (fUnwrittenTransactions > 0) {
		// start a sub transaction
		cache_start_sub_transaction(fVolume->BlockCache(), fTransactionID);
		fHasSubtransaction = true;
	} else
		fTransactionID = cache_start_transaction(fVolume->BlockCache());

	if (fTransactionID < B_OK) {
		fLock.Unlock();
		return fTransactionID;
	}

	return B_OK;
}


void
Journal::Unlock(Transaction *owner, bool success)
{
	if (fLock.OwnerCount() == 1) {
		// we only end the transaction if we would really unlock it
		// ToDo: what about failing transactions that do not unlock?
		_TransactionDone(success);

		fTimestamp = system_time();
		fOwner = NULL;
	}

	fLock.Unlock();
}


uint32
Journal::_TransactionSize() const
{
	int32 count = cache_blocks_in_transaction(fVolume->BlockCache(),
		fTransactionID);
	if (count < 0)
		return 0;

	return count;
}


status_t
Journal::_TransactionDone(bool success)
{
	if (!success) {
		if (_HasSubTransaction())
			cache_abort_sub_transaction(fVolume->BlockCache(), fTransactionID);
		else
			cache_abort_transaction(fVolume->BlockCache(), fTransactionID);

		return B_OK;
	}

	// If necessary, flush the log, so that we have enough space for this
	// transaction
	if (_TransactionSize() > FreeLogBlocks())
		cache_sync_transaction(fVolume->BlockCache(), fTransactionID);

	// Up to a maximum size, we will just batch several
	// transactions together to improve speed
	if (_TransactionSize() < fMaxTransactionSize) {
		fUnwrittenTransactions++;
		return B_OK;
	}

	return _WriteTransactionToLog();
}


//	#pragma mark - Transaction


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

