/* Journal - transaction and logging
 *
 * Copyright 2001-2005, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef JOURNAL_H
#define JOURNAL_H


#include <KernelExport.h>
#include <util/DoublyLinkedList.h>

#ifdef USER
#	include "myfs.h"
#	include <stdio.h>
#endif

#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif

#include "Volume.h"
#include "Chain.h"
#include "Utility.h"


struct run_array;
class LogEntry;
typedef DoublyLinkedList<LogEntry> LogEntryList;


// Locking policy in BFS: if you need both, the volume lock and the
//	journal lock, you must lock the volume first - or else you will
//	end up in a deadlock.
//	That is, if you start a transaction, and will need to lock the
//	volume while the transaction is in progress (for the unsafe
//	get_vnode() call, for example), you must lock the volume before
//	starting the transaction.

class Journal {
	public:
		Journal(Volume *);
		~Journal();
		
		status_t InitCheck();

		status_t Lock(Transaction *owner);
		void Unlock(Transaction *owner, bool success);

		status_t ReplayLog();

		status_t LogBlocks(off_t blockNumber, const uint8 *buffer, size_t numBlocks);

		Transaction *CurrentTransaction() const { return fOwner; }

		status_t FlushLogAndBlocks();
		Volume *GetVolume() const { return fVolume; }
		int32 TransactionID() const { return fTransactionID; }

		inline uint32 FreeLogBlocks() const;

	private:
		bool _HasSubTransaction() { return fHasSubtransaction; }
		uint32 _TransactionSize() const;
		status_t _WriteTransactionToLog();
		status_t _CheckRunArray(const run_array *array);
		status_t _ReplayRunArray(int32 *start);
		status_t _TransactionDone(bool success);
		static void _blockNotify(int32 transactionID, void *arg);

		Volume			*fVolume;
		RecursiveLock	fLock;
		Transaction 	*fOwner;
		uint32			fLogSize, fMaxTransactionSize, fUsed;
		int32			fUnwrittenTransactions;
		SimpleLock		fEntriesLock;
		LogEntryList	fEntries;
		bigtime_t		fTimestamp;
		int32			fTransactionID;
		bool			fHasSubtransaction;
};


inline uint32 
Journal::FreeLogBlocks() const
{
	return fVolume->LogStart() <= fVolume->LogEnd() ?
		fLogSize - fVolume->LogEnd() + fVolume->LogStart()
		: fVolume->LogStart() - fVolume->LogEnd();
}


// For now, that's only a dumb class that does more or less nothing
// else than writing the blocks directly to the real location.
// It doesn't yet use logging.

class Transaction {
	public:
		Transaction(Volume *volume, off_t refBlock)
			:
			fJournal(NULL)
		{
			Start(volume, refBlock);
		}

		Transaction(Volume *volume, block_run refRun)
			:
			fJournal(NULL)
		{
			Start(volume, volume->ToBlock(refRun));
		}

		Transaction()
			:
			fJournal(NULL)
		{
		}

		~Transaction()
		{
			if (fJournal)
				fJournal->Unlock(this, false);
		}

		status_t Start(Volume *volume, off_t refBlock);
		bool IsStarted() const { return fJournal != NULL; }

		void Done()
		{
			if (fJournal != NULL)
				fJournal->Unlock(this, true);
			fJournal = NULL;
		}

		bool HasParent()
		{
			if (fJournal != NULL)
				return fJournal->CurrentTransaction() == this;

			return false;
		}

		status_t WriteBlocks(off_t blockNumber, const uint8 *buffer, size_t numBlocks = 1)
		{
			if (fJournal == NULL)
				return B_NO_INIT;

			// ToDo: implement this properly!
			// Currently only used in BlockAllocator::StopChecking(), 
			// so chkbfs won't work correctly
#if 0
			return fJournal->LogBlocks(blockNumber, buffer, numBlocks);
#endif
			return B_ERROR;
		}

		Volume	*GetVolume() { return fJournal != NULL ? fJournal->GetVolume() : NULL; }
		int32 ID() const { return fJournal->TransactionID(); }

	private:
		Transaction(const Transaction &);
		Transaction &operator=(const Transaction &);
			// no implementation

		Journal	*fJournal;
};

#endif	/* JOURNAL_H */
