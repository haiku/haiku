/*
 * Copyright 2008-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Bruno Albuquerque, bga@bug-br.org.br
 */


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

			void				LookupAll();
			status_t			Lookup(const dev_t device);

private:
			bool				_CanLookup(const dev_t device, uint32* cddbId,
										scsi_toc_toc* toc) const;
			QueryResponseData*	_SelectResult(BList* response) const;
			status_t			_WriteCDData(dev_t device,
									QueryResponseData* diskData,
									ReadResponseData* readResponse);
};


extern const char *__progname;
static const char *kProgramName = __progname;

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
CDDBLookup::LookupAll()
{
	BVolumeRoster roster;
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {
		Lookup(volume.Device());
	}
}


status_t
CDDBLookup::Lookup(const dev_t device)
{
	scsi_toc_toc* toc = (scsi_toc_toc*)malloc(kMaxTocSize);
	if (toc == NULL)
		return B_NO_MEMORY;

	uint32 cddbId;
	if (!_CanLookup(device, &cddbId, toc)) {
		free(toc);
		printf("Skipping device with id %" B_PRId32 ".\n", device);
		return B_BAD_TYPE;
	}

	printf("Looking up CD with CDDB Id %08" B_PRIx32 ".\n", cddbId);

	CDDBServer cddb_server("freedb.freedb.org:80");

	status_t result;

	BList queryResponse;
	if ((result = cddb_server.Query(cddbId, toc, &queryResponse)) != B_OK) {
		printf("Error when querying CD.\n");
		free(toc);
		return result;
	}

	free(toc);

	QueryResponseData* diskData = _SelectResult(&queryResponse);
	if (diskData == NULL) {
		printf("Could not find any CD entries in query response.\n");
		return B_BAD_INDEX;
	}

	ReadResponseData readResponse;
	if ((result = cddb_server.Read(diskData, &readResponse)) != B_OK) {
		return result;
	}

	if (_WriteCDData(device, diskData, &readResponse) == B_OK) {
		printf("CD data saved.\n");
	} else {
		printf("Error writting CD data.\n" );
	}

	// Delete itens in the query response BList;
	int32 count = queryResponse.CountItems();
	for (int32 i = 0; i < count; ++i) {
		delete (QueryResponseData*)queryResponse.RemoveItem((int32)0);
	}

	queryResponse.MakeEmpty();

	// Delete itens in the track data BList in the read response data;
	count = readResponse.tracks.CountItems();
	for (int32 i = 0; i < count; ++i) {
		delete (TrackData*)readResponse.tracks.RemoveItem((int32)0);
	}

	readResponse.tracks.MakeEmpty();

	return B_OK;
}


bool
CDDBLookup::_CanLookup(const dev_t device, uint32* cddbId,
	scsi_toc_toc* toc) const
{
	if (cddbId == NULL || toc == NULL)
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
	if (directory.ReadAttr("CD:cddbid", B_UINT32_TYPE, 0, (void *)cddbId,
		sizeof(uint32)) < B_OK)
		return false;

	// Does it have the CD:toc attribute?
	if (directory.ReadAttr("CD:toc", B_RAW_TYPE, 0, (void *)toc,
		kMaxTocSize) < B_OK)
		return false;

	return true;
}


QueryResponseData*
CDDBLookup::_SelectResult(BList* response) const
{
	// Select a single CD match from the response and return it.
	//
	// TODO(bga):Right now it just picks the first entry on the list but
	// someday we may want to let the user choose one.
	int32 numItems = response->CountItems();
	if (numItems > 0) {
		if (numItems > 1) {
			printf("Multiple matches found :\n");
		};
		for (int32 i = 0; i < numItems; i++) {
			QueryResponseData* data = (QueryResponseData*)response->ItemAt(i);
			printf("* %s : %s - %s (%s)\n", (data->cddbId).String(),
				(data->artist).String(), (data->title).String(),
				(data->category).String());
		}
		if (numItems > 1) {
			printf("Returning first entry.\n");
		}

		return (QueryResponseData*)response->ItemAt(0L);
	}

	return NULL;
}


status_t
CDDBLookup::_WriteCDData(dev_t device, QueryResponseData* diskData,
	ReadResponseData* readResponse)
{
	// Rename volume.
	BVolume volume(device);

	status_t error = B_OK;

	BString name = diskData->artist << " - " << diskData->title;
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
		TrackData* data = (TrackData*)((readResponse->tracks).ItemAt(index));

		// Update name.
		int trackNum = index + 1; // index=0 is actually Track 1
		name.SetToFormat("%02d %s.wav", trackNum, data->title.String());
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
		node.WriteAttr("Media:Title", B_STRING_TYPE, 0, data->title.String(),
			data->title.Length());
		node.WriteAttr("Audio:Album", B_STRING_TYPE, 0,
			readResponse->title.String(),
			readResponse->title.Length());
		if (readResponse->genre.Length() != 0) {
			node.WriteAttr("Media:Genre", B_STRING_TYPE, 0,
				readResponse->genre.String(),
				readResponse->genre.Length());
		}
		if (readResponse->year != 0) {
			node.WriteAttr("Media:Year", B_INT32_TYPE, 0,
				&readResponse->year, sizeof(int32));
		}

		if (data->artist == "") {
			node.WriteAttr("Audio:Artist", B_STRING_TYPE, 0,
				readResponse->artist.String(),
				readResponse->artist.Length());
		} else {
			node.WriteAttr("Audio:Artist", B_STRING_TYPE, 0,
				data->artist.String(), data->artist.Length());
		}

		index++;
	}

	return error;
}


// #pragma mark -


int
main(void)
{
	// TODO: support arguments to specify a device
	CDDBLookup cddb;
	cddb.LookupAll();

	return 0;
}
