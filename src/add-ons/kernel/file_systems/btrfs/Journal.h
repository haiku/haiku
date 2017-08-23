#ifndef JOURNAL_H
#define JOURNAL_H


#include "Volume.h"

class Transaction;


class Journal {
public:
							Journal(Volume* volume);
							~Journal();

			Volume*			GetVolume() const { return fVolume; }
			Transaction*	CurrentTransaction() const { return fOwner; }
			uint64			SystemTransactionID() const
								{ return fCurrentGeneration; }
			int32			TransactionID() const { return fTransactionID; }
			status_t		Lock(Transaction* owner);
			status_t		UnLock(Transaction* owner, bool success);

private:
	static	void			_TransactionWritten(int32 transactionID, int32 event,
								void* _journal);
			status_t		_TransactionDone(bool success);

private:
			Volume*			fVolume;
			recursive_lock	fLock;
			Transaction*	fOwner;
			int32			fTransactionID;
			uint64			fCurrentGeneration;
};


class Transaction {
public:
							Transaction(Volume* volume);
							Transaction();
							~Transaction();

			int32			ID() const { return fJournal->TransactionID(); }
			uint64			SystemID() const
								{ return fJournal->SystemTransactionID(); }
			bool			HasBlock(fsblock_t blockNumber) const;
			Transaction*	Parent() const { return fParent; }
			void			SetParent(Transaction* parent) { fParent = parent; }
			status_t		Start(Volume* volume);
			status_t		Done();
private:
			Journal*		fJournal;
			Transaction*	fParent;
};


#endif	// JOURNAL_H
