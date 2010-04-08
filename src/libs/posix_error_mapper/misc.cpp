/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include "posix_error_mapper.h"


static int*
real_errnop()
{
	GET_REAL_FUNCTION(int*, _errnop, (void));
	return sReal__errnop();
}


int*
_errnop(void)
{
	HIDDEN_FUNCTION(_errnop);

	// convert errno to positive error code
	int* error = real_errnop();
	if (*error < 0)
		*error = B_TO_POSITIVE_ERROR(*error);
	return error;
}



WRAPPER_FUNCTION(char*, strerror, (int errorCode),
	return sReal_strerror(B_TO_NEGATIVE_ERROR(errorCode));
)


WRAPPER_FUNCTION(int, strerror_r,
		(int errorCode, char* buffer, size_t bufferSize),
	return sReal_strerror_r(B_TO_NEGATIVE_ERROR(errorCode), buffer, bufferSize);
)


WRAPPER_FUNCTION(void, perror, (const char* errorPrefix),
	// convert errno to negative error code
	int* error = real_errnop();
	int oldError = *error;
	if (*error > 0)
		*error = B_TO_NEGATIVE_ERROR(*error);

	// call the real perror()
	sReal_perror(errorPrefix);

	// reset errno
	*error = oldError;
)
