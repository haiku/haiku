/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include "openfirmware.h"

bigtime_t
system_time(void)
{
	int result = of_milliseconds();
	return (result == OF_FAILED ? 0 : bigtime_t(result) * 1000);
}
