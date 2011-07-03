/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "package.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern const char* __progname;
const char* kCommandName = __progname;


static const char* kUsage =
	"Usage: %s <command> <command args>\n"
	"Creates, inspects, or extracts a Haiku package.\n"
	"\n"
	"Commands:\n"
	"  add [ <options> ] <package> <entries>...\n"
	"    Adds the specified entries <entries> to package file <package>.\n"
	"\n"
	"    -C <dir>   - Change to directory <dir> before adding entries.\n"
	"    -f         - Force adding, replacing already existing entries. "
		"Without\n"
	"                 this option adding will fail when encountering a "
		"pre-exiting\n"
	"                 entry (directories will be merged, though).\n"
	"    -i <info>  - Use the package info file <info>. It will be added as\n"
	"                 \".PackageInfo\", overriding a \".PackageInfo\" file,\n"
	"                 existing.\n"
	"    -q         - Be quiet (don't show any output except for errors).\n"
	"    -v         - Be verbose (show more info about created package).\n"
	"\n"
	"  create [ <options> ] <package>\n"
	"    Creates package file <package> from contents of current directory.\n"
	"\n"
	"    -C <dir>   - Change to directory <dir> before adding entries.\n"
	"    -i <info>  - Use the package info file <info>. It will be added as\n"
	"                 \".PackageInfo\", overriding a \".PackageInfo\" file,\n"
	"                 existing.\n"
	"    -q         - Be quiet (don't show any output except for errors).\n"
	"    -v         - Be verbose (show more info about created package).\n"
	"\n"
	"  dump [ <options> ] <package>\n"
	"    Dumps the TOC section of package file <package>. For debugging only.\n"
	"\n"
	"  extract [ <options> ] <package> [ <entries>... ]\n"
	"    Extracts the contents of package file <package>. If <entries> are\n"
	"    specified, only those entries are extracted (directories "
		"recursively).\n"
	"\n"
	"    -C <dir>   - Change to directory <dir> before extracting the "
		"contents\n"
	"                  of the archive.\n"
	"    -i <info>  - Extract the .PackageInfo file to <info> instead.\n"
	"\n"
	"  list [ <options> ] <package>\n"
	"    Lists the contents of package file <package>.\n"
	"\n"
	"    -a         - Also list the file attributes.\n"
	"\n"
	"Common Options:\n"
	"  -h, --help   - Print this usage info.\n"
;


void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName);
    exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	if (argc < 2)
		print_usage_and_exit(true);

	const char* command = argv[1];
	if (strcmp(command, "add") == 0)
		return command_add(argc - 1, argv + 1);

	if (strcmp(command, "create") == 0)
		return command_create(argc - 1, argv + 1);

	if (strcmp(command, "dump") == 0)
		return command_dump(argc - 1, argv + 1);

	if (strcmp(command, "extract") == 0)
		return command_extract(argc - 1, argv + 1);

	if (strcmp(command, "list") == 0)
		return command_list(argc - 1, argv + 1);

	if (strcmp(command, "help") == 0)
		print_usage_and_exit(false);
	else
		print_usage_and_exit(true);

	// never gets here
	return 0;
}
