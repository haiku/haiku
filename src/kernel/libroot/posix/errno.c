/* errno.c
 *
 * Simple file to get errno defined!!
 */

#include <errno.h>

/* XXX - Fix this once TLS works */

static int errno_storage;

int*
_errnop(void)
{
	return &errno_storage;
}

