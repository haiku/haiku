/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_FIND_DIRECTORY_PRIVATE_H
#define _SYSTEM_FIND_DIRECTORY_PRIVATE_H


#include <sys/cdefs.h>

#include <FindDirectory.h>


__BEGIN_DECLS


status_t __find_directory(directory_which which, dev_t device, bool createIt,
	char *returnedPath, int32 pathLength);


__END_DECLS


#endif	/* _SYSTEM_FIND_DIRECTORY_PRIVATE_H */
