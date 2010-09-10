/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "Transaction.h"

#include <string.h>

#include <fs_cache.h>

#include "Journal.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


TransactionListener::TransactionListener()
{
}


TransactionListener::~TransactionListener()
{
}


Transaction::Transaction()
	:
	fJournal(NULL),
	fParent(NULL)
{
}


Transaction::Transaction(Journal* journal)
	:
	fJournal(NULL),
	fParent(NULL)
{
	Start(journal);
}


Transaction::~Transaction()
{
	if (IsStarted())
		fJournal->Unlock(this, false);
}

status_t
Transaction::Start(Journal* journal)
{
	if (IsStarted())
		return B_OK;

	fJournal = journal;
	if (fJournal == NULL)
		return B_ERROR;
	
	status_t status = fJournal->Lock(this, false);
	if (status != B_OK)
		fJournal = NULL;

	return status;
}


status_t
Transaction::Done(bool success)
{
	if (!IsStarted())
		return B_OK;

	status_t status = fJournal->Unlock(this, success);
	
	if (status == B_OK)
		fJournal = NULL;

	return status;
}


int32
Transaction::ID() const
{
	if (!IsStarted())
		return -1;

	return fJournal->TransactionID();
}


bool
Transaction::IsStarted() const
{
	return fJournal != NULL;
}


bool
Transaction::HasParent() const
{
	return fParent != NULL;
}


status_t
Transaction::WriteBlocks(off_t blockNumber, const uint8* buffer,
	size_t numBlocks)
{
	if (!IsStarted())
		return B_NO_INIT;

	void* cache = GetVolume()->BlockCache();
	size_t blockSize = GetVolume()->BlockSize();

	for (size_t i = 0; i < numBlocks; ++i) {
		void* block = block_cache_get_empty(cache, blockNumber + i, ID());
		if (block == NULL)
			return B_ERROR;
		
		memcpy(block, buffer, blockSize);
		buffer += blockSize;

		block_cache_put(cache, blockNumber + i);
	}

	return B_OK;
}


void
Transaction::Split()
{
	cache_start_sub_transaction(fJournal->GetFilesystemVolume()->BlockCache(),
		ID());
}


Volume*
Transaction::GetVolume() const
{
	if (!IsStarted())
		return NULL;

	return fJournal->GetFilesystemVolume();
}


void
Transaction::AddListener(TransactionListener* listener)
{
	TRACE("Transaction::AddListener()\n");
	if (!IsStarted())
		panic("Transaction is not running!");

	fListeners.Add(listener);
}


void
Transaction::RemoveListener(TransactionListener* listener)
{
	TRACE("Transaction::RemoveListener()\n");
	if (!IsStarted())
		panic("Transaction is not running!");

	fListeners.Remove(listener);
	listener->RemovedFromTransaction();
}


void
Transaction::NotifyListeners(bool success)
{
	TRACE("Transaction::NotifyListeners(): fListeners.First(): %p\n",
		fListeners.First());
	if (success) {
		TRACE("Transaction::NotifyListeners(true): Number of listeners: %ld\n",
			fListeners.Count());
	} else {
		TRACE("Transaction::NotifyListeners(false): Number of listeners: %ld\n",
			fListeners.Count());
	}
	TRACE("Transaction::NotifyListeners(): Finished counting\n");

	while (TransactionListener* listener = fListeners.RemoveHead()) {
		listener->TransactionDone(success);
		listener->RemovedFromTransaction();
	}
}


void
Transaction::MoveListenersTo(Transaction* transaction)
{
	TRACE("Transaction::MoveListenersTo()\n");
	while (TransactionListener* listener = fListeners.RemoveHead())
		transaction->fListeners.Add(listener);
}


void
Transaction::SetParent(Transaction* transaction)
{
	fParent = transaction;
}


Transaction*
Transaction::Parent() const
{
	return fParent;
}
