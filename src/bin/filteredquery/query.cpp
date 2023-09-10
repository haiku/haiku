// query.cpp
//
// A shell utility for somewhat emulating the Tracker's "Find By Formula"
// functionality.
//
// by Ficus Kirkpatrick (ficus@ior.com)
//
// Modified by Jerome Duval on November 03, 2003
//
// Filtering capability added by Stefano Ceccherini on January 14, 2005


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Path.h>
#include <Query.h>
#include <Entry.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <SupportDefs.h>
#include <String.h>

#include "FilteredQuery.h"

extern const char *__progname;

struct folder_params
{
	BPath path;
	bool includeSubFolders;
};


static bool
FilterByFolder(const entry_ref *ref, void *arg)
{
	folder_params* params = static_cast<folder_params*>(arg);
	BPath& wantedPath = params->path;
	bool includeSub = params->includeSubFolders;

	BPath path(ref);
	if (includeSub) {
		if (!strncmp(path.Path(), wantedPath.Path(),
				strlen(wantedPath.Path())))
			return true;
	} else {
		if (path.GetParent(&path) == B_OK &&
			path == wantedPath)
			return true;
	}
	return false;
}


// Option variables.
bool o_all_volumes = false;       // Query all volumes?
bool o_escaping = true;       // Escape metacharacters?
bool o_subfolders = false;

void
usage(void)
{
	printf("usage: %s [ -e ] [ -p <path-to-search> ] [ -s ] [ -a || -v <path-to-volume> ] expression\n"
		"  -e\t\tdon't escape meta-characters\n"
		"  -p <path>\tsearch only in the given path. Defaults to the current directory.\n"
		"  -s\t\tinclude subfolders\n"
		"  -a\t\tperform the query on all volumes\n"
		"  -v <file>\tperform the query on just one volume; <file> can be any\n"
		"\t\tfile on that volume. Defaults to the current volume.\n"
		"  Hint: '%s name=foo' will find files named \"foo\"\n",
		__progname, __progname);
	exit(0);
}


void
perform_query(BVolume &volume, const char *predicate, const char *filterpath)
{
	TFilteredQuery query;

	// Set up the volume and predicate for the query.
	query.SetVolume(&volume);
	query.SetPredicate(predicate);
	folder_params options;
	if (filterpath != NULL) {
		options.path = filterpath;
		options.includeSubFolders = o_subfolders;
		query.AddFilter(FilterByFolder, &options);
	}

	status_t status = query.Fetch();
	if (status == B_BAD_VALUE) {
		// the "name=" part may be omitted in our arguments
		BString string = "name=";
		string << predicate;

		query.SetPredicate(string.String());
		status = query.Fetch();
	}
	if (status != B_OK) {
		printf("query: bad query expression\n");
		return;
	}

	BEntry entry;
	BPath path;
	while (query.GetNextEntry(&entry) == B_OK) {
		if (entry.GetPath(&path) < B_OK) {
			fprintf(stderr, "%s: could not get path for entry\n", __progname);
			continue;
		}

		printf("%s\n", o_escaping ? 
			BString().CharacterEscape(path.Path(), " ()?*&\"'[]^\\~|;!<>*$", '\\').String()
			: path.Path());
	}
}


int
main(int32 argc, const char **argv)
{
	// Make sure we have the minimum number of arguments.
	if (argc < 2)
		usage();

	// Which volume do we make the query on?
	// Default to the current volume.
	char volumePath[B_FILE_NAME_LENGTH];
	char directoryPath[B_PATH_NAME_LENGTH];

	strcpy(volumePath, ".");
	strcpy(directoryPath, ".");

	// Parse command-line arguments.
	int opt;
	while ((opt = getopt(argc, (char **)argv, "easv:p:")) != -1) {
		switch(opt) {
		case 'a':
			o_all_volumes = true;
			break;

		case 'e':
			o_escaping = false;
			break;

		case 'v':
			strncpy(volumePath, optarg, B_FILE_NAME_LENGTH);
			break;

		case 'p':
			strncpy(directoryPath, optarg, B_PATH_NAME_LENGTH);
			break;

		case 's':
			o_subfolders = true;
			break;

		default:
			usage();
			break;
		}
	}

	BVolume volume;

	if (!o_all_volumes) {
		// Find the volume that the query should be performed on,
		// and set the query to it.
		BEntry entry(volumePath);
		if (entry.InitCheck() != B_OK) {
			printf("query: %s is not a valid file\n", volumePath);
			exit(1);
		}

		status_t status = entry.GetVolume(&volume);
		if (status != B_OK) {
			fprintf(stderr, "%s: could not get volume: %s\n", __progname, strerror(status));
			exit(1);
		}

		if (!volume.KnowsQuery())
			printf("query: volume containing %s is not query-enabled\n", volumePath);
		else
			perform_query(volume, argv[optind], directoryPath);
	} else {
		// Okay, we want to query all the disks -- so iterate over
		// them, one by one, running the query.
		BVolumeRoster volumeRoster;
		while (volumeRoster.GetNextVolume(&volume) == B_OK) {
			// We don't print errors here -- this will catch /pipe and
			// other filesystems we don't care about.
			if (volume.KnowsQuery())
				perform_query(volume, argv[optind], directoryPath);
		}
	}

	return 0;
}
