/*
 * Copyright 2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>


int
main()
{
	BApplication app("application/x-vnd.Haiku-test-wss");

	while (true) {
		activate_workspace(0);
		snooze(3000);
		activate_workspace(1);
	}
}

