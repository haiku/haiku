/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FSTransaction.h"

#include <Entry.h>
#include <package/CommitTransactionResult.h>
#include <Path.h>

#include <CopyEngine.h>
#include <RemoveEngine.h>

#include "DebugSupport.h"
#include "Exception.h"


// #pragma mark - OperationInfo


struct FSTransaction::OperationInfo {
public:
	enum Type {
		TYPE_CREATE,
		TYPE_REMOVE,
		TYPE_MOVE,
	};

public:
	OperationInfo(Type type, const std::string& fromPath,
		const std::string& toPath, int32 modifiedOperation)
		:
		fType(type),
		fFromPath(fromPath),
		fToPath(toPath),
		fModifiedOperation(modifiedOperation),
		fEnabled(true)
	{
	}

	const std::string& FromPath() const
	{
		return fFromPath;
	}

	const std::string& ToPath() const
	{
		return fToPath;
	}

	int32 ModifiedOperation() const
	{
		return fModifiedOperation;
	}

	void SetModifiedOperation(int32 modifiedOperation)
	{
		fModifiedOperation = modifiedOperation;
	}

	bool IsEnabled() const
	{
		return fEnabled;
	}

	void SetEnabled(bool enabled)
	{
		fEnabled = enabled;
	}

	status_t RollBack() const
	{
		switch (fType) {
			case TYPE_CREATE:
			{
				status_t error = BRemoveEngine().RemoveEntry(
					Entry(fFromPath.c_str()));
				if (error != B_OK) {
					ERROR("Failed to remove \"%s\": %s\n", fFromPath.c_str(),
						strerror(error));
				}
				return error;
			}

			case TYPE_REMOVE:
			{
				if (fToPath.empty())
					return B_NOT_SUPPORTED;

				status_t error = BCopyEngine(
						BCopyEngine::COPY_RECURSIVELY
							| BCopyEngine::UNLINK_DESTINATION)
					.CopyEntry(fToPath.c_str(), fFromPath.c_str());
				if (error != B_OK) {
					ERROR("Failed to copy \"%s\" to \"%s\": %s\n",
						fToPath.c_str(), fFromPath.c_str(), strerror(error));
				}
				return error;
			}

			case TYPE_MOVE:
			{
				BEntry entry;
				status_t error = entry.SetTo(fToPath.c_str());
				if (error != B_OK) {
					ERROR("Failed to init entry for \"%s\": %s\n",
						fToPath.c_str(), strerror(error));
					return error;
				}

				error = entry.Rename(fFromPath.c_str(), true);
				if (error != B_OK) {
					ERROR("Failed to move \"%s\" to \"%s\": %s\n",
						fToPath.c_str(), fFromPath.c_str(), strerror(error));
					return error;
				}
				return error;
			}
		}

		return B_ERROR;
	}

private:
	Type		fType;
	std::string	fFromPath;
	std::string	fToPath;
	int32		fModifiedOperation;
	bool		fEnabled;
};


// #pragma mark - FSTransaction


FSTransaction::FSTransaction()
{
}


FSTransaction::~FSTransaction()
{
}


void
FSTransaction::RollBack()
{
	int32 count = (int32)fOperations.size();
	for (int32 i = count - 1; i >= 0; i--) {
		const OperationInfo& operation = fOperations[i];
		bool rolledBack = false;
		if (operation.IsEnabled())
			rolledBack = operation.RollBack() == B_OK;

		if (!rolledBack && operation.ModifiedOperation() >= 0)
			fOperations[operation.ModifiedOperation()].SetEnabled(false);
	}
}


int32
FSTransaction::CreateEntry(const Entry& entry, int32 modifiedOperation)
{
	fOperations.push_back(
		OperationInfo(OperationInfo::TYPE_CREATE, _GetPath(entry),
			std::string(), modifiedOperation));
	return (int32)fOperations.size() - 1;
}


int32
FSTransaction::RemoveEntry(const Entry& entry, const Entry& backupEntry,
	int32 modifiedOperation)
{
	fOperations.push_back(
		OperationInfo(OperationInfo::TYPE_REMOVE, _GetPath(entry),
			_GetPath(backupEntry), modifiedOperation));
	return (int32)fOperations.size() - 1;
}


int32
FSTransaction::MoveEntry(const Entry& fromEntry, const Entry& toEntry,
	int32 modifiedOperation)
{
	fOperations.push_back(
		OperationInfo(OperationInfo::TYPE_MOVE, _GetPath(fromEntry),
			_GetPath(toEntry), modifiedOperation));
	return (int32)fOperations.size() - 1;
}


void
FSTransaction::RemoveOperationAt(int32 index)
{
	int32 count = fOperations.size();
	if (index < 0 || index >= count) {
		ERROR("FSTransaction::RemoveOperationAt(): invalid "
			"operation index %" B_PRId32 "/%" B_PRId32, index, count);
		throw Exception(BPackageKit::B_TRANSACTION_INTERNAL_ERROR);
	}

	fOperations.erase(fOperations.begin() + index);

	for (int32 i = index; i < count; i++) {
		int32 modifiedOperation = fOperations[i].ModifiedOperation();
		if (modifiedOperation == index)
			fOperations[i].SetModifiedOperation(-1);
		else if (modifiedOperation > index)
			fOperations[i].SetModifiedOperation(modifiedOperation - 1);
	}
}


/*static*/ std::string
FSTransaction::_GetPath(const Entry& entry)
{
	BPath pathBuffer;
	const char* path;
	status_t error = entry.GetPath(pathBuffer, path);
	if (error == B_OK && path[0] != '/') {
		// make absolute
		error = pathBuffer.SetTo(path);
	}

	if (error != B_OK) {
		if (error == B_NO_MEMORY)
			throw Exception(BPackageKit::B_TRANSACTION_NO_MEMORY);

		throw Exception(BPackageKit::B_TRANSACTION_FAILED_TO_GET_ENTRY_PATH)
			.SetPath1(entry.PathOrName())
			.SetSystemError(error);
	}

	return path;
}
