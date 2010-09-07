/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef NOJOURNAL_H
#define NOJOURNAL_H


#include "Journal.h"


class NoJournal : public Journal {
public:
						NoJournal(Volume* volume);
						~NoJournal();

			status_t	InitCheck();
			status_t	Recover();
			status_t	StartLog();
			
			status_t	Lock(Transaction* owner, bool separateSubTransactions);
			status_t	Unlock(Transaction* owner, bool success);

private:
			status_t	_WriteTransactionToLog();

	static	void		_TransactionWritten(int32 transactionID,
							int32 event, void* param);
};

#endif	// NOJOURNAL_H
