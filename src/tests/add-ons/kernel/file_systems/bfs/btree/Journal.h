#ifndef JOURNAL_H
#define JOURNAL_H
/* Journal - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <stdio.h>

#include "Volume.h"
#include "Debug.h"
#include "Utility.h"

#include "cache.h"


class TransactionListener
	: public DoublyLinkedListLinkImpl<TransactionListener> {
public:
								TransactionListener();
	virtual						~TransactionListener();

	virtual void				TransactionDone(bool success) = 0;
	virtual void				RemovedFromTransaction() = 0;
};

typedef DoublyLinkedList<TransactionListener> TransactionListeners;


class Transaction {
	public:
		Transaction(Volume *volume,off_t refBlock)
			:
			fVolume(volume)
		{
		}

		~Transaction()
		{
		}

		status_t WriteBlocks(off_t blockNumber,const uint8 *buffer,size_t numBlocks = 1)
		{
			return cached_write(fVolume->Device(),blockNumber,buffer,numBlocks,fVolume->BlockSize());
		}

		void Done()
		{
		}

	protected:
		Volume	*fVolume;
};


#endif	/* JOURNAL_H */
