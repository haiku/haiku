/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef TRANSACTION_H
#define TRANSACTION_H


#include <util/DoublyLinkedList.h>


class Journal;
class Volume;


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
									Transaction();
									Transaction(Journal* journal);
									~Transaction();

			status_t				Start(Journal* journal);
			status_t				Done(bool success = true);

			bool					IsStarted() const;
			bool					HasParent() const;

			status_t				WriteBlocks(off_t blockNumber,
										const uint8* buffer, 
										size_t numBlocks = 1);

			void					Split();

			Volume*					GetVolume() const;
			int32					ID() const;

			void					AddListener(TransactionListener* listener);
			void					RemoveListener(
										TransactionListener* listener);

			void					NotifyListeners(bool success);
			void					MoveListenersTo(Transaction* transaction);
			
			void					SetParent(Transaction* transaction);
			Transaction*			Parent() const;
private:
									Transaction(const Transaction& other);
			Transaction&			operator=(const Transaction& other);
				// no implementation

			Journal*				fJournal;
			TransactionListeners	fListeners;
			Transaction*			fParent;
};

#endif	// TRANSACTION_H
