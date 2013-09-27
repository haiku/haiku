/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__MANAGER__PRIVATE__EXCEPTIONS_H_
#define _PACKAGE__MANAGER__PRIVATE__EXCEPTIONS_H_


#include <package/Context.h>


namespace BPackageKit {

namespace BManager {

namespace BPrivate {


class BException {
public:
								BException();
								BException(const BString& message);

			const BString&		Message() const
									{ return fMessage; }

protected:
			BString				fMessage;
};


class BFatalErrorException : public BException {
public:
								BFatalErrorException();
								BFatalErrorException(const char* format, ...);
								BFatalErrorException(status_t error,
									const char* format, ...);

			const BString&		Details() const
									{ return fDetails; }
			BFatalErrorException& SetDetails(const BString& details);

			status_t			Error() const
									{ return fError; }

private:
			BString				fDetails;
			status_t			fError;
};


class BAbortedByUserException : public BException {
public:
								BAbortedByUserException();
};


class BNothingToDoException : public BException {
public:
								BNothingToDoException();
};


}	// namespace BPrivate

}	// namespace BManager

}	// namespace BPackageKit


#endif	// _PACKAGE__MANAGER__PRIVATE__EXCEPTIONS_H_
