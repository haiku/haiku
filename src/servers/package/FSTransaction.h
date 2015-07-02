/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_TRANSACTION_H
#define FS_TRANSACTION_H


#include <string>
#include <vector>

#include "FSUtils.h"


class FSTransaction {
public:
			typedef FSUtils::Entry Entry;

			class Operation;
			class CreateOperation;
			class RemoveOperation;
			class MoveOperation;

public:
								FSTransaction();
								~FSTransaction();

			void				RollBack();

			int32				CreateEntry(const Entry& entry,
									int32 modifiedOperation = -1);
			int32				RemoveEntry(const Entry& entry,
									const Entry& backupEntry,
									int32 modifiedOperation = -1);
			int32				MoveEntry(const Entry& fromEntry,
									const Entry& toEntry,
									int32 modifiedOperation = -1);

			void				RemoveOperationAt(int32 index);

private:
			struct OperationInfo;
			typedef std::vector<OperationInfo> OperationList;

private:
	static	std::string			_GetPath(const Entry& entry);

private:
			OperationList		fOperations;
};


class FSTransaction::Operation {
public:
	Operation(FSTransaction* transaction, int32 operation)
		:
		fTransaction(transaction),
		fOperation(operation),
		fIsFinished(false)
	{
	}

	~Operation()
	{
		if (fTransaction != NULL && fOperation >= 0 && !fIsFinished)
			fTransaction->RemoveOperationAt(fOperation);
	}

	/*!	Arms the operation rollback, i.e. rolling back the transaction will
		revert this operation.
	*/
	void Finished()
	{
		fIsFinished = true;
	}

	/*!	Unregisters the operation rollback, i.e. rolling back the transaction
		will not revert this operation.
	*/
	void Unregister()
	{
		if (fTransaction != NULL && fOperation >= 0) {
			fTransaction->RemoveOperationAt(fOperation);
			fIsFinished = false;
			fTransaction = NULL;
			fOperation = -1;
		}
	}

private:
	FSTransaction*	fTransaction;
	int32			fOperation;
	bool			fIsFinished;
};


class FSTransaction::CreateOperation : public FSTransaction::Operation {
public:
	CreateOperation(FSTransaction* transaction, const Entry& entry,
		int32 modifiedOperation = -1)
		:
		Operation(transaction,
			transaction->CreateEntry(entry, modifiedOperation))
	{
	}
};


class FSTransaction::RemoveOperation : public FSTransaction::Operation {
public:
	RemoveOperation(FSTransaction* transaction, const Entry& entry,
		const Entry& backupEntry, int32 modifiedOperation = -1)
		:
		Operation(transaction,
			transaction->RemoveEntry(entry, backupEntry, modifiedOperation))
	{
	}
};


class FSTransaction::MoveOperation : public FSTransaction::Operation {
public:
	MoveOperation(FSTransaction* transaction, const Entry& fromEntry,
		const Entry& toEntry, int32 modifiedOperation = -1)
		:
		Operation(transaction,
			transaction->MoveEntry(fromEntry, toEntry, modifiedOperation))
	{
	}
};


#endif	// FS_TRANSACTION_H
