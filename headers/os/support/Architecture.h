/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SUPPORT_ARCHITECTURE_H
#define _SUPPORT_ARCHITECTURE_H


#include <sys/cdefs.h>

#include <sys/types.h>


__BEGIN_DECLS


const char*	get_architecture();
const char*	get_primary_architecture();
size_t		get_secondary_architectures(const char** architectures,
				size_t count);
const char*	guess_architecture_for_path(const char* path);


__END_DECLS


#endif	/* _SUPPORT_ARCHITECTURE_H */
