/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_UNISTD_PRIVATE_H
#define _LIBROOT_UNISTD_PRIVATE_H


#include <sys/cdefs.h>
#include <sys/times.h>


__BEGIN_DECLS


long	__sysconf_beos(int name);
long	__sysconf(int name);


__END_DECLS


#endif	// _LIBROOT_UNISTD_PRIVATE_H
