/*
 * Copyright 2005-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ficus Kirkpatrick (ficus@ior.com)
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Application.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Query.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <String.h>

#include <ObjectList.h>


static const uint32 kMsgAddQuery = 'adqu';

extern const char *__progname;
static const char *kProgramName = __progname;

// Option variables.
static bool sAllVolumes = false;		// Query all volumes?
static bool sEscapeMetaChars = true;	// Escape metacharacters?
static bool sFilesOnly = false;			// Show only files?


class LiveQuery : public BApplication {
public:
								LiveQuery();
	virtual						~LiveQuery();

	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual void				ReadyToRun();

	virtual void				MessageReceived(BMessage* message);

private:
			void				_PrintUsage();
			void				_AddQuery(BVolume& volume,
									const char* predicate);
			void				_PerformQuery(BQuery& query);

			BObjectList<BQuery>	fQueries;
			bool				fArgsReceived;
};


LiveQuery::LiveQuery()
	:
	BApplication("application/x-vnd.test-live-query"),
	fQueries(10, true),
	fArgsReceived(false)
{
}


LiveQuery::~LiveQuery()
{
}


void
LiveQuery::ArgvReceived(int32 argc, char** argv)
{
	fArgsReceived = true;

	// Which volume do we make the query on?
	// Default to the current volume.
	char volumePath[B_FILE_NAME_LENGTH];
	strcpy(volumePath, ".");

	// Parse command-line arguments.
	int opt;
	while ((opt = getopt(argc, argv, "efav:")) != -1) {
		switch (opt) {
			case 'e':
				sEscapeMetaChars = false;
				break;
			case 'f':
				sFilesOnly = true;
				break;
			case 'a':
				sAllVolumes = true;
				break;
			case 'v':
				strncpy(volumePath, optarg, B_FILE_NAME_LENGTH);
				break;

			default:
				_PrintUsage();
				break;
		}
	}

	BVolume volume;

	if (!sAllVolumes) {
		// Find the volume that the query should be performed on,
		// and set the query to it.
		BEntry entry(volumePath);
		if (entry.InitCheck() != B_OK) {
			fprintf(stderr, "%s: \"%s\" is not a valid file\n", kProgramName,
				volumePath);
			exit(1);
		}

		status_t status = entry.GetVolume(&volume);
		if (status != B_OK) {
			fprintf(stderr, "%s: could not get volume: %s\n", kProgramName,
				strerror(status));
			exit(1);
		}

		if (!volume.KnowsQuery()) {
			fprintf(stderr, "%s: volume containing %s is not query-enabled\n",
				kProgramName, volumePath);
		} else
			_AddQuery(volume, argv[optind]);
	} else {
		// Okay, we want to query all the disks -- so iterate over
		// them, one by one, running the query.
		BVolumeRoster volumeRoster;
		while (volumeRoster.GetNextVolume(&volume) == B_OK) {
			// We don't print errors here -- this will catch /pipe and
			// other filesystems we don't care about.
			if (volume.KnowsQuery())
				_AddQuery(volume, argv[optind]);
		}
	}
}


void
LiveQuery::ReadyToRun()
{
	if (!fArgsReceived)
		_PrintUsage();
}


void
LiveQuery::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddQuery:
		{
			int32 device;
			const char* predicate;
			if (message->FindInt32("volume", &device) != B_OK
				|| message->FindString("predicate", &predicate) != B_OK)
				break;

			BVolume volume(device);
			BQuery* query = new BQuery;

			// Set up the volume and predicate for the query.
			query->SetVolume(&volume);
			query->SetPredicate(predicate);
			query->SetTarget(this);

			fQueries.AddItem(query);
			_PerformQuery(*query);
			break;
		}

		case B_QUERY_UPDATE:
		{
			int32 what;
			message->FindInt32("opcode", &what);

			int32 device;
			int64 directory;
			int64 node;
			const char* name;
			message->FindInt32("device", &device);
			message->FindInt64("directory", &directory);
			message->FindInt64("node", &node);
			message->FindString("name", &name);

			switch (what) {
				case B_ENTRY_CREATED:
				{
					printf("CREATED %s\n", name);
					break;
				}
				case B_ENTRY_REMOVED:
					printf("REMOVED %s\n", name);
					break;
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
LiveQuery::_PrintUsage()
{
	printf("usage: %s [ -ef ] [ -a || -v <path-to-volume> ] expression\n"
		"  -e\t\tdon't escape meta-characters\n"
		"  -f\t\tshow only files (ie. no directories or symbolic links)\n"
		"  -a\t\tperform the query on all volumes\n"
		"  -v <file>\tperform the query on just one volume; <file> can be any\n"
		"\t\tfile on that volume. Defaults to the current volume.\n"
		"  Hint: '%s name=bar' will find files named \"bar\"\n",
		kProgramName, kProgramName);

	Quit();
}


void
LiveQuery::_AddQuery(BVolume& volume, const char* predicate)
{
	BMessage add(kMsgAddQuery);
	add.AddInt32("volume", volume.Device());
	add.AddString("predicate", predicate);

	PostMessage(&add);
}


void
LiveQuery::_PerformQuery(BQuery& query)
{
	status_t status = query.Fetch();
	if (status != B_OK) {
		fprintf(stderr, "%s: bad query expression\n", kProgramName);
		return;
	}

	int32 count = 0;

	BEntry entry;
	BPath path;
	while (query.GetNextEntry(&entry) == B_OK) {
		if (sFilesOnly && !entry.IsFile())
			continue;

		if (entry.GetPath(&path) != B_OK) {
			fprintf(stderr, "%s: could not get path for entry\n", kProgramName);
			continue;
		}

		printf("%s\n", sEscapeMetaChars ? BString().CharacterEscape(
				path.Path(), " ()?*&\"'[]^\\~|;!<>*$\t", '\\').String()
			: path.Path());

		count++;
	}

	printf("FOUND %ld entries\n", count);
}


// #pragma mark -


int
main(int argc, char** argv)
{
	LiveQuery query;
	query.Run();

	return 0;
}
