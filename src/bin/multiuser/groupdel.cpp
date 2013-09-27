/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <getopt.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <OS.h>

#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>

#include "multiuser_utils.h"


extern const char *__progname;


static const char* kUsage =
	"Usage: %s [ <options> ] <group name>\n"
	"Deletes the specified group.\n"
	"\n"
	"Options:\n"
	"  -h, --help\n"
	"    Print usage info.\n"
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
	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "h", sLongOptions, NULL);
		if (c == -1)
			break;

	
		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind != argc - 1)
		print_usage_and_exit(true);

	const char* group = argv[optind];

	if (geteuid() != 0) {
		fprintf(stderr, "Error: Only root may delete groups.\n");
		exit(1);
	}

	if (getgrnam(group) == NULL) {
		fprintf(stderr, "Error: Group \"%s\" doesn't exists.\n", group);
		exit(1);
	}

	// prepare request for the registrar
	KMessage message(BPrivate::B_REG_DELETE_GROUP);
	if (message.AddString("name", group) != B_OK) {
		fprintf(stderr, "Error: Out of memory!\n");
		exit(1);
	}

	// send the request
	KMessage reply;
	status_t error = send_authentication_request_to_registrar(message, reply);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to delete group: %s\n", strerror(error));
		exit(1);
	}

	return 0;
}
