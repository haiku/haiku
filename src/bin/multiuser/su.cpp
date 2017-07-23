/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Augustin Cavalier <waddlesplash>
 */


#include <SupportDefs.h>

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "multiuser_utils.h"


extern const char* __progname;
const char* kProgramName = __progname;

const uint32 kRetries = 3;


static void
usage()
{
	fprintf(stderr, "usage: %s [-pl] [-c command] [username]\n", kProgramName);
	exit(1);
}


int
main(int argc, char *argv[])
{
	bool loginShell = false;
	const char* command = NULL;

	char c;
	while ((c = getopt(argc, argv, "c:l")) != -1) {
		switch (c) {
			case 'l':
				loginShell = true;
				break;

			case 'c':
				command = optarg;
				break;

			default:
				usage();
				break;
		}
	}

	argc -= optind;
	argv += optind;

	const char* user = NULL;
	if (argc > 0)
		user = argv[0];

	if (user == NULL)
		user = "user";
		// aka 'root' on Haiku

	// login

	openlog(kProgramName, 0, LOG_AUTH);

	status_t status = B_ERROR;
	struct passwd* passwd = NULL;

	status = authenticate_user("password: ", user, &passwd, NULL,
		kRetries, false);

	if (status < B_OK || !passwd) {
		if (passwd != NULL)
			syslog(LOG_NOTICE, "su failed for \"%s\"", passwd->pw_name);
		else
			syslog(LOG_NOTICE, "su attempt for non-existent user \"%s\"", user);
		exit(1);
	}

	// setup environment for the user

	status = setup_environment(passwd, true, false);
	if (status < B_OK) {
		// refused login
		fprintf(stderr, "%s: Refused login. Setting up environment failed: %s\n",
			kProgramName, strerror(status));
		syslog(LOG_NOTICE, "su refused for \"%s\"", passwd->pw_name);
		exit(1);
	}

	syslog(LOG_INFO, "su as \"%s\"", passwd->pw_name);

	// start shell
	const char* args[] = {getenv("SHELL"), NULL, NULL, NULL, NULL};
	int nextarg = 1;
	if (loginShell) {
		args[nextarg++] = "-login";
	}
	if (command != NULL) {
		args[nextarg++] = "-c";
		args[nextarg++] = command;
	}

	execv(args[0], (char **)args);

	// try default shell
	args[0] = "/bin/sh";
	execv(args[0], (char **)args);

	fprintf(stderr, "%s: starting the shell failed: %s", kProgramName,
		strerror(errno));

	return 1;
}

