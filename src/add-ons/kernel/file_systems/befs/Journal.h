#ifndef JOURNAL_H
#define JOURNAL_H
/* Journal - transaction and logging
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>

#ifdef USER
#	include "myfs.h"
#	include <stdio.h>
#endif

#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif

extern "C" {
	#include <lock.h>
	#include <cache.h>
}

#include "Volume.h"
#include "Chain.h"
#include "Utility.h"


struct log_entry : node<log_entry> {
	uint16		start;
	uint16		length;
	uint32		cached_blocks;
	Journal		*journal;
};


class Journal {
	public:
		Journal(Volume *);
		~Journal();
		
		status_t InitCheck();

		status_t Lock(Transaction *owner);
		void Unlock(Transaction *owner,bool success);

		status_t CheckLogEntry(int32 count, off_t *array);
		status_t ReplayLogEntry(int32 *start);
		status_t ReplayLog();

		status_t WriteLogEntry();
		status_t LogBlocks(off_t blockNumber,const uint8 *buffer, size_t numBlocks);

		thread_id CurrentThread() const { return fOwningThread; }
		Transaction *CurrentTransaction() const { return fOwner; }
		uint32 TransactionSize() const { return fArray.CountItems() + fArray.BlocksUsed(); }

		status_t FlushLogAndBlocks();
		Volume *GetVolume() const { return fVolume; }

		inline int32 FreeLogBlocks() const;

	private:
		friend log_entry;

		static void blockNotify(off_t blockNumber, size_t numBlocks, void *arg);
		status_t TransactionDone(bool success);

		Volume		*fVolume;
		Benaphore	fLock;
		Transaction *fOwner;
		thread_id	fOwningThread;
		BlockArray	fArray;
		uint32		fLogSize,fMaxTransactionSize,fUsed;
		int32		fTransactionsInEntry;
		SimpleLock		fEntriesLock;
		list<log_entry>	fEntries;
		log_entry	*fCurrent;
		bool		fHasChangedBlocks;
		bigtime_t	fTimestamp;
};


inline int32 
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
		Transaction(Volume *volume,off_t refBlock)
			:
			fJournal(NULL)
		{
			Start(volume,refBlock);
		}

		Transaction(Volume *volume,block_run refRun)
			:
			fJournal(NULL)
		{
			Start(volume,volume->ToBlock(refRun));
		}

		Transaction()
			:
			fJournal(NULL)
		{
		}

		~Transaction()
		{
			if (fJournal)
				fJournal->Unlock(this,false);
		}

		status_t Start(Volume *volume,off_t refBlock);

		void Done()
		{
			if (fJournal != NULL)
				fJournal->Unlock(this,true);
			fJournal = NULL;
		}

		status_t WriteBlocks(off_t blockNumber,const uint8 *buffer,size_t numBlocks = 1)
		{
			if (fJournal == NULL)
				return B_NO_INIT;

			return fJournal->LogBlocks(blockNumber,buffer,numBlocks);
			//status_t status = cached_write/*_locked*/(fVolume->Device(),blockNumber,buffer,numBlocks,fVolume->BlockSize());
			//return status;
		}

		Volume	*GetVolume() { return fJournal != NULL ? fJournal->GetVolume() : NULL; }

	protected:
		Journal	*fJournal;
};

#endif	/* JOURNAL_H */
