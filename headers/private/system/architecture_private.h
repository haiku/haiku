/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCHITECTURE_PRIVATE_H
#define _SYSTEM_ARCHITECTURE_PRIVATE_H


#include <Architecture.h>


__BEGIN_DECLS


const char*	__get_architecture();
const char*	__get_primary_architecture();
size_t		__get_secondary_architectures(const char** architectures,
				size_t count);
size_t		__get_architectures(const char** architectures, size_t count);
const char*	__guess_architecture_for_path(const char* path);


__END_DECLS


#endif	/* _SYSTEM_ARCHITECTURE_PRIVATE_H */
