#ifndef JOURNAL_H
#define JOURNAL_H
/* Journal - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include <stdio.h>

#include "Volume.h"
#include "Debug.h"
#include "Utility.h"

#include "cache.h"


class TransactionListener
	: public DoublyLinkedListLinkImpl<TransactionListener> {
public:
								TransactionListener()
								{
								}

	virtual						~TransactionListener()
								{
								}

	virtual void				TransactionDone(bool success) = 0;
	virtual void				RemovedFromTransaction() = 0;
};

typedef DoublyLinkedList<TransactionListener> TransactionListeners;


class Transaction {
	public:
		Transaction(Volume *volume, off_t refBlock)
			:
			fVolume(volume),
			fID(volume->GenerateTransactionID())
		{
		}

		~Transaction()
		{
		}

		int32 ID() const
		{
			return fID;
		}

		Volume* GetVolume()
		{
			return fVolume;
		}

		status_t WriteBlocks(off_t blockNumber, const uint8* buffer,
			size_t numBlocks = 1)
		{
			return cached_write(fVolume->Device(), blockNumber, buffer,
				numBlocks);
		}


		status_t Start(Volume* volume, off_t refBlock)
		{
			return B_OK;
		}


		status_t Done()
		{
			NotifyListeners(true);
			return B_OK;
		}

		void
		AddListener(TransactionListener* listener)
		{
			fListeners.Add(listener);
		}

		void
		RemoveListener(TransactionListener* listener)
		{
			fListeners.Remove(listener);
			listener->RemovedFromTransaction();
		}

		void
		NotifyListeners(bool success)
		{
			while (TransactionListener* listener = fListeners.RemoveHead()) {
				listener->TransactionDone(success);
				listener->RemovedFromTransaction();
			}
		}

	protected:
		Volume*	fVolume;
		int32	fID;
		TransactionListeners	fListeners;
};


#endif	/* JOURNAL_H */
