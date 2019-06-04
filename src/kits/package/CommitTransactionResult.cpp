/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/CommitTransactionResult.h>

#include <Message.h>

//#include <package/DaemonDefs.h>


namespace BPackageKit {


// #pragma mark - BTransactionIssue


BTransactionIssue::BTransactionIssue()
	:
	fType(B_WRITABLE_FILE_TYPE_MISMATCH),
	fPackageName(),
	fPath1(),
	fPath2(),
 	fSystemError(B_OK),
 	fExitCode(0)
{
}


BTransactionIssue::BTransactionIssue(BType type, const BString& packageName,
	const BString& path1, const BString& path2, status_t systemError,
	int exitCode)
	:
	fType(type),
	fPackageName(packageName),
	fPath1(path1),
	fPath2(path2),
 	fSystemError(systemError),
 	fExitCode(exitCode)
{
}


BTransactionIssue::BTransactionIssue(const BTransactionIssue& other)
{
	*this = other;
}


BTransactionIssue::~BTransactionIssue()
{
}


BTransactionIssue::BType
BTransactionIssue::Type() const
{
	return fType;
}


const BString&
BTransactionIssue::PackageName() const
{
	return fPackageName;
}


const BString&
BTransactionIssue::Path1() const
{
	return fPath1;
}


const BString&
BTransactionIssue::Path2() const
{
	return fPath2;
}


status_t
BTransactionIssue::SystemError() const
{
	return fSystemError;
}


int
BTransactionIssue::ExitCode() const
{
	return fExitCode;
}


BString
BTransactionIssue::ToString() const
{
	const char* messageTemplate = "";
	switch (fType) {
		case B_WRITABLE_FILE_TYPE_MISMATCH:
			messageTemplate = "\"%path1%\" cannot be updated automatically,"
				" since its type doesn't match the type of \"%path2%\" which it"
				" is supposed to be updated with."
				" Please perform the update manually if needed.";
			break;
		case B_WRITABLE_FILE_NO_PACKAGE_ATTRIBUTE:
			messageTemplate = "\"%path1%\" cannot be updated automatically,"
				" since it doesn't have a SYS:PACKAGE attribute."
				" Please perform the update manually if needed.";
			break;
		case B_WRITABLE_FILE_OLD_ORIGINAL_FILE_MISSING:
			messageTemplate = "\"%path1%\" cannot be updated automatically,"
				" since \"%path2%\" which we need to compare it with is"
				" missing."
				" Please perform the update manually if needed.";
			break;
		case B_WRITABLE_FILE_OLD_ORIGINAL_FILE_TYPE_MISMATCH:
			messageTemplate = "\"%path1%\" cannot be updated automatically,"
				" since its type doesn't match the type of \"%path2%\" which we"
				" need to compare it with."
				" Please perform the update manually if needed.";
			break;
		case B_WRITABLE_FILE_COMPARISON_FAILED:
			messageTemplate = "\"%path1%\" cannot be updated automatically,"
				" since the comparison with \"%path2%\" failed: %error%."
				" Please perform the update manually if needed.";
			break;
		case B_WRITABLE_FILE_NOT_EQUAL:			// !keep old
			messageTemplate = "\"%path1%\" cannot be updated automatically,"
				" since it was changed manually from previous version"
				" \"%path2%\"."
				" Please perform the update manually if needed.";
			break;
		case B_WRITABLE_SYMLINK_COMPARISON_FAILED:	// !keep old
			messageTemplate = "Symbolic link \"%path1%\" cannot be updated"
				" automatically, since the comparison with \"%path2%\" failed:"
				" %error%."
				" Please perform the update manually if needed.";
			break;
		case B_WRITABLE_SYMLINK_NOT_EQUAL:			// !keep old
			messageTemplate = "Symbolic link \"%path1%\" cannot be updated"
				" automatically, since it was changed manually from previous"
				" version \"%path2%\"."
				" Please perform the update manually if needed.";
			break;
		case B_POST_INSTALL_SCRIPT_NOT_FOUND:
			messageTemplate = "Failed to find post-installation script "
				" \"%path1%\": %error%.";
			break;
		case B_STARTING_POST_INSTALL_SCRIPT_FAILED:
			messageTemplate = "Failed to run post-installation script "
				" \"%path1%\": %error%.";
			break;
		case B_POST_INSTALL_SCRIPT_FAILED:
			messageTemplate = "The post-installation script "
				" \"%path1%\" failed with exit code %exitCode%.";
			break;
		case B_PRE_UNINSTALL_SCRIPT_NOT_FOUND:
			messageTemplate = "Failed to find pre-uninstall script "
				" \"%path1%\": %error%.";
			break;
		case B_STARTING_PRE_UNINSTALL_SCRIPT_FAILED:
			messageTemplate = "Failed to run pre-uninstall script "
				" \"%path1%\": %error%.";
			break;
		case B_PRE_UNINSTALL_SCRIPT_FAILED:
			messageTemplate = "The pre-uninstall script "
				" \"%path1%\" failed with exit code %exitCode%.";
			break;
	}

	BString message(messageTemplate);
	message.ReplaceAll("%path1%", fPath1)
		.ReplaceAll("%path2%", fPath2)
		.ReplaceAll("%error%", strerror(fSystemError))
		.ReplaceAll("%exitCode%", BString() << fExitCode);
	return message;
}


status_t
BTransactionIssue::AddToMessage(BMessage& message) const
{
	status_t error;
	if ((error = message.AddInt32("type", (int32)fType)) != B_OK
		|| (error = message.AddString("package", fPackageName)) != B_OK
		|| (error = message.AddString("path1", fPath1)) != B_OK
		|| (error = message.AddString("path2", fPath2)) != B_OK
		|| (error = message.AddInt32("system error", (int32)fSystemError))
				!= B_OK
		|| (error = message.AddInt32("exit code", (int32)fExitCode)) != B_OK) {
			return error;
	}

	return B_OK;
}


status_t
BTransactionIssue::ExtractFromMessage(const BMessage& message)
{
	status_t error;
	int32 type;
	int32 systemError;
	int32 exitCode;
	if ((error = message.FindInt32("type", &type)) != B_OK
		|| (error = message.FindString("package", &fPackageName)) != B_OK
		|| (error = message.FindString("path1", &fPath1)) != B_OK
		|| (error = message.FindString("path2", &fPath2)) != B_OK
		|| (error = message.FindInt32("system error", &systemError)) != B_OK
		|| (error = message.FindInt32("exit code", &exitCode)) != B_OK) {
			return error;
	}

	fType = (BType)type;
	fSystemError = (status_t)systemError;
	fExitCode = (int)exitCode;

	return B_OK;
}


BTransactionIssue&
BTransactionIssue::operator=(const BTransactionIssue& other)
{
	fType = other.fType;
	fPackageName = other.fPackageName;
	fPath1 = other.fPath1;
	fPath2 = other.fPath2;
 	fSystemError = other.fSystemError;
	fExitCode = other.fExitCode;

	return *this;
}


// #pragma mark - BCommitTransactionResult


BCommitTransactionResult::BCommitTransactionResult()
	:
	fError(B_TRANSACTION_INTERNAL_ERROR),
	fSystemError(B_ERROR),
	fErrorPackage(),
	fPath1(),
	fPath2(),
	fString1(),
	fString2(),
	fOldStateDirectory(),
	fIssues(10, true)
{
}


BCommitTransactionResult::BCommitTransactionResult(BTransactionError error)
	:
	fError(error),
	fSystemError(B_ERROR),
	fErrorPackage(),
	fPath1(),
	fPath2(),
	fString1(),
	fString2(),
	fOldStateDirectory(),
	fIssues(10, true)
{
}


BCommitTransactionResult::BCommitTransactionResult(
	const BCommitTransactionResult& other)
	:
	fError(B_TRANSACTION_INTERNAL_ERROR),
	fSystemError(B_ERROR),
	fErrorPackage(),
	fPath1(),
	fPath2(),
	fString1(),
	fString2(),
	fOldStateDirectory(),
	fIssues(10, true)
{
	*this = other;
}


BCommitTransactionResult::~BCommitTransactionResult()
{
}


void
BCommitTransactionResult::Unset()
{
	fError = B_TRANSACTION_INTERNAL_ERROR;
	fSystemError = B_ERROR;
	fErrorPackage.Truncate(0);
	fPath1.Truncate(0);
	fPath2.Truncate(0);
	fString1.Truncate(0);
	fString2.Truncate(0);
	fOldStateDirectory.Truncate(0);
	fIssues.MakeEmpty();
}


int32
BCommitTransactionResult::CountIssues() const
{
	return fIssues.CountItems();
}


const BTransactionIssue*
BCommitTransactionResult::IssueAt(int32 index) const
{
	if (index < 0 || index >= CountIssues())
		return NULL;
	return fIssues.ItemAt(index);
}


bool
BCommitTransactionResult::AddIssue(const BTransactionIssue& issue)
{
	BTransactionIssue* newIssue = new(std::nothrow) BTransactionIssue(issue);
	if (newIssue == NULL || !fIssues.AddItem(newIssue)) {
		delete newIssue;
		return false;
	}
	return true;
}


BTransactionError
BCommitTransactionResult::Error() const
{
	return fError > 0 ? (BTransactionError)fError : B_TRANSACTION_OK;
}


void
BCommitTransactionResult::SetError(BTransactionError error)
{
	fError = error;
}


status_t
BCommitTransactionResult::SystemError() const
{
	return fSystemError;
}


void
BCommitTransactionResult::SetSystemError(status_t error)
{
	fSystemError = error;
}


const BString&
BCommitTransactionResult::ErrorPackage() const
{
	return fErrorPackage;
}


void
BCommitTransactionResult::SetErrorPackage(const BString& packageName)
{
	fErrorPackage = packageName;
}


BString
BCommitTransactionResult::FullErrorMessage() const
{
	if (fError == 0)
		return "no error";

	const char* messageTemplate = "";
	switch ((BTransactionError)fError) {
		case B_TRANSACTION_OK:
			messageTemplate = "Everything went fine.";
			break;
		case B_TRANSACTION_NO_MEMORY:
			messageTemplate = "Out of memory.";
			break;
		case B_TRANSACTION_INTERNAL_ERROR:
			messageTemplate = "An internal error occurred. Specifics can be"
				" found in the syslog.";
			break;
		case B_TRANSACTION_INSTALLATION_LOCATION_BUSY:
			messageTemplate = "Another package operation is already in"
				" progress.";
			break;
		case B_TRANSACTION_CHANGE_COUNT_MISMATCH:
			messageTemplate = "The transaction is out of date.";
			break;
		case B_TRANSACTION_BAD_REQUEST:
			messageTemplate = "The requested transaction is invalid.";
			break;
		case B_TRANSACTION_NO_SUCH_PACKAGE:
			messageTemplate = "No such package \"%package%\".";
			break;
		case B_TRANSACTION_PACKAGE_ALREADY_EXISTS:
			messageTemplate = "The to be activated package \"%package%\" does"
				" already exist.";
			break;
		case B_TRANSACTION_FAILED_TO_GET_ENTRY_PATH:
			if (fPath1.IsEmpty()) {
				if (fErrorPackage.IsEmpty()) {
					messageTemplate = "A file path could not be determined:"
						"%error%";
				} else {
					messageTemplate = "While processing package \"%package%\""
						" a file path could not be determined: %error%";
				}
			} else {
				if (fErrorPackage.IsEmpty()) {
					messageTemplate = "The file path for \"%path1%\" could not"
						" be determined: %error%";
				} else {
					messageTemplate = "While processing package \"%package%\""
						" the file path for \"%path1%\" could not be"
						" determined: %error%";
				}
			}
			break;
		case B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY:
			messageTemplate = "Failed to open directory \"%path1%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_CREATE_DIRECTORY:
			messageTemplate = "Failed to create directory \"%path1%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_REMOVE_DIRECTORY:
			messageTemplate = "Failed to remove directory \"%path1%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_MOVE_DIRECTORY:
			messageTemplate = "Failed to move directory \"%path1%\" to"
				" \"%path2%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_WRITE_ACTIVATION_FILE:
			messageTemplate = "Failed to write new package activation file"
				" \"%path1%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_READ_PACKAGE_FILE:
			messageTemplate = "Failed to read package file \"%path1%\":"
				" %error%";
			break;
		case B_TRANSACTION_FAILED_TO_EXTRACT_PACKAGE_FILE:
			messageTemplate = "Failed to extract \"%path1%\" from package"
				" \"%package%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_OPEN_FILE:
			messageTemplate = "Failed to open file \"%path1%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_MOVE_FILE:
			messageTemplate = "Failed to move file \"%path1%\" to \"%path2%\":"
				" %error%";
			break;
		case B_TRANSACTION_FAILED_TO_COPY_FILE:
			messageTemplate = "Failed to copy file \"%path1%\" to \"%path2%\":"
				" %error%";
			break;
		case B_TRANSACTION_FAILED_TO_WRITE_FILE_ATTRIBUTE:
			messageTemplate = "Failed to write attribute \"%string1%\" of file"
				" \"%path1%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_ACCESS_ENTRY:
			messageTemplate = "Failed to access entry \"%path1%\": %error%";
			break;
		case B_TRANSACTION_FAILED_TO_ADD_GROUP:
			messageTemplate = "Failed to add user group \"%string1%\" required"
				" by package \"%package%\".";
			break;
		case B_TRANSACTION_FAILED_TO_ADD_USER:
			messageTemplate = "Failed to add user \"%string1%\" required"
				" by package \"%package%\".";
			break;
		case B_TRANSACTION_FAILED_TO_ADD_USER_TO_GROUP:
			messageTemplate = "Failed to add user \"%string1%\" to group"
				" \"%string2%\" as required by package \"%package%\".";
			break;
		case B_TRANSACTION_FAILED_TO_CHANGE_PACKAGE_ACTIVATION:
			messageTemplate = "Failed to change the package activation in"
				" packagefs: %error%";
			break;
	}

	BString message(messageTemplate);
	message.ReplaceAll("%package%", fErrorPackage)
		.ReplaceAll("%path1%", fPath1)
		.ReplaceAll("%path2%", fPath2)
		.ReplaceAll("%string1%", fString1)
		.ReplaceAll("%string2%", fString2)
		.ReplaceAll("%error%", strerror(fSystemError));
	return message;
}


const BString&
BCommitTransactionResult::Path1() const
{
	return fPath1;
}


void
BCommitTransactionResult::SetPath1(const BString& path)
{
	fPath1 = path;
}


const BString&
BCommitTransactionResult::Path2() const
{
	return fPath2;
}


void
BCommitTransactionResult::SetPath2(const BString& path)
{
	fPath2 = path;
}


const BString&
BCommitTransactionResult::String1() const
{
	return fString1;
}


void
BCommitTransactionResult::SetString1(const BString& string)
{
	fString1 = string;
}


const BString&
BCommitTransactionResult::String2() const
{
	return fString2;
}


void
BCommitTransactionResult::SetString2(const BString& string)
{
	fString2 = string;
}


const BString&
BCommitTransactionResult::OldStateDirectory() const
{
	return fOldStateDirectory;
}


void
BCommitTransactionResult::SetOldStateDirectory(const BString& directory)
{
	fOldStateDirectory = directory;
}


status_t
BCommitTransactionResult::AddToMessage(BMessage& message) const
{
	status_t error;
	if ((error = message.AddInt32("error", (int32)fError)) != B_OK
		|| (error = message.AddInt32("system error", (int32)fSystemError))
			!= B_OK
		|| (error = message.AddString("error package", fErrorPackage)) != B_OK
		|| (error = message.AddString("path1", fPath1)) != B_OK
		|| (error = message.AddString("path2", fPath2)) != B_OK
		|| (error = message.AddString("string1", fString1)) != B_OK
		|| (error = message.AddString("string2", fString2)) != B_OK
		|| (error = message.AddString("old state", fOldStateDirectory))
				!= B_OK) {
		return error;
	}

	int32 count = fIssues.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BTransactionIssue* issue = fIssues.ItemAt(i);
		BMessage issueMessage;
		if ((error = issue->AddToMessage(issueMessage)) != B_OK
			|| (error = message.AddMessage("issues", &issueMessage)) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


status_t
BCommitTransactionResult::ExtractFromMessage(const BMessage& message)
{
	Unset();

	int32 resultError;
	int32 systemError;
	status_t error;
	if ((error = message.FindInt32("error", &resultError)) != B_OK
		|| (error = message.FindInt32("system error", &systemError)) != B_OK
		|| (error = message.FindString("error package", &fErrorPackage)) != B_OK
		|| (error = message.FindString("path1", &fPath1)) != B_OK
		|| (error = message.FindString("path2", &fPath2)) != B_OK
		|| (error = message.FindString("string1", &fString1)) != B_OK
		|| (error = message.FindString("string2", &fString2)) != B_OK
		|| (error = message.FindString("old state", &fOldStateDirectory))
				!= B_OK) {
		return error;
	}

	fError = (BTransactionError)resultError;
	fSystemError = (status_t)systemError;

	BMessage issueMessage;
	for (int32 i = 0; message.FindMessage("issues", i, &issueMessage) == B_OK;
			i++) {
		BTransactionIssue issue;
		error = issue.ExtractFromMessage(issueMessage);
		if (error != B_OK)
			return error;

		if (!AddIssue(issue))
			return B_NO_MEMORY;
	}

	return B_OK;
}


BCommitTransactionResult&
BCommitTransactionResult::operator=(const BCommitTransactionResult& other)
{
	Unset();

	fError = other.fError;
	fSystemError = other.fSystemError;
	fErrorPackage = other.fErrorPackage;
	fPath1 = other.fPath1;
	fPath2 = other.fPath2;
	fString1 = other.fString1;
	fString2 = other.fString2;
	fOldStateDirectory = other.fOldStateDirectory;

	for (int32 i = 0; const BTransactionIssue* issue = other.fIssues.ItemAt(i);
			i++) {
		AddIssue(*issue);
	}

	return *this;
}


} // namespace BPackageKit
