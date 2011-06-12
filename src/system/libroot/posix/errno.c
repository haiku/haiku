/*
** Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/* Provides user space storage for "errno", located in TLS
 */

#include <errno.h>

#include "support/TLS.h"
#include "tls.h"


int *
_errnop(void)
{
	return (int *)tls_address(TLS_ERRNO_SLOT);
}


// This is part of the Linuxbase binary specification
// and is referenced by some code in libgcc.a.
// ToDo: maybe we even want to include this always
#ifdef __linux__
extern int *(*__errno_location)(void) __attribute__ ((weak, alias("_errnop")));
#endif


// #pragma mark -


int
_to_positive_error(int error)
{
	if (error < 0)
		return error == B_NO_MEMORY ? -B_POSIX_ENOMEM : -error;
	return error;
}


int
_to_negative_error(int error)
{
	return error > 0 ? -error : error;
}
