/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Mime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


extern const char *__progname;
static const char *sProgramName = __progname;

// options
bool gFiles = true;
bool gApps = false;
int gForce = 0;


void
usage(int status)
{
	printf("usage: %s [OPTION]... [PATH]...\n"
		"  -all\t\tcombine default action and the -apps option\n"
		"  -apps\t\tupdate 'app' and 'meta_mime' information\n"
		"  -f\t\tforce updating, even if previously updated\n"
		"  	\t  (will not overwrite the 'type' of a file)\n"
		"  -F\t\tforce updating, even if previously updated\n"
		"  	\t  (will overwrite the 'type' of a file)\n"
		"  --help\tdisplay this help information\n"
		"When PATH is @, file names are read from stdin\n\n",
		sProgramName);

	exit(status);
}


void
process_file(const char *path)
{
	status_t status = B_OK;

	BEntry entry(path);
	if (!entry.Exists())
		status = B_ENTRY_NOT_FOUND;

	if (gFiles && status >= B_OK)
		status = update_mime_info(path, true, true, gForce);
	if (gApps && status >= B_OK)
		create_app_meta_mime(path, true, true, gForce);

	if (status < B_OK) {
		fprintf(stderr, "%s: \"%s\": %s\n",
			sProgramName, path, strerror(status));
	}
}


int
main(int argc, char **argv)
{
	// parse arguments

	if (argc < 2)
		usage(-1);

	while (*++argv) {
		char *arg = *argv;
		if (*arg != '-')
			break;

		if (!strcmp(arg, "-all"))
			gApps = true;
		else if (!strcmp(arg, "-apps")) {
			gApps = true;
			gFiles = false;
		} else if (!strcmp(arg, "-f"))
			gForce = 1;
		else if (!strcmp(arg, "-F"))
			gForce = 2;
		else if (!strcmp(arg, "--help"))
			usage(0);
		else {
			fprintf(stderr, "unknown  option \"%s\"\n", arg);
			usage(-1);
		}
	}

	// process files

	BApplication app("application/x-vnd.haiku.mimeset");

	while (*argv) {
		char *arg = *argv++;

		if (!strcmp(arg, "@")) {
			// read file names from stdin
			char name[B_PATH_NAME_LENGTH];
			while (fgets(name, sizeof(name), stdin) != NULL) {
				name[strlen(name) - 1] = '\0';
					// remove trailing '\n'
				process_file(name);
			}
		} else
			process_file(arg);
	}

	return 0;
}

