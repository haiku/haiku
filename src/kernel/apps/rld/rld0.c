/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include "rld_priv.h"


int RLD_STARTUP(void *args)
{
#if DEBUG_RLD
	close(0); open("/dev/console", 0); /* stdin   */
	close(1); open("/dev/console", 0); /* stdout  */
	close(2); open("/dev/console", 0); /* stderr  */
#endif
	return rldmain(args);
}

