/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_DIRENT_PRIVATE_H
#define _LIBROOT_DIRENT_PRIVATE_H

#include <sys/cdefs.h>

#include <dirent.h>


__BEGIN_DECLS

DIR*	__create_dir_struct(int fd);


__END_DECLS


#endif	// _LIBROOT_DIRENT_PRIVATE_H
