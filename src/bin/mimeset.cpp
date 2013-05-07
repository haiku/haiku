/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Mime.h>


#ifdef HAIKU_HOST_PLATFORM_SUNOS
static const char* sProgramName = "mimeset";
#else
extern const char* __progname;
static const char* sProgramName = __progname;
#endif

// options
bool gFiles = true;
bool gApps = false;
int gForce = B_UPDATE_MIME_INFO_NO_FORCE;


static void
usage(int status)
{
	printf("Usage: %s <options> <path> ...\n"
		"Recursively updates the MIME related attributes (e.g. file type) for\n"
		"the given files. Alternatively or additionally encountered\n"
		"applications are entered into the MIME database. When \"@\" is\n"
		"specified as <path>, file paths are read from stdin.\n"
		"\n"
		"Options:\n"
		"  -A, --all\n"
		"    Update the files' MIME information and enter applications into\n"
		"    the MIME database.\n"
		"  -a, --apps\n"
		"    Only enter applications into the MIME database.\n"
		"  -f\n"
		"    Force updating, even if previously updated, but do not overwrite\n"
		"    the type of a file.\n"
		"  -F\n"
		"    Force updating, even if previously updated. Also overwrite the\n"
		"    type of a file.\n"
		"  -h, --help\n"
		"    Display this help information.\n"
		"\n"
		"Obsolete options:\n"
		"  -all  (synonymous with --all)\n"
		"  -apps (synonymous with --apps)\n"
		"\n",
		sProgramName);

	exit(status);
}


static status_t
process_file(const char* path)
{
	status_t status = B_OK;

	BEntry entry(path);
	if (!entry.Exists())
		status = B_ENTRY_NOT_FOUND;

	if (gFiles && status == B_OK)
		status = update_mime_info(path, true, true, gForce);
	if (gApps && status == B_OK)
		status = create_app_meta_mime(path, true, true, gForce);

	if (status != B_OK) {
		fprintf(stderr, "%s: \"%s\": %s\n",
			sProgramName, path, strerror(status));
	}
	return status;
}


int
main(int argc, const char** argv)
{
	// parse arguments

	// replace old-style options first
	for (int i = 1; i < argc; i++) {
		const char* arg = argv[i];
		if (*arg != '-')
			break;
		if (strcmp(arg, "-all") == 0)
			argv[i] = "--all";
		else if (strcmp(arg, "-apps") == 0)
			argv[i] = "--apps";
	}

	for (;;) {
		static struct option sLongOptions[] = {
			{ "all", no_argument, 0, 'A' },
			{ "apps", no_argument, 0, 'a' },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "aAfFh", sLongOptions,
			NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				gApps = true;
				gFiles = false;
				break;
			case 'A':
				gApps = true;
				gFiles = true;
				break;
			case 'f':
				gForce = B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE;
				break;
			case 'F':
				gForce = B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL;
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	if (argc - optind < 1)
		usage(1);

	// process files

	BApplication app("application/x-vnd.haiku.mimeset");

	for (; optind < argc; optind++) {
		const char* arg = argv[optind];

		if (strcmp(arg, "@") == 0) {
			// read file names from stdin
			char name[B_PATH_NAME_LENGTH];
			while (fgets(name, sizeof(name), stdin) != NULL) {
				name[strlen(name) - 1] = '\0';
					// remove trailing '\n'
				if (process_file(name) != B_OK)
					exit(1);
			}
		} else {
			if (process_file(arg) != B_OK)
				exit(1);
		}
	}

	return 0;
}
