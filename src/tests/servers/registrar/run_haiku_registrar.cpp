/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <image.h>
#include <Entry.h>
#include <Query.h>
#include <Path.h>
#include <Volume.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


extern const char* __progname;


int
main()
{
	team_info teamInfo;
	int32 cookie = 0;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (!strncmp(teamInfo.args, "/boot/beos/", 11)) {
			// this is a system component and not worth to investigate
			continue;
		}

		thread_info threadInfo;
		int32 threadCookie = 0;
		while (get_next_thread_info(teamInfo.team, &threadCookie, &threadInfo) == B_OK) {
			// search for the roster thread
			if (!strcmp(threadInfo.name, "_roster_thread_")) {
				port_id port = find_port("_haiku_roster_port_");
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
	query.SetPredicate("name==haiku_registrar");

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
		const char* distro = strstr(registrarPath, "distro");
		if (distro == NULL)
			continue;

		if (!strncmp(currentPath.Path(), registrarPath, distro - registrarPath)) {
			// gotcha!
			const char* args[] = { registrarPath, NULL };
			thread_id thread = load_image(1, args, (const char**)environ);
			if (thread < B_OK) {
				fprintf(stderr, "%s: Could not start the registrar: %s\n",
					__progname, strerror(thread));
				return -1;
			}

			resume_thread(thread);
			return 0;
		}
	}

	fprintf(stderr, "%s: Could not find the Haiku Registrar.\n"
		"    (This tool only works when used in the Haiku tree, but maybe\n"
		"    the registrar just needs to be built.)\n",
		__progname);
	return -1;
}

