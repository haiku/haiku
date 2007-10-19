/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_DEVICE_UTILS_H
#define _DISK_DEVICE_UTILS_H

#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>


namespace BPrivate {


// set_string
static inline status_t
set_string(char*& location, const char* newString)
{
	char* string = NULL;
	if (newString) {
		string = strdup(newString);
		if (!string)
			return B_NO_MEMORY;
	}

	free(location);
	location = string;

	return B_OK;
}


#define SET_STRING_RETURN_ON_ERROR(location, string)	\
{														\
	status_t error = set_string(location, string);		\
	if (error != B_OK)									\
		return error;									\
}


}	// namespace BPrivate

using BPrivate::set_string;

#endif	// _DISK_DEVICE_UTILS_H
