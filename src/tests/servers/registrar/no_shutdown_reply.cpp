/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>

int
main()
{
	BApplication app("application/x-vnd.haiku.no-shutdown-reply");

	while (true)
		snooze(1000000);

	return 0;
}
