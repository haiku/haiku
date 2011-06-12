/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_TIMES_PRIVATE_H
#define _LIBROOT_TIMES_PRIVATE_H


#include <sys/cdefs.h>
#include <sys/times.h>


__BEGIN_DECLS


clock_t __times_beos(struct tms* tms);
clock_t __times(struct tms* tms);


__END_DECLS


#endif	// _LIBROOT_TIMES_PRIVATE_H
