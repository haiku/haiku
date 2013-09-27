/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <getopt.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <set>
#include <string>

#include <OS.h>

#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>

#include "multiuser_utils.h"


extern const char *__progname;


static const char* kUsage =
	"Usage: %s [ <options> ] <group name>\n"
	"Creates a new group <group name>.\n"
	"\n"
	"Options:\n"
	"  -A, --add-user <user>\n"
	"    Add the user <user> to the group.\n"
	"  -h, --help\n"
	"    Print usage info.\n"
	"  -R, --remove-user <user>\n"
	"    Remove the user <user> from the group.\n"
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
	typedef std::set<std::string> StringSet;

	StringSet usersToAdd;
	StringSet usersToRemove;

	while (true) {
		static struct option sLongOptions[] = {
			{ "add-user", required_argument, 0, 'A' },
			{ "help", no_argument, 0, 'h' },
			{ "remove-user", required_argument, 0, 'A' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "A:hR:", sLongOptions, NULL);
		if (c == -1)
			break;

	
		switch (c) {
			case 'A':
				usersToAdd.insert(optarg);
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'R':
				usersToRemove.insert(optarg);
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
		fprintf(stderr, "Error: Only root may modify groups.\n");
		exit(1);
	}

	// get the group
	struct group* groupInfo = getgrnam(group);
	if (groupInfo == NULL) {
		fprintf(stderr, "Error: Group \"%s\" doesn't exist.\n", group);
		exit(1);
	}

	// check, if anything needs to be done
	if (usersToAdd.empty() && usersToRemove.empty()) {
		fprintf(stderr, "Error: No modification specified.\n");
		exit(1);
	}

	// prepare request for the registrar
	KMessage message(BPrivate::B_REG_UPDATE_GROUP);
	if (message.AddInt32("gid", groupInfo->gr_gid) != B_OK
		|| message.AddString("name", group) != B_OK
		|| message.AddString("password", groupInfo->gr_passwd) != B_OK
		|| message.AddBool("add group", false) != B_OK) {
		fprintf(stderr, "Error: Out of memory!\n");
		exit(1);
	}

	for (int32 i = 0; const char* user = groupInfo->gr_mem[i]; i++) {
		if (usersToRemove.erase(user) > 0)
			continue;

		usersToAdd.insert(user);
	}

	if (!usersToRemove.empty()) {
		fprintf(stderr, "Error: \"%s\" is not a member of group \"%s\"\n",
			usersToRemove.begin()->c_str(), group);
		exit(1);
	}

	// If the group doesn't have any more members, insert an empty string as an
	// indicator for the registrar to remove all members.
	if (usersToAdd.empty())
		usersToAdd.insert("");

	for (StringSet::const_iterator it = usersToAdd.begin();
		it != usersToAdd.end(); ++it) {
		if (message.AddString("members", it->c_str()) != B_OK) {
			fprintf(stderr, "Error: Out of memory!\n");
			exit(1);
		}
	}

	// send the request
	KMessage reply;
	status_t error = send_authentication_request_to_registrar(message, reply);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to create group: %s\n", strerror(error));
		exit(1);
	}

	return 0;
}
