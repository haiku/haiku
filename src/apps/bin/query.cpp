// query.cpp
//
// A shell utility for somewhat emulating the Tracker's "Find By Formula"
// functionality.
//
// by Ficus Kirkpatrick (ficus@ior.com)
//
// Modified by Jerome Duval on November 03, 2003
//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <storage/Path.h>
#include <storage/Query.h>
#include <storage/Entry.h>
#include <storage/Volume.h>
#include <storage/VolumeRoster.h>
#include <support/SupportDefs.h>
#include <support/String.h>


// Option variables.
bool o_all_volumes = false;       // Query all volumes?
bool o_escaping = true;       // Escape metacharacters?


void
usage(void)
{
	printf("usage:  query [ -e ] [ -a || -v volume ] expression\n");
	printf("        -e         don't escape meta-characters\n");
	printf("        -a         perform the query on all volumes\n");
	printf("        -v <file>  perform the query on just one volume;\n");
	printf("                   <file> can be any file on that volume.\n");
	printf("                   defaults to the current volume.\n");
	printf(" hint:  query 'name=foo' will find a file named \"foo\"\n");
	exit(0);
}


void
perform_query(BVolume &volume, const char *predicate)
{
	BQuery query;

	// Set up the volume and predicate for the query.
	query.SetVolume(&volume);
	query.SetPredicate(predicate);

	if (query.Fetch() != B_OK) {
		printf("query: bad query expression\n");
		return;
	}

	BEntry entry;
	BPath path;
	while (query.GetNextEntry(&entry) == B_OK) {
		if (entry.GetPath(&path) < B_OK) {
			fprintf(stderr, "could not get path for entry\n");
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
	strcpy(volumePath, ".");

	// Parse command-line arguments.
	int opt;
	while ((opt = getopt(argc, argv, "eav:")) != -1) {
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
			exit(0);
		}
		entry.GetVolume(&volume);

		if (!volume.KnowsQuery())
			printf("query: volume containing %s is not query-enabled\n", volumePath);
		else
			perform_query(volume, argv[optind]);
	} else {	
		// Okay, we want to query all the disks -- so iterate over
		// them, one by one, running the query.
		BVolumeRoster volumeRoster;
		while (volumeRoster.GetNextVolume(&volume) == B_OK) {
			// We don't print errors here -- this will catch /pipe and
			// other filesystems we don't care about.
			if (volume.KnowsQuery())
				perform_query(volume, argv[optind]);
		}
	}

	return 0;
}
