/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/manager/Exceptions.h>

#include <stdarg.h>


namespace BPackageKit {

namespace BManager {

namespace BPrivate {


// #pragma mark - BException


BException::BException()
	:
	fMessage()
{
}


BException::BException(const BString& message)
	:
	fMessage(message)
{
}


// #pragma mark - BFatalErrorException


BFatalErrorException::BFatalErrorException()
	:
	BException(),
	fDetails(),
	fError(B_OK),
	fCommitTransactionResult(),
	fCommitTransactionFailed(false)
{
}


BFatalErrorException::BFatalErrorException(const char* format, ...)
	:
	BException(),
	fDetails(),
	fError(B_OK),
	fCommitTransactionResult(),
	fCommitTransactionFailed(false)
{
	va_list args;
	va_start(args, format);
	fMessage.SetToFormatVarArgs(format, args);
	va_end(args);
}


BFatalErrorException::BFatalErrorException(status_t error, const char* format,
	...)
	:
	BException(),
	fDetails(),
	fError(error),
	fCommitTransactionResult(),
	fCommitTransactionFailed(false)
{
	va_list args;
	va_start(args, format);
	fMessage.SetToFormatVarArgs(format, args);
	va_end(args);
}


BFatalErrorException::BFatalErrorException(
	const BCommitTransactionResult& result)
	:
	BException(),
	fDetails(),
	fError(B_OK),
	fCommitTransactionResult(result),
	fCommitTransactionFailed(true)
{
	fMessage.SetToFormat("failed to commit transaction: %s",
		result.FullErrorMessage().String());
}


BFatalErrorException&
BFatalErrorException::SetDetails(const BString& details)
{
	fDetails = details;
	return *this;
}


// #pragma mark - BAbortedByUserException


BAbortedByUserException::BAbortedByUserException()
	:
	BException()
{
}


// #pragma mark - BNothingToDoException


BNothingToDoException::BNothingToDoException()
	:
	BException()
{
}


}	// namespace BPrivate

}	// namespace BManager

}	// namespace BPackageKit
