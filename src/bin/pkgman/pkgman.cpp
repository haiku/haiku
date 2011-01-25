/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "pkgman.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern const char* __progname;
const char* kProgramName = __progname;


static const char* kUsage =
	"Usage: %s <command> <command args>\n"
	"Creates, inspects, or extracts a Haiku package.\n"
	"\n"
	"Commands:\n"
	"  add-repo <repo-base-url>\n"
	"    Adds the repository with the given <repo-base-URL>.\n"
	"\n"
	"  drop-repo <repo-name>\n"
	"    Drops the repository with the given <repo-name>.\n"
	"\n"
	"  list-repos\n"
	"    Lists all repositories.\n"
	"\n"
	"  refresh [<repo-name> ...]\n"
	"    Refreshes all or just the given repositories.\n"
	"\n"
	"Common Options:\n"
	"  -h, --help   - Print this usage info.\n"
;


void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kProgramName);
    exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	if (argc < 2)
		print_usage_and_exit(true);

	const char* command = argv[1];
	if (strncmp(command, "add-r", 5) == 0)
		return command_add_repo(argc - 1, argv + 1);

	if (strncmp(command, "drop-r", 6) == 0)
		return command_drop_repo(argc - 1, argv + 1);

	if (strncmp(command, "list-r", 6) == 0)
		return command_list_repos(argc - 1, argv + 1);

	if (strncmp(command, "refr", 4) == 0)
		return command_refresh(argc - 1, argv + 1);

//	if (strcmp(command, "search") == 0)
//		return command_search(argc - 1, argv + 1);

	if (strcmp(command, "help") == 0)
		print_usage_and_exit(false);
	else
		print_usage_and_exit(true);

	// never gets here
	return 0;
}
