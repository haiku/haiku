/*
 * Copyright 2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdlib.h>

#include <Application.h>


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-test-wss");

	if (argc == 1) {
		// switch forever
		while (true) {
			activate_workspace(0);
			snooze(3000);
			activate_workspace(1);
		}
		return 0;
	}

	for (int i = 1; i < argc; i++) {
		activate_workspace(strtoul(argv[i], NULL, 0));
		snooze(3000);
	}

	return 0;
}

