/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_STDLIB_PRIVATE_H
#define _LIBROOT_STDLIB_PRIVATE_H


#include <sys/cdefs.h>
#include <sys/types.h>


__BEGIN_DECLS


ssize_t	__getenv_reentrant(const char* name, char* buffer,
			size_t bufferSize);


__END_DECLS


#endif	/* _LIBROOT_STDLIB_PRIVATE_H */
