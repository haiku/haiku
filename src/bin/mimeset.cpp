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
#include <Path.h>

#include <mime/AppMetaMimeCreator.h>
#include <mime/Database.h>
#include <mime/DatabaseLocation.h>
#include <mime/MimeInfoUpdater.h>
#include <mime/MimeSnifferAddonManager.h>
#include <mime/TextSnifferAddon.h>


using namespace BPrivate::Storage::Mime;


extern const char* __progname;
static const char* sProgramName = __progname;

// options
bool gFiles = true;
bool gApps = false;
int gForce = B_UPDATE_MIME_INFO_NO_FORCE;

static Database* sDatabase = NULL;


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
		"  -m, --mimedb <directory>\n"
		"    Instead of the system MIME DB use the given directory\n"
		"    <directory>. The option can occur multiple times to specify a\n"
		"    list of directories. MIME DB changes are written to the first\n"
		"    specified directory.\n"
		"\n"
		"Obsolete options:\n"
		"  -all  (synonymous with --all)\n"
		"  -apps (synonymous with --apps)\n"
		"\n",
		sProgramName);

	exit(status);
}


static status_t
process_file_with_custom_mime_db(const BEntry& entry)
{
	AppMetaMimeCreator appMetaMimeCreator(sDatabase, NULL, gForce);
	MimeInfoUpdater mimeInfoUpdater(sDatabase, NULL, gForce);

	entry_ref ref;
	status_t error = entry.GetRef(&ref);

	if (gFiles && error == B_OK)
		error = mimeInfoUpdater.DoRecursively(ref);
	if (gApps && error == B_OK) {
		error = appMetaMimeCreator.DoRecursively(ref);
		if (error == B_BAD_TYPE) {
			// Ignore B_BAD_TYPE silently. The most likely cause is that the
			// file doesn't have a "BEOS:APP_SIG" attribute.
			error = B_OK;
		}
	}

	if (error != B_OK) {
		BPath path;
		fprintf(stderr, "%s: \"%s\": %s\n",
			sProgramName,
			entry.GetPath(&path) == B_OK ? path.Path() : entry.Name(),
			strerror(error));
		return error;
	}

	return B_OK;
}


static status_t
process_file(const char* path)
{
	status_t status = B_OK;

	BEntry entry(path);
	if (!entry.Exists())
		status = B_ENTRY_NOT_FOUND;

	if (sDatabase != NULL)
		return process_file_with_custom_mime_db(entry);

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

	BStringList databaseDirectories;

	for (;;) {
		static struct option sLongOptions[] = {
			{ "all", no_argument, 0, 'A' },
			{ "apps", no_argument, 0, 'a' },
			{ "help", no_argument, 0, 'h' },
			{ "mimedb", required_argument, 0, 'm' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "aAfFhm:", sLongOptions,
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
			case 'm':
				databaseDirectories.Add(optarg);
				break;
			default:
				usage(1);
				break;
		}
	}

	if (argc - optind < 1)
		usage(1);

	// set up custom MIME DB, if specified
	DatabaseLocation databaseLocation;
	if (!databaseDirectories.IsEmpty()) {
		int32 count = databaseDirectories.CountStrings();
		for (int32 i = 0; i < count; i++)
			databaseLocation.AddDirectory(databaseDirectories.StringAt(i));

		status_t error = MimeSnifferAddonManager::CreateDefault();
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to create MIME sniffer add-on "
				"manager: %s\n", sProgramName, strerror(error));
			exit(1);
		}
		MimeSnifferAddonManager* manager = MimeSnifferAddonManager::Default();
		manager->AddMimeSnifferAddon(
			new(std::nothrow) TextSnifferAddon(&databaseLocation));

		sDatabase = new(std::nothrow) Database(&databaseLocation, manager,
			NULL);
		if (sDatabase == NULL) {
			fprintf(stderr, "%s: Out of memory!\n", sProgramName);
			exit(1);
		}

		error = sDatabase->InitCheck();
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to init MIME DB: %s\n", sProgramName,
				strerror(error));
			exit(1);
		}
	}

	// process files

	BApplication app("application/x-vnd.haiku.mimeset");

	for (; optind < argc; optind++) {
		const char* arg = argv[optind];

		if (strcmp(arg, "@") == 0) {
			// read file names from stdin
			char name[B_PATH_NAME_LENGTH];
			while (fgets(name, sizeof(name), stdin) != NULL) {
				if (name[0] != '\0') {
					name[strlen(name) - 1] = '\0';
						// remove trailing '\n'
				}
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
