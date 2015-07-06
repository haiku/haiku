/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>

#include <LaunchRoster.h>


int
main()
{
	if (getuid() != 0)
		return EXIT_FAILURE;

	// TODO: This will obviously be done differently in a multi-user
	// environment; we'll probably want to merge this with the standard
	// login application then.
	struct passwd* passwd = getpwuid(0);
	if (passwd == NULL)
		return EXIT_FAILURE;

	status_t status = BLaunchRoster().StartSession(passwd->pw_name);
	if (status != B_OK)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
