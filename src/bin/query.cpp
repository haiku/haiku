/*
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	Ficus Kirkpatrick (ficus@ior.com)
 *	Jérôme Duval
 *	Axel Dörfler, axeld@pinc-software.de
 */

/* A shell utility for somewhat emulating the Tracker's "Find By Formula"
 * functionality.
 */


#include <Entry.h>
#include <LocaleRoster.h>
#include <Path.h>
#include <Query.h>
#include <String.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


extern const char *__progname;
static const char *kProgramName = __progname;

// Option variables.
static bool sAllVolumes = false;		// Query all volumes?
static bool sEscapeMetaChars = true;	// Escape metacharacters?
static bool sFilesOnly = false;			// Show only files?
static bool sLocalizedAppNames = false;	// match localized names


void
usage(void)
{
	printf("usage: %s [ -ef ] [ -a || -v <path-to-volume> ] expression\n"
		"  -e\t\tdon't escape meta-characters\n"
		"  -f\t\tshow only files (ie. no directories or symbolic links)\n"
		"  -l\t\tmatch expression with localized application names\n"
		"  -a\t\tperform the query on all volumes\n"
		"  -v <file>\tperform the query on just one volume; <file> can be any\n"
		"\t\tfile on that volume. Defaults to the current volume.\n"
		"  Hint: '%s name=foo' will find files named \"foo\"\n",
		kProgramName, kProgramName);
	exit(0);
}


void
perform_query(BVolume &volume, const char *predicate)
{
	BQuery query;
	query.SetVolume(&volume);

	if (sLocalizedAppNames)
		query.SetPredicate("BEOS:APP_SIG=*");
	else
		query.SetPredicate(predicate);

	status_t status = query.Fetch();
	if (status == B_BAD_VALUE) {
		// the "name=" part may be omitted in our arguments
		BString string = "name=";
		string << predicate;

		query.SetPredicate(string.String());
		status = query.Fetch();
	}
	if (status != B_OK) {
		fprintf(stderr, "%s: bad query expression\n", kProgramName);
		return;
	}

	BEntry entry;
	BPath path;
	while (query.GetNextEntry(&entry) == B_OK) {
		if (sFilesOnly && !entry.IsFile())
			continue;

		if (entry.GetPath(&path) < B_OK) {
			fprintf(stderr, "%s: could not get path for entry\n", kProgramName);
			continue;
		}

		BString string;
		if (sLocalizedAppNames && predicate != NULL) {
			entry_ref ref;

			if (entry.GetRef(&ref) != B_OK || BLocaleRoster::Default()
				->GetLocalizedFileName(string, ref) != B_OK)
				continue;

			if (string.IFindFirst(predicate) < 0)
				continue;
		}

		string = path.Path();
		if (sEscapeMetaChars)
			string.CharacterEscape(" ()?*&\"'[]^\\~|;!<>*$\t", '\\');

		printf("%s\n", string.String());
	}
}


int
main(int argc, char **argv)
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
	while ((opt = getopt(argc, argv, "efalv:")) != -1) {
		switch(opt) {
			case 'e':
				sEscapeMetaChars = false;
				break;
			case 'f':
				sFilesOnly = true;
				break;
			case 'a':
				sAllVolumes = true;
				break;
			case 'l':
				sLocalizedAppNames = true;
				break;
			case 'v':
				strlcpy(volumePath, optarg, B_FILE_NAME_LENGTH);
				break;

			default:
				usage();
				break;
		}
	}

	BVolume volume;

	if (!sAllVolumes) {
		// Find the volume that the query should be performed on,
		// and set the query to it.
		BEntry entry(volumePath);
		if (entry.InitCheck() != B_OK) {
			fprintf(stderr, "%s: \"%s\" is not a valid file\n", kProgramName, volumePath);
			exit(1);
		}

		status_t status = entry.GetVolume(&volume);
		if (status != B_OK) {
			fprintf(stderr, "%s: could not get volume: %s\n", kProgramName, strerror(status));
			exit(1);
		}

		if (!volume.KnowsQuery())
			fprintf(stderr, "%s: volume containing %s is not query-enabled\n", kProgramName, volumePath);
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
