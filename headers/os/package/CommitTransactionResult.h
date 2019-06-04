/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__COMMIT_TRANSACTION_RESULT_H_
#define _PACKAGE__COMMIT_TRANSACTION_RESULT_H_


#include <ObjectList.h>
#include <String.h>


class BMessage;


namespace BPackageKit {


enum BTransactionError {
	B_TRANSACTION_OK = 0,
	B_TRANSACTION_NO_MEMORY,
	B_TRANSACTION_INTERNAL_ERROR,
	B_TRANSACTION_INSTALLATION_LOCATION_BUSY,
	B_TRANSACTION_CHANGE_COUNT_MISMATCH,
	B_TRANSACTION_BAD_REQUEST,
	B_TRANSACTION_NO_SUCH_PACKAGE,
	B_TRANSACTION_PACKAGE_ALREADY_EXISTS,
	B_TRANSACTION_FAILED_TO_GET_ENTRY_PATH,
	B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY,
	B_TRANSACTION_FAILED_TO_CREATE_DIRECTORY,
	B_TRANSACTION_FAILED_TO_REMOVE_DIRECTORY,
	B_TRANSACTION_FAILED_TO_MOVE_DIRECTORY,
	B_TRANSACTION_FAILED_TO_WRITE_ACTIVATION_FILE,
	B_TRANSACTION_FAILED_TO_READ_PACKAGE_FILE,
	B_TRANSACTION_FAILED_TO_EXTRACT_PACKAGE_FILE,
	B_TRANSACTION_FAILED_TO_OPEN_FILE,
	B_TRANSACTION_FAILED_TO_MOVE_FILE,
	B_TRANSACTION_FAILED_TO_COPY_FILE,
	B_TRANSACTION_FAILED_TO_WRITE_FILE_ATTRIBUTE,
	B_TRANSACTION_FAILED_TO_ACCESS_ENTRY,
	B_TRANSACTION_FAILED_TO_ADD_GROUP,
	B_TRANSACTION_FAILED_TO_ADD_USER,
	B_TRANSACTION_FAILED_TO_ADD_USER_TO_GROUP,
	B_TRANSACTION_FAILED_TO_CHANGE_PACKAGE_ACTIVATION,
};


class BTransactionIssue {
public:
			enum BType {
				B_WRITABLE_FILE_TYPE_MISMATCH,
				B_WRITABLE_FILE_NO_PACKAGE_ATTRIBUTE,
				B_WRITABLE_FILE_OLD_ORIGINAL_FILE_MISSING,
				B_WRITABLE_FILE_OLD_ORIGINAL_FILE_TYPE_MISMATCH,
				B_WRITABLE_FILE_COMPARISON_FAILED,
				B_WRITABLE_FILE_NOT_EQUAL,
				B_WRITABLE_SYMLINK_COMPARISON_FAILED,
				B_WRITABLE_SYMLINK_NOT_EQUAL,
				B_POST_INSTALL_SCRIPT_NOT_FOUND,
				B_STARTING_POST_INSTALL_SCRIPT_FAILED,
				B_POST_INSTALL_SCRIPT_FAILED,
				B_PRE_UNINSTALL_SCRIPT_NOT_FOUND,
				B_STARTING_PRE_UNINSTALL_SCRIPT_FAILED,
				B_PRE_UNINSTALL_SCRIPT_FAILED,
			};

public:
								BTransactionIssue();
								BTransactionIssue(BType type,
									const BString& packageName,
									const BString& path1, const BString& path2,
 									status_t systemError, int exitCode);
								BTransactionIssue(
									const BTransactionIssue& other);
								~BTransactionIssue();

			BType				Type() const;
			const BString&		PackageName() const;
			const BString&		Path1() const;
			const BString&		Path2() const;
			status_t			SystemError() const;
			int					ExitCode() const;

			BString				ToString() const;

			status_t			AddToMessage(BMessage& message) const;
			status_t			ExtractFromMessage(const BMessage& message);

			BTransactionIssue&	operator=(const BTransactionIssue& other);

private:
			BType				fType;
			BString				fPackageName;
			BString				fPath1;
			BString				fPath2;
 			status_t			fSystemError;
			int					fExitCode;
};


class BCommitTransactionResult {
public:
								BCommitTransactionResult();
								BCommitTransactionResult(
									BTransactionError error);
								BCommitTransactionResult(
									const BCommitTransactionResult& other);
								~BCommitTransactionResult();

			void				Unset();

			int32				CountIssues() const;
			const BTransactionIssue* IssueAt(int32 index) const;
			bool				AddIssue(const BTransactionIssue& issue);

			BTransactionError	Error() const;
			void				SetError(BTransactionError error);

			status_t			SystemError() const;
			void				SetSystemError(status_t error);

			const BString&		ErrorPackage() const;
									// may be empty, even on error
			void				SetErrorPackage(const BString& packageName);

			BString				FullErrorMessage() const;

			const BString&		Path1() const;
			void				SetPath1(const BString& path);

			const BString&		Path2() const;
			void				SetPath2(const BString& path);

			const BString&		Path3() const;
			void				SetPath3(const BString& path);

			const BString&		String1() const;
			void				SetString1(const BString& string);

			const BString&		String2() const;
			void				SetString2(const BString& string);

			const BString&		OldStateDirectory() const;
			void				SetOldStateDirectory(const BString& directory);

			status_t			AddToMessage(BMessage& message) const;
			status_t			ExtractFromMessage(const BMessage& message);

			BCommitTransactionResult& operator=(
									const BCommitTransactionResult& other);

private:
			typedef BObjectList<BTransactionIssue> IssueList;

private:
			BTransactionError	fError;
			status_t			fSystemError;
			BString				fErrorPackage;
			BString				fPath1;
			BString				fPath2;
			BString				fString1;
			BString				fString2;
			BString				fOldStateDirectory;
			IssueList			fIssues;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__COMMIT_TRANSACTION_RESULT_H_
