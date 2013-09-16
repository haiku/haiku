/*
 * Copyright 2008-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <getopt.h>
#include <pwd.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <OS.h>
#include <parsedate.h>

#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>

#include <AutoDeleter.h>

#include "multiuser_utils.h"


extern const char *__progname;


static const char* kUsage =
	"Usage: %s [ <options> ] <user name>\n"
	"Creates a new user <user name>.\n"
	"\n"
	"Options:\n"
	"  -d <home>\n"
	"    Specifies the home directory for the new user.\n"
	"  -e <expiration>\n"
	"    Specifies the expiration date for the new user's account.\n"
	"  -f <inactive>\n"
	"    Specifies the number of days after the expiration of the new user's "
			"password\n"
	"    until the account expires.\n"
	"  -g <gid>\n"
	"    Specifies the new user's primary group by ID or name.\n"
	"  -h, --help\n"
	"    Print usage info.\n"
	"  -s <shell>\n"
	"    Specifies the new user's login shell.\n"
	"  -n <real name>\n"
	"    Specifies the new user's real name.\n"
	;

static void
print_usage_and_exit(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, __progname);
	exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	const char* home = "/boot/home";
	int expiration = 99999;
	int inactive = -1;
	const char* group = NULL;
	const char* shell = "/bin/sh";
	const char* realName = "";

	int min = -1;
	int max = -1;
	int warn = 7;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "d:e:f:g:hn:s:", sLongOptions,
			NULL);
		if (c == -1)
			break;

	
		switch (c) {
			case 'd':
				home = optarg;
				break;

			case 'e':
				expiration = parsedate(optarg, time(NULL)) / (3600 * 24);
				break;

			case 'f':
				inactive = atoi(optarg);
				break;

			case 'g':
			{
				group = optarg;
				break;
			}

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'n':
				realName = optarg;
				break;

			case 's':
				shell = optarg;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind != argc - 1)
		print_usage_and_exit(true);

	const char* user = argv[optind];

	if (geteuid() != 0) {
		fprintf(stderr, "Error: Only root may add users.\n");
		exit(1);
	}

	// check, if user already exists
	if (getpwnam(user) != NULL) {
		fprintf(stderr, "Error: User \"%s\" already exists.\n", user);
		exit(1);
	}

	// get group ID
	gid_t gid = 100;
	if (group != NULL) {
		char* end;
		gid = strtol(group, &end, 0);
		if (*end == '\0') {
			// seems to be a number
			if (gid < 1) {
				fprintf(stderr, "Error: Invalid group ID \"%s\".\n",
					group);
				exit(1);
			}
		} else {
			// must be a group name -- get it
			char* buffer = NULL;
			ssize_t bufferSize = sysconf(_SC_GETGR_R_SIZE_MAX);
			if (bufferSize <= 0)
				bufferSize = 256;
			for (;;) {
				buffer = (char*)realloc(buffer, bufferSize);
				if (buffer == NULL) {
					fprintf(stderr, "Error: Out of memory!\n");
					exit(1);
				}

				struct group groupBuffer;
				struct group* groupFound;
				int error = getgrnam_r(group, &groupBuffer, buffer, bufferSize,
					&groupFound);
				if (error == ERANGE) {
					bufferSize *= 2;
					continue;
				}

				if (error != 0) {
					fprintf(stderr, "Error: Failed to get info for group "
						"\"%s\".\n", group);
					exit(1);
				}
				if (groupFound == NULL) {
					fprintf(stderr, "Error: Specified group \"%s\" doesn't "
						"exist.\n", group);
					exit(1);
				}

				gid = groupFound->gr_gid;
				break;
			}
		}
	}

	// find an unused UID
	uid_t uid = 1000;
	while (getpwuid(uid) != NULL)
		uid++;

	// prepare request for the registrar
	KMessage message(BPrivate::B_REG_UPDATE_USER);
	if (message.AddInt32("uid", uid) != B_OK
		|| message.AddInt32("gid", gid) != B_OK
		|| message.AddString("name", user) != B_OK
		|| message.AddString("password", "x") != B_OK
		|| message.AddString("home", home) != B_OK
		|| message.AddString("shell", shell) != B_OK
		|| message.AddString("real name", realName) != B_OK
		|| message.AddString("shadow password", "!") != B_OK
		|| message.AddInt32("last changed", time(NULL)) != B_OK
		|| message.AddInt32("min", min) != B_OK
		|| message.AddInt32("max", max) != B_OK
		|| message.AddInt32("warn", warn) != B_OK
		|| message.AddInt32("inactive", inactive) != B_OK
		|| message.AddInt32("expiration", expiration) != B_OK
		|| message.AddInt32("flags", 0) != B_OK
		|| message.AddBool("add user", true) != B_OK) {
		fprintf(stderr, "Error: Out of memory!\n");
		exit(1);
	}

	// send the request
	KMessage reply;
	status_t error = send_authentication_request_to_registrar(message, reply);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to create user: %s\n", strerror(error));
		exit(1);
	}

	return 0;
}
