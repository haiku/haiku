/*
 * Copyright 2008-2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Bruno Albuquerque, bga@bug-br.org.br
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <fs_info.h>
#include <Message.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <scsi_cmds.h>

#include "cddb_server.h"


class CDDBLookup : public BApplication {
public:
								CDDBLookup();
	virtual						~CDDBLookup();

			void				LookupAll(CDDBServer& server, bool dumpOnly,
									bool verbose);
			status_t			Lookup(CDDBServer& server, const char* path,
									bool dumpOnly, bool verbose);
			status_t			Lookup(CDDBServer& server, const dev_t device,
									bool dumpOnly, bool verbose);
			status_t			Dump(CDDBServer& server, const char* category,
									const char* cddbID, bool verbose);

private:
			bool				_ReadTOC(const dev_t device, uint32* cddbID,
									scsi_toc_toc* toc) const;
			const QueryResponseData*
								_SelectResult(
									const QueryResponseList& responses) const;
			status_t			_WriteCDData(dev_t device,
									const QueryResponseData& diskData,
									const ReadResponseData& readResponse);
			void				_Dump(const ReadResponseData& readResponse)
									const;
};


static struct option const kLongOptions[] = {
	{"info", required_argument, 0, 'i'},
	{"dump", no_argument, 0, 'd'},
	{"verbose", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};


extern const char *__progname;
static const char *kProgramName = __progname;

static const char* kDefaultServerAddress = "freedb.freedb.org:80";
static const char* kCddaFsName = "cdda";
static const int kMaxTocSize = 1024;


CDDBLookup::CDDBLookup()
	:
	BApplication("application/x-vnd.Haiku-cddb_lookup")
{
}


CDDBLookup::~CDDBLookup()
{
}


void
CDDBLookup::LookupAll(CDDBServer& server, bool dumpOnly, bool verbose)
{
	BVolumeRoster roster;
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {
		Lookup(server, volume.Device(), dumpOnly, verbose);
	}
}


status_t
CDDBLookup::Lookup(CDDBServer& server, const char* path, bool dumpOnly,
	bool verbose)
{
	BVolumeRoster roster;
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {
		fs_info info;
		if (fs_stat_dev(volume.Device(), &info) != B_OK)
			continue;

		if (strcmp(path, info.device_name) == 0)
			return Lookup(server, volume.Device(), dumpOnly, verbose);
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
CDDBLookup::Lookup(CDDBServer& server, const dev_t device, bool dumpOnly,
	bool verbose)
{
	scsi_toc_toc* toc = (scsi_toc_toc*)malloc(kMaxTocSize);
	if (toc == NULL)
		return B_NO_MEMORY;

	uint32 cddbID;
	if (!_ReadTOC(device, &cddbID, toc)) {
		free(toc);
		fprintf(stderr, "Skipping device with id %" B_PRId32 ".\n", device);
		return B_BAD_TYPE;
	}

	printf("Looking up CD with CDDB Id %08" B_PRIx32 ".\n", cddbID);

	BObjectList<QueryResponseData> queryResponses(10, true);
	status_t result = server.Query(cddbID, toc, queryResponses);
	if (result != B_OK) {
		fprintf(stderr, "Error when querying CD: %s\n", strerror(result));
		free(toc);
		return result;
	}

	free(toc);

	const QueryResponseData* diskData = _SelectResult(queryResponses);
	if (diskData == NULL) {
		fprintf(stderr, "Could not find any CD entries in query response.\n");
		return B_BAD_INDEX;
	}

	ReadResponseData readResponse;
	result = server.Read(*diskData, readResponse, verbose);
	if (result != B_OK) {
		fprintf(stderr, "Could not read detailed CD entry from server: %s\n",
			strerror(result));
		return result;
	}

	if (dumpOnly)
		_Dump(readResponse);

	if (!dumpOnly) {
		result = _WriteCDData(device, *diskData, readResponse);
		if (result == B_OK)
			printf("CD data saved.\n");
		else
			fprintf(stderr, "Error writing CD data: %s\n", strerror(result));
	}
	return B_OK;
}


status_t
CDDBLookup::Dump(CDDBServer& server, const char* category, const char* cddbID,
	bool verbose)
{
	ReadResponseData readResponse;
	status_t status = server.Read(category, cddbID, "", readResponse, verbose);
	if (status != B_OK) {
		fprintf(stderr, "Could not read detailed CD entry from server: %s\n",
			strerror(status));
		return status;
	}

	_Dump(readResponse);
	return B_OK;
}


bool
CDDBLookup::_ReadTOC(const dev_t device, uint32* cddbID,
	scsi_toc_toc* toc) const
{
	if (cddbID == NULL || toc == NULL)
		return false;

	// Is it an Audio disk?
	fs_info info;
	fs_stat_dev(device, &info);
	if (strncmp(info.fsh_name, kCddaFsName, strlen(kCddaFsName)) != 0)
		return false;

	// Does it have the CD:do_lookup attribute and is it true?
	BVolume volume(device);
	BDirectory directory;
	volume.GetRootDirectory(&directory);

	bool doLookup;
	if (directory.ReadAttr("CD:do_lookup", B_BOOL_TYPE, 0, (void *)&doLookup,
			sizeof(bool)) < B_OK || !doLookup)
		return false;

	// Does it have the CD:cddbid attribute?
	if (directory.ReadAttr("CD:cddbid", B_UINT32_TYPE, 0, (void *)cddbID,
			sizeof(uint32)) < B_OK)
		return false;

	// Does it have the CD:toc attribute?
	if (directory.ReadAttr("CD:toc", B_RAW_TYPE, 0, (void *)toc,
			kMaxTocSize) < B_OK)
		return false;

	return true;
}


const QueryResponseData*
CDDBLookup::_SelectResult(const QueryResponseList& responses) const
{
	// Select a single CD match from the response and return it.
	//
	// TODO(bga):Right now it just picks the first entry on the list but
	// someday we may want to let the user choose one.
	int32 numItems = responses.CountItems();
	if (numItems > 0) {
		if (numItems > 1)
			printf("Multiple matches found :\n");

		for (int32 i = 0; i < numItems; i++) {
			QueryResponseData* data = responses.ItemAt(i);
			printf("* %s : %s - %s (%s)\n", data->cddbID.String(),
				data->artist.String(), data->title.String(),
				data->category.String());
		}
		if (numItems > 1)
			printf("Returning first entry.\n");

		return responses.ItemAt(0);
	}

	return NULL;
}


status_t
CDDBLookup::_WriteCDData(dev_t device, const QueryResponseData& diskData,
	const ReadResponseData& readResponse)
{
	// Rename volume.
	BVolume volume(device);

	status_t error = B_OK;

	BString name = diskData.artist;
	name += " - ";
	name += diskData.title;
	name.ReplaceSet("/", " ");

	status_t result = volume.SetName(name.String());
	if (result != B_OK) {
		printf("Can't set volume name.\n");
		return result;
	}

	// Rename tracks and add relevant Audio attributes.
	BDirectory cddaRoot;
	volume.GetRootDirectory(&cddaRoot);

	BEntry entry;
	int index = 0;
	while (cddaRoot.GetNextEntry(&entry) == B_OK) {
		TrackData* track = readResponse.tracks.ItemAt(index);

		// Update name.
		int trackNum = index + 1; // index=0 is actually Track 1
		name.SetToFormat("%02d %s.wav", trackNum, track->title.String());
		name.ReplaceSet("/", " ");

		result = entry.Rename(name.String());
		if (result != B_OK) {
			fprintf(stderr, "%s: Failed renaming entry at index %d to "
				"\"%s\".\n", kProgramName, index, name.String());
			error = result;
				// User can benefit from continuing through all tracks.
				// Report error later.
		}

		// Add relevant attributes. We consider an error here as non-fatal.
		BNode node(&entry);
		node.WriteAttrString("Media:Title", &track->title);
		node.WriteAttrString("Audio:Album", &readResponse.title);
		if (readResponse.genre.Length() != 0)
			node.WriteAttrString("Media:Genre", &readResponse.genre);
		if (readResponse.year != 0) {
			node.WriteAttr("Media:Year", B_INT32_TYPE, 0,
				&readResponse.year, sizeof(int32));
		}

		if (track->artist == "")
			node.WriteAttrString("Audio:Artist", &readResponse.artist);
		else
			node.WriteAttrString("Audio:Artist", &track->artist);

		index++;
	}

	return error;
}


void
CDDBLookup::_Dump(const ReadResponseData& readResponse) const
{
	printf("Artist: %s\n", readResponse.artist.String());
	printf("Title:  %s\n", readResponse.title.String());
	printf("Genre:  %s\n", readResponse.genre.String());
	printf("Year:   %" B_PRIu32 "\n", readResponse.year);
	puts("Tracks:");
	for (int32 i = 0; i < readResponse.tracks.CountItems(); i++) {
		TrackData* track = readResponse.tracks.ItemAt(i);
		if (track->artist.IsEmpty()) {
			printf("  %2" B_PRIu32 ". %s\n", track->trackNumber + 1,
				track->title.String());
		} else {
			printf("  %2" B_PRIu32 ". %s - %s\n", track->trackNumber + 1,
				track->artist.String(), track->title.String());
		}
	}
}


// #pragma mark -


static void
usage(int exitCode)
{
	fprintf(exitCode == EXIT_SUCCESS ? stdout : stderr,
		"Usage: %s [-vdh] [-s <server>] [-i <category> <cddb-id>|<device>]\n"
		"\nYou can specify the device either as path on the device, or using "
		"the\ndevice name directly. If you do not specify a device, and are\n"
		"using the -i option, all volumes will be scanned for CD info.\n\n"
		"  -s, --server\tUse alternative server. Default is %s.\n"
		"  -v, --verbose\tVerbose output.\n"
		"  -d, --dump\tDo not write attributes, only dump info to terminal.\n"
		"  -h, --help\tThis help text.\n"
		"  -i\t\tDump info for the specified category/cddb ID pair.\n",
		kProgramName, kDefaultServerAddress);
	exit(exitCode);
}


int
main(int argc, char* const* argv)
{
	const char* serverAddress = kDefaultServerAddress;
	const char* category = NULL;
	bool verbose = false;
	bool dump = false;

	int c;
	while ((c = getopt_long(argc, argv, "i:s:vdh", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'i':
				category = optarg;
				break;
			case 's':
				serverAddress = optarg;
				break;
			case 'v':
				verbose = true;
				break;
			case 'd':
				dump = true;
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	CDDBServer server(serverAddress);
	CDDBLookup cddb;
	int left = argc - optind;

	if (category != NULL) {
		if (left != 1) {
			fprintf(stderr, "CDDB disc ID expected!\n");
			return EXIT_FAILURE;
		}

		const char* cddbID = argv[optind];
		cddb.Dump(server, category, cddbID, verbose);
	} else {
		// Lookup via actual CD
		if (left > 0) {
			for (int i = optind; i < argc; i++) {
				// Allow to specify a device
				const char* path = argv[i];
				status_t status;
				if (strncmp(path, "/dev/", 5) == 0) {
					status = cddb.Lookup(server, path, dump, verbose);
				} else {
					dev_t device = dev_for_path(path);
					if (device >= 0)
						status = cddb.Lookup(server, device, dump, verbose);
					else
						status = (status_t)device;
				}

				if (status != B_OK) {
					fprintf(stderr, "Invalid path \"%s\": %s\n", path,
						strerror(status));
					return EXIT_FAILURE;
				}
			}
		} else
			cddb.LookupAll(server, dump, verbose);
	}

	return 0;
}
