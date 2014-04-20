/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <package/PackageResolvableExpression.h>
#include <Path.h>
#include <PathFinder.h>
#include <StringList.h>


extern const char* __progname;
const char* kCommandName = __progname;


struct DirectoryConstantEntry {
	const char*			string;
	path_base_directory	constant;
	const char*			description;
};

#define DEFINE_CONSTANT(constant, description)	\
	{ #constant, constant, description }

static const DirectoryConstantEntry kDirectoryConstants[] = {
	DEFINE_CONSTANT(B_FIND_PATH_INSTALLATION_LOCATION_DIRECTORY,
		"the installation location"),
	DEFINE_CONSTANT(B_FIND_PATH_ADD_ONS_DIRECTORY,
		"the add-ons directory"),
	DEFINE_CONSTANT(B_FIND_PATH_APPS_DIRECTORY,
		"the applications directory"),
	DEFINE_CONSTANT(B_FIND_PATH_BIN_DIRECTORY,
		"the command line programs directory"),
	DEFINE_CONSTANT(B_FIND_PATH_BOOT_DIRECTORY,
		"the boot data directory"),
	DEFINE_CONSTANT(B_FIND_PATH_CACHE_DIRECTORY,
		"the cache directory"),
	DEFINE_CONSTANT(B_FIND_PATH_DATA_DIRECTORY,
		"the data directory"),
	DEFINE_CONSTANT(B_FIND_PATH_DEVELOP_DIRECTORY,
		"the develop directory"),
	DEFINE_CONSTANT(B_FIND_PATH_DEVELOP_LIB_DIRECTORY,
		"the development libraries directory"),
	DEFINE_CONSTANT(B_FIND_PATH_DOCUMENTATION_DIRECTORY,
		"the documentation directory"),
	DEFINE_CONSTANT(B_FIND_PATH_ETC_DIRECTORY,
		"the Unix etc directory (global settings)"),
	DEFINE_CONSTANT(B_FIND_PATH_FONTS_DIRECTORY,
		"the fonts directory"),
	DEFINE_CONSTANT(B_FIND_PATH_HEADERS_DIRECTORY,
		"the development headers directory"),
	DEFINE_CONSTANT(B_FIND_PATH_LIB_DIRECTORY,
		"the libraries directory"),
	DEFINE_CONSTANT(B_FIND_PATH_LOG_DIRECTORY,
		"the logging directory"),
	DEFINE_CONSTANT(B_FIND_PATH_MEDIA_NODES_DIRECTORY,
		"the media node add-ons directory"),
	DEFINE_CONSTANT(B_FIND_PATH_PACKAGES_DIRECTORY,
		"the packages directory"),
	DEFINE_CONSTANT(B_FIND_PATH_PREFERENCES_DIRECTORY,
		"the preference applications directory"),
	DEFINE_CONSTANT(B_FIND_PATH_SERVERS_DIRECTORY,
		"the server programs directory"),
	DEFINE_CONSTANT(B_FIND_PATH_SETTINGS_DIRECTORY,
		"the global settings directory"),
	DEFINE_CONSTANT(B_FIND_PATH_SOUNDS_DIRECTORY,
		"the sound files directory"),
	DEFINE_CONSTANT(B_FIND_PATH_SPOOL_DIRECTORY,
		"the (mail) spool directory"),
	DEFINE_CONSTANT(B_FIND_PATH_TRANSLATORS_DIRECTORY,
		"the translator add-ons directory"),
	DEFINE_CONSTANT(B_FIND_PATH_VAR_DIRECTORY,
		"the Unix var directory (global writable data)"),
	DEFINE_CONSTANT(B_FIND_PATH_PACKAGE_PATH,
		"the path of the package the file specified via -p belongs to"),
};

static const size_t kDirectoryConstantCount
	= sizeof(kDirectoryConstants) / sizeof(kDirectoryConstants[0]);


static const char* kUsage =
	"Usage: %s [ <options> ] [ <kind> [<subpath>] ]\n"
	"Prints paths specified by directory constant <kind>. <subpath>, if\n"
	"specified, is appended to each path. By default a path is printed for\n"
	"each existing installation location (one per line); the options can\n"
	"modify this behavior.\n"
	"\n"
	"Options:\n"
	"  -a <architecture>\n"
	"    If the path(s) specified by <kind> are architecture specific, use\n"
	"    architecture <architecture>. If not specified, the primary\n"
	"    architecture is used, unless the -p/--path option is specified, in\n"
	"    which case the architecture associated with the given <path> is\n"
	"    used.\n"
	"  -c <separator>\n"
	"    Concatenate the resulting paths, separated only by <separator>,\n"
	"    instead of printing a path per line.\n"
	"  -d, --dependency <dependency>\n"
	"    Modifies the behavior of the -p option. Use the installation "
		"location\n"
	"    where the dependency <dependency> of the package that the entry\n"
	"    referred to by <path> belongs to is installed.\n"
	"  -e, --existing\n"
	"    Print only paths that refer to existing entries.\n"
	"  -h, --help\n"
	"    Print this usage info.\n"
	"  -l, --list\n"
	"    Print a list of the possible constants for the <kind> parameter.\n"
	"  -p, --path <path>\n"
	"    Print only one path, the one for the installation location that\n"
	"    contains the path <path>.\n"
	"  -r, --resolvable <expression>\n"
	"    Print only one path, the one for the installation location for the\n"
	"    package providing the resolvable matching the expression\n"
	"    <expression>. The expressions can be a simple resolvable name or\n"
	"    a resolvable name with operator and version (e.g.\n"
	"    \"cmd:perl >= 5\"; must be one argument).\n"
	"  -R, --reverse\n"
	"    Print paths in reverse order, i.e. from most general to most\n"
	"    specific.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName);
    exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	const char* architecture = NULL;
	const char* dependency = NULL;
	const char* referencePath = NULL;
	const char* resolvable = NULL;
	bool existingOnly = false;
	bool reverseOrder = false;
	const char* separator = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "architecture", required_argument, 0, 'a' },
			{ "dependency", required_argument, 0, 'd' },
			{ "help", no_argument, 0, 'h' },
			{ "list", no_argument, 0, 'l' },
			{ "path", required_argument, 0, 'p' },
			{ "resolvable", required_argument, 0, 'pr' },
			{ "reverse", no_argument, 0, 'R' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+a:c:d:ehlp:r:R",
			sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				architecture = optarg;
				break;

			case 'c':
				separator = optarg;
				break;

			case 'd':
				dependency = optarg;
				break;

			case 'e':
				existingOnly = true;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'l':
				for (size_t i = 0; i < kDirectoryConstantCount; i++) {
					const DirectoryConstantEntry& entry
						= kDirectoryConstants[i];
					printf("%s\n    - %s\n", entry.string, entry.description);
				}
				exit(0);

			case 'p':
				referencePath = optarg;
				break;

			case 'r':
				resolvable = optarg;
				break;

			case 'R':
				reverseOrder = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining arguments are the kind constant and optionally the subpath.
	if (optind >= argc || optind + 2 < argc)
		print_usage_and_exit(true);

	const char* kindConstant = argv[optind++];

	const char* subPath = NULL;
	if (optind < argc)
		subPath = argv[optind++];

	// only one of path or resolvable may be specified
	if (referencePath != NULL && resolvable != NULL)
		print_usage_and_exit(true);

	// resolve the directory constant
	path_base_directory baseDirectory = B_FIND_PATH_IMAGE_PATH;
	bool found = false;
	for (size_t i = 0; i < kDirectoryConstantCount; i++) {
		const DirectoryConstantEntry& entry = kDirectoryConstants[i];
		if (strcmp(kindConstant, entry.string) == 0) {
			found = true;
			baseDirectory = entry.constant;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "Error: Unsupported directory constant \"%s\".\n",
			kindConstant);
		exit(1);
	}

	if (referencePath != NULL || resolvable != NULL) {
		BPathFinder pathFinder;
		if (referencePath != NULL) {
			pathFinder.SetTo(referencePath, dependency);
		} else {
			pathFinder.SetTo(
				BPackageKit::BPackageResolvableExpression(resolvable),
				dependency);
		}

		BPath path;
		status_t error = pathFinder.FindPath(architecture, baseDirectory,
			subPath, existingOnly ? B_FIND_PATH_EXISTING_ONLY : 0, path);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to find path: %s\n",
				strerror(error));
			exit(1);
		}

		printf("%s\n", path.Path());
	} else {
		BStringList paths;
		status_t error = BPathFinder::FindPaths(architecture, baseDirectory,
			subPath, existingOnly ? B_FIND_PATH_EXISTING_ONLY : 0, paths);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to find paths: %s\n",
				strerror(error));
			exit(1);
		}

		int32 count = paths.CountStrings();
		if (reverseOrder) {
			for (int32 i = 0; i < count / 2; i++)
				paths.Swap(i, count - i - 1);
		}

		if (separator != NULL) {
			BString result = paths.Join(separator);
			if (result.IsEmpty()) {
				fprintf(stderr, "Error: Out of memory!\n");
				exit(1);
			}
			printf("%s\n", result.String());
		} else {
			for (int32 i = 0; i < count; i++)
				printf("%s\n", paths.StringAt(i).String());
		}
	}

	return 0;
}
