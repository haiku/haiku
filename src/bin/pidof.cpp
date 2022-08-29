/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oscar Lesta <oscar.lesta@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include <extended_system_info.h>
#include <extended_system_info_defs.h>
#include <util/KMessage.h>


static void
usage(FILE* output = stdout)
{
	fprintf(output, "Return the process id(s) (PID) for the given process name.\n\n"
		"Usage:\tpidof [-hsv] process_name\n"
		"\t\t-h : Show this help.\n"
		"\t\t-s : Print only the PID for the first matching process.\n"
		"\t\t-v : Verbose output.\n"
	);
}


int
main(int argc, char* argv[])
{
	bool singleShot = false;
	bool verbose = false;

	int c;
	while ((c = getopt(argc, argv, "hvs")) != -1) {
		switch (c) {
			case 'h':
				usage();
				return 0;
			case 's':
				singleShot = true;
				break;
			case 'v':
				verbose = true;
				break;
			default:
				usage(stderr);
				return 1;
		}
	}

	if ((argc == optind) || (argc - optind) > 1) {
		if (verbose)
			fprintf(stderr, "Error: A (single) process name is expected.\n");

		usage(stderr);
		return 1;
	}

	int32 cookie = 0;
	team_info teamInfo;
	bool processFound = false;
	const char* processName = argv[argc - 1];

	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		KMessage extendedInfo;
		status_t status = get_extended_team_info(teamInfo.team, B_TEAM_INFO_BASIC, extendedInfo);

		if (status == B_BAD_TEAM_ID) {
			// The team might have simply ended between the last function calls.
			continue;
		} else if (status != B_OK) {
			fprintf(stderr, "Error: get_extended_team_info() == 0x%" B_PRIx32 " (%s)\n",
				status, strerror(status));
			return 1;
		}

		const char* teamName = NULL;
		if (extendedInfo.FindString("name", &teamName) != B_OK)
			continue;

		if (strcmp(teamName, processName) == 0) {
			processFound = true;
			printf("%" B_PRId32 "\n", teamInfo.team);
			if (singleShot)
				return 0;
		}
	}

	if (verbose && !processFound)
		fprintf(stderr, "Error: Process name not found.\n");

	return processFound ? 0 : 1;
}
