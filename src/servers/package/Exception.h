/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXCEPTION_H
#define EXCEPTION_H


#include <String.h>

#include <package/CommitTransactionResult.h>


using BPackageKit::BCommitTransactionResult;
using BPackageKit::BTransactionError;


class Exception {
public:
								Exception(BTransactionError error);

			BTransactionError	Error() const
									{ return fError; }

			status_t			SystemError() const
									{ return fSystemError; }
			Exception&			SetSystemError(status_t error);

			const BString&		PackageName() const
									{ return fPackageName; }
			Exception&			SetPackageName(const BString& packageName);

			const BString&		Path1() const
									{ return fPath1; }
			Exception&			SetPath1(const BString& path);

			const BString&		Path2() const
									{ return fPath2; }
			Exception&			SetPath2(const BString& path);

			const BString&		String1() const
									{ return fString1; }
			Exception&			SetString1(const BString& string);

			const BString&		String2() const
									{ return fString2; }
			Exception&			SetString2(const BString& string);

			void				SetOnResult(BCommitTransactionResult& result);

private:
			BTransactionError	fError;
			status_t			fSystemError;
			BString				fPackageName;
			BString				fPath1;
			BString				fPath2;
			BString				fString1;
			BString				fString2;
};


#endif	// EXCEPTION_H
