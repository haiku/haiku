/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Exception.h"


using namespace BPackageKit::BPrivate;


Exception::Exception(int32 error, const char* errorMessage,
	const char* packageName)
	:
	fError(error),
	fErrorMessage(errorMessage),
	fPackageName(packageName)
{
}


BString
Exception::ToString() const
{
	const char* error;
	if (fError >= 0) {
		switch (fError) {
			case B_DAEMON_OK:
				error = "no error";
				break;
			case B_DAEMON_CHANGE_COUNT_MISMATCH:
				error = "transaction out of date";
				break;
			case B_DAEMON_BAD_REQUEST:
				error = "invalid transaction";
				break;
			case B_DAEMON_NO_SUCH_PACKAGE:
				error = "no such package";
				break;
			case B_DAEMON_PACKAGE_ALREADY_EXISTS:
				error = "package already exists";
				break;
			default:
				error = "unknown error";
				break;
		}
	} else
		error = strerror(fError);

	BString string;
	if (!fErrorMessage.IsEmpty()) {
		string = fErrorMessage;
		string << ": ";
	}

	string << error;

	if (!fPackageName.IsEmpty())
		string << ", package: \"" << fPackageName << '"';

	return string;
}
