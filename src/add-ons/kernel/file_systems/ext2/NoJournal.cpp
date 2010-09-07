/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "NoJournal.h"

#include <string.h>

#include <fs_cache.h>


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


NoJournal::NoJournal(Volume* volume)
	:
	Journal()
{
	fFilesystemVolume = volume;
	fFilesystemBlockCache = volume->BlockCache();
	fJournalVolume = volume;
	fHasSubTransaction = false;
	fSeparateSubTransactions = false;
}


NoJournal::~NoJournal()
{
}


status_t
NoJournal::InitCheck()
{
	return B_OK;
}


status_t
NoJournal::Recover()
{
	return B_OK;
}


status_t
NoJournal::StartLog()
{
	return B_OK;
}


status_t
NoJournal::Lock(Transaction* owner, bool separateSubTransactions)
{
	status_t status = block_cache_sync(fFilesystemBlockCache);
	TRACE("NoJournal::Lock(): block_cache_sync: %s\n", strerror(status));
	
	if (status == B_OK)
		status = Journal::Lock(owner, separateSubTransactions);
	
	return status;
}


status_t
NoJournal::Unlock(Transaction* owner, bool success)
{
	TRACE("NoJournal::Unlock\n");
	return Journal::Unlock(owner, success);
}


status_t
NoJournal::_WriteTransactionToLog()
{
	TRACE("NoJournal::_WriteTransactionToLog(): Ending transaction %ld\n",
		fTransactionID);

	fTransactionID = cache_end_transaction(fFilesystemBlockCache,
		fTransactionID, _TransactionWritten, NULL);
	
	return B_OK;
}


/*static*/ void
NoJournal::_TransactionWritten(int32 transactionID, int32 event, void* param)
{
	TRACE("Transaction %ld checkpointed\n", transactionID);
}
