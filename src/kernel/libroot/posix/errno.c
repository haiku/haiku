/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
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

