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
#include <storage/Path.h>
#include <storage/Query.h>
#include <storage/Entry.h>
#include <storage/Volume.h>
#include <storage/VolumeRoster.h>
#include <support/SupportDefs.h>
#include <support/String.h>

extern "C" {
	int32 getopt(int32, const char **, const char *);
	extern char *optarg;
	extern int32 optind;
}

// Option variables.
bool o_all_volumes = false;       // Query all volumes?
bool o_escaping = true;       // Escape metacharacters?

void usage(void)
{
	printf("usage:  query [ -e ] [ -a || -v volume ] expression\n");
	printf("        -e         don't escape meta-characters\n");
	printf("        -a         perform the query on all volumes\n");
	printf("        -v <file>  perform the query on just one volume;\n");
	printf("                   <file> can be any file on that volume.\n");
	printf("                   defaults to the boot volume.\n");
	printf(" hint:  query 'name=foo' will find a file named \"foo\"\n");
	exit(0);
}

void perform_query(BVolume *v, const char *predicate)
{
	BQuery query;
	
	// Set up the volume and predicate for the query.
	query.SetVolume(v);
	query.SetPredicate(predicate);

	if (query.Fetch() != B_OK) {
		printf("query: bad query expression\n");
		return;
	}
	
	BEntry e;
	BPath p;
	while (query.GetNextEntry(&e) == B_OK) {
		e.GetPath(&p);
		printf("%s\n", o_escaping ? 
			BString().CharacterEscape(p.Path(), " ()?*&\"'[]^\\~|;!<>*$", '\\').String() :
			p.Path());
	}
	
	return;
}

int main(int32 argc, const char **argv)
{
	// Make sure we have the minimum number of arguments.
	if (argc < 2) usage();	

	// Which volume do we make the query on?
	char volume_path[B_FILE_NAME_LENGTH];
	// Default to the boot volume.
	strcpy(volume_path, "/boot");
	
	// Parse command-line arguments.
	int32 opt;
	while ((opt = getopt(argc, argv, "ave:")) != -1) {
		switch(opt) {
		case 'a':
			o_all_volumes = true;
			break;
			
		case 'e':
			o_escaping = false;
			break;
		
		case 'v':
			strncpy(volume_path, optarg, B_FILE_NAME_LENGTH);
			break;
		
		default:
			usage();
			break;
		}
	}

	BVolume query_volume;
	
	if (!o_all_volumes) {
		// Find the volume that the query should be performed on,
		// and set the query to it.
		BEntry e(volume_path);
		if (e.InitCheck() != B_OK) {
			printf("query: %s is not a valid file\n", volume_path);
			exit(0);
		}
		e.GetVolume(&query_volume);

		if (!query_volume.KnowsQuery())
			printf("query: volume containing %s is not query-enabled\n", volume_path);
		else
			perform_query(&query_volume, argv[optind]);
		
		exit(0);
	}			

	// Okay, we want to query all the disks -- so iterate over
	// them, one by one, running the query.
	BVolumeRoster volume_roster;
	while (volume_roster.GetNextVolume(&query_volume) == B_OK) {
		// We don't print errors here -- this will catch /pipe and
		// other filesystems we don't care about.
		if (query_volume.KnowsQuery())
			perform_query(&query_volume, argv[optind]);
	}

	exit(0);	
}
