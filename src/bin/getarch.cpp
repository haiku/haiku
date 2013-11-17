/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Architecture.h>
#include <Path.h>
#include <PathFinder.h>
#include <StringList.h>


extern const char* __progname;
const char* kCommandName = __progname;


static const char* kUsage =
	"Usage: %s [ <options> ] [ <path> ]\n"
	"Prints the architecture currently set via the PATH environment variable,\n"
	"when no arguments are given. When <path> is specified, the architecture\n"
	"associated with that path is printed. The options allow to print the\n"
	"primary architecture or the secondary architectures.\n"
	"\n"
	"Options:\n"
	"  -h, --help\n"
	"    Print this usage info.\n"
	"  -p, --primary\n"
	"    Print the primary architecture.\n"
	"  -s, --secondary\n"
	"    Print all secondary architectures for which support is installed.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName);
    exit(error ? 1 : 0);
}


static BString
get_current_architecture()
{
	// get the system installation location path
	BPath systemPath;
	if (find_directory(B_SYSTEM_DIRECTORY, &systemPath) != B_OK)
		return BString();

	// get all architectures
	BStringList architectures;
	get_architectures(architectures);
	if (architectures.CountStrings() < 2)
		return BString();

	// get the system bin directory for each architecture
	BStringList binDirectories;
	BPathFinder pathFinder(systemPath.Path());
	int32 architectureCount = architectures.CountStrings();
	for (int32 i = 0; i < architectureCount; i++) {
		BPath path;
		if (pathFinder.FindPath(architectures.StringAt(i),
				B_FIND_PATH_BIN_DIRECTORY, NULL, 0, path) != B_OK
			|| !binDirectories.Add(path.Path())) {
			return BString();
		}
	}

	// Get and split the PATH environmental variable value. The first system
	// bin path we encounter implies the architecture.
	char* pathVariableValue = getenv("PATH");
	BStringList paths;
	if (pathVariableValue != NULL
		&& BString(pathVariableValue).Split(":", true, paths)) {
		int32 count = paths.CountStrings();
		for (int32 i = 0; i < count; i++) {
			// normalize the path, but skip a relative one
			BPath path;
			if (paths.StringAt(i)[0] != '/'
				|| path.SetTo(paths.StringAt(i), NULL, true) != B_OK) {
				continue;
			}

			int32 index = binDirectories.IndexOf(path.Path());
			if (index >= 0)
				return architectures.StringAt(index);
		}
	}

	return BString();
}


int
main(int argc, const char* const* argv)
{
	bool printPrimary = false;
	bool printSecondary = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "primary", no_argument, 0, 'p' },
			{ "secondary", no_argument, 0, 's' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+hps",
			sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			case 'p':
				printPrimary = true;
				break;

			case 's':
				printSecondary = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining argument is the optional path.
	const char* path = optind < argc ? argv[optind++] : NULL;
	if (optind < argc)
		print_usage_and_exit(true);

	// only one of path, printPrimary, printSecondary may be specified
	if (int(path != NULL) + int(printPrimary) + int(printSecondary) > 1)
		print_usage_and_exit(true);

	if (path != NULL) {
		// architecture for given path
		printf("%s\n", guess_architecture_for_path(path));
	} else if (printPrimary) {
		// primary architecture
		printf("%s\n", get_primary_architecture());
	} else if (printSecondary) {
		// secondary architectures
		BStringList architectures;
		get_secondary_architectures(architectures);
		int32 count = architectures.CountStrings();
		for (int32 i = 0; i < count; i++)
			printf("%s\n", architectures.StringAt(i).String());
	} else {
		// current architecture as implied by PATH
		BString architecture = get_current_architecture();
		printf("%s\n",
			architecture.IsEmpty()
				? get_primary_architecture() : architecture.String());
	}

	return 0;
}
