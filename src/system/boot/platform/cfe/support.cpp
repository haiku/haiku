/*
 * Copyright 2011, François Revol, revol@free.fr.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "support.h"
#include <boot/platform/cfe/cfe.h>


bigtime_t
system_time(void)
{
	bigtime_t result = cfe_getticks() * 1000000LL / CFE_HZ ;
	return result;
}

