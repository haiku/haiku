/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXCEPTION_H
#define EXCEPTION_H


#include <String.h>

#include <package/DaemonDefs.h>


class Exception {
public:
								Exception(int32 error,
									const char* errorMessage = NULL,
									const char* packageName = NULL);

			int32				Error() const
									{ return fError; }

			const BString&		ErrorMessage() const
									{ return fErrorMessage; }

			const BString&		PackageName() const
									{ return fPackageName; }

			BString				ToString() const;

private:
			int32				fError;
			BString				fErrorMessage;
			BString				fPackageName;
};


#endif	// EXCEPTION_H
