/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <OS.h>
#include <image.h>
#include <Entry.h>
#include <Query.h>
#include <Path.h>
#include <String.h>
#include <Volume.h>
#include <fs_info.h>


extern const char* __progname;


static status_t
launch_registrar(const char* registrarPath)
{
	const char* args[] = { registrarPath, NULL };
	thread_id thread = load_image(1, args, (const char**)environ);
	if (thread < B_OK) {
		fprintf(stderr, "%s: Could not start the registrar: %s\n",
			__progname, strerror(thread));
		return (status_t)thread;
	}

	return resume_thread(thread);
}


int
main(int argc, char* argv[])
{
	team_info teamInfo;
	int32 cookie = 0;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (!strncmp(teamInfo.args, "/boot/beos/", 11)
			|| !strncmp(teamInfo.args, "/boot/system/", 13)) {
			// this is a system component and not worth to investigate
			continue;
		}

		thread_info threadInfo;
		int32 threadCookie = 0;
		while (get_next_thread_info(teamInfo.team, &threadCookie, &threadInfo)
				== B_OK) {
			// search for the roster thread
			if (!strcmp(threadInfo.name, "_roster_thread_")) {
				port_id port = find_port("haiku-test:roster");
				port_info portInfo;
				if (get_port_info(port, &portInfo) == B_OK
					&& portInfo.team == teamInfo.team) {
					puts("The Haiku Registrar is already running.");
					return 0;
				}
			}
		}
	}

	// the Haiku registrar doesn't seem to run yet, change this

	BPath currentPath(".");

	BQuery query;
	query.SetPredicate("name==test_registrar");

	// search on current volume only
	dev_t device = dev_for_path(".");
	BVolume volume(device);

	query.SetVolume(&volume);
	query.Fetch();

	entry_ref ref;
	while (query.GetNextRef(&ref) == B_OK) {
		BPath path(&ref);
		if (path.InitCheck() != B_OK)
			continue;

		const char* registrarPath = path.Path();
		const char* generatedPath = strstr(registrarPath, "generated");
		if (generatedPath == NULL)
			continue;

		if (!strncmp(currentPath.Path(), registrarPath, generatedPath - registrarPath)) {
			// gotcha!
			if (launch_registrar(registrarPath) == B_OK)
				return 0;
		}
	}

	// As a fallback (maybe the volume does not support queries for example)
	// try to find the test_registrar in the current folder...
	BString registrarPath(argv[0]);
	registrarPath.RemoveLast(__progname);
	registrarPath.Append("test_registrar");
	if (launch_registrar(registrarPath.String()) == B_OK)
		return 0;

	fprintf(stderr, "%s: Could not find the Haiku Registrar.\n"
		"    (This tool only works when used in the Haiku tree, but maybe\n"
		"    the registrar just needs to be built.)\n",
		__progname);
	return -1;
}
