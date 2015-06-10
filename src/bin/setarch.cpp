/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <getopt.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Architecture.h>
#include <Path.h>
#include <PathFinder.h>
#include <StringList.h>


extern const char* __progname;
const char* kCommandName = __progname;


static const char* kUsage =
	"Usage: %s [-hl]\n"
	"       %s [-p] <architecture> [ <command> ... ]\n"
	"Executes the given command or, by default, a shell with a PATH\n"
	"environment variable modified such that commands for the given\n"
	"architecture will be preferred, respectively used exclusively in case of\n"
	"the primary architecture.\n"
	"\n"
	"Options:\n"
	"  -h, --help\n"
	"    Print this usage info.\n"
	"  -l, --list-architectures\n"
	"    List all architectures.\n"
	"  -p, --print-path\n"
	"    Only print the modified PATH variable value; don't execute any\n"
	"    command.\n"
;


static void
print_usage_and_exit(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, kCommandName, kCommandName);
	exit(error ? 1 : 0);
}


static bool
is_primary_architecture(const char* architecture)
{
	return strcmp(architecture, get_primary_architecture()) == 0;
}


static void
get_bin_directories(const char* architecture, BStringList& _directories)
{
	status_t error = BPathFinder::FindPaths(architecture,
		B_FIND_PATH_BIN_DIRECTORY, NULL, 0, _directories);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to get bin directories for architecture "
			"%s: %s\n", architecture, strerror(error));
		exit(1);
	}
}


static void
compute_new_paths(const char* architecture, BStringList& _paths)
{
	// get the primary architecture bin paths
	BStringList primaryBinDirectories;
	get_bin_directories(get_primary_architecture(), primaryBinDirectories);

	// get the bin paths to insert
	BStringList binDirectoriesToInsert;
	if (!is_primary_architecture(architecture))
		get_bin_directories(architecture, binDirectoriesToInsert);

	// split the PATH variable
	char* pathVariableValue = getenv("PATH");
	BStringList paths;
	if (pathVariableValue != NULL
		&& !BString(pathVariableValue).Split(":", true, paths)) {
		fprintf(stderr, "Error: Out of memory!\n");
		exit(1);
	}

	// Filter the paths, removing any path that isn't associated with the
	// primary architecture. Also find the insertion index for the architecture
	// bin paths.
	int32 insertionIndex = -1;
	int32 count = paths.CountStrings();
	for (int32 i = 0; i < count; i++) {
		// We always keep relative paths. Filter absolute ones only.
		const char* path = paths.StringAt(i);
		if (path[0] == '/') {
			// try to normalize the path
			BPath normalizedPath;
			if (normalizedPath.SetTo(path, NULL, true) == B_OK)
				path = normalizedPath.Path();

			// Check, if this is a primary bin directory. If not, determine the
			// path's architecture.
			int32 index = primaryBinDirectories.IndexOf(path);
			if (index >= 0) {
				if (insertionIndex < 0)
					insertionIndex = _paths.CountStrings();
			} else if (!is_primary_architecture(
					guess_architecture_for_path(path))) {
				// a non-primary architecture path -- skip
				continue;
			}
		}

		if (!_paths.Add(paths.StringAt(i))) {
			fprintf(stderr, "Error: Out of memory!\n");
			exit(1);
		}
	}

	// Insert the paths for the specified architecture, if any.
	if (!binDirectoriesToInsert.IsEmpty()) {
		if (!(insertionIndex < 0
				? _paths.Add(binDirectoriesToInsert)
				: _paths.Add(binDirectoriesToInsert, insertionIndex))) {
			fprintf(stderr, "Error: Out of memory!\n");
			exit(1);
		}
	}
}


int
main(int argc, const char* const* argv)
{
	bool printPath = false;
	bool listArchitectures = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "list-architectures", no_argument, 0, 'l' },
			{ "print-path", no_argument, 0, 'p' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+hlp",
			sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			case 'l':
				listArchitectures = true;
				break;

			case 'p':
				printPath = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// only one of listArchitectures, printPath may be specified
	if (listArchitectures && printPath)
		print_usage_and_exit(true);

	// get architectures
	BStringList architectures;
	status_t error = get_architectures(architectures);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to get architectures: %s\n",
			strerror(error));
		exit(1);
	}

	// list architectures
	if (listArchitectures) {
		if (optind != argc)
			print_usage_and_exit(true);

		int32 count = architectures.CountStrings();
		for (int32 i = 0; i < count; i++)
			printf("%s\n", architectures.StringAt(i).String());
		return 0;
	}

	// The remaining arguments are the architecture and optionally the command
	// to execute.
	if (optind >= argc)
		print_usage_and_exit(true);
	const char* architecture = optind < argc ? argv[optind++] : NULL;

	int commandArgCount = argc - optind;
	const char* const* commandArgs = commandArgCount > 0 ? argv + optind : NULL;

	if (printPath && commandArgs != NULL)
		print_usage_and_exit(true);

	// check the architecture
	if (!architectures.HasString(architecture)) {
		fprintf(stderr, "Error: Unsupported architecture \"%s\"\n",
			architecture);
		exit(1);
	}

	// get the new paths
	BStringList paths;
	compute_new_paths(architecture, paths);

	BString pathVariableValue = paths.Join(":");
	if (!paths.IsEmpty() && pathVariableValue.IsEmpty())
		fprintf(stderr, "Error: Out of memory!\n");

	if (printPath) {
		printf("%s\n", pathVariableValue.String());
		return 0;
	}

	// set PATH
	if (setenv("PATH", pathVariableValue, 1) != 0) {
		fprintf(stderr, "Error: Failed to set PATH: %s\n", strerror(errno));
		exit(1);
	}

	// if no command is given, get the user's shell
	const char* shellCommand[3];
	if (commandArgs == NULL) {
		struct passwd* pwd = getpwuid(geteuid());
		shellCommand[0] = pwd != NULL ? pwd->pw_shell : "/bin/sh";
		shellCommand[1] = "-l";
		shellCommand[2] = NULL;
		commandArgs = shellCommand;
		commandArgCount = 2;
	}

	// exec the command
	execvp(commandArgs[0], (char* const*)commandArgs);

	fprintf(stderr, "Error: Executing \"%s\" failed: %s\n", commandArgs[0],
		strerror(errno));
	return 1;
}
