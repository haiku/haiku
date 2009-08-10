/*
 * Copyright 2008-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */

#include "cddb_daemon.h"

#include "cddb_server.h"

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Message.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <fs_info.h>
#include <stdlib.h>


static const char* kCddaFsName = "cdda";
static const int kMaxTocSize = 1024;


CDDBDaemon::CDDBDaemon()
	: BApplication("application/x-vnd.Haiku-cddb_daemon"),
	  fVolumeRoster(new BVolumeRoster)
{
	fVolumeRoster->StartWatching();
	
	BVolume volume;
	printf("Checking currently mounted volumes ...\n");
	while (fVolumeRoster->GetNextVolume(&volume) == B_OK) {
		if (_Lookup(volume.Device()) != B_OK) {
			continue;
		}
	}
	printf("Checking complete. Listening for device mounts.\n");
}


CDDBDaemon::~CDDBDaemon()
{
	fVolumeRoster->StopWatching();
	delete fVolumeRoster;
}


void
CDDBDaemon::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_NODE_MONITOR:
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) == B_OK) {
				if (opcode == B_DEVICE_MOUNTED) {
					dev_t device;
					if (message->FindInt32("new device", &device) == B_OK) {
						if (_Lookup(device) != B_OK)
							break;
					}
				}		
			}
			break;
		default:
			BApplication::MessageReceived(message);
	}
}


status_t
CDDBDaemon::_Lookup(const dev_t device)
{
	scsi_toc_toc* toc = (scsi_toc_toc*)malloc(kMaxTocSize);
	if (toc == NULL)
		return B_NO_MEMORY;

	uint32 cddbId;
	if (!_CanLookup(device, &cddbId, toc)) {
		free(toc);
		printf("Skipping device with id %ld.\n", device);
		return B_BAD_TYPE;
	}
		
	printf("Looking up CD with CDDB Id %08lx.\n", cddbId);

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
		delete (QueryResponseData*)queryResponse.RemoveItem(0L);
	}
						
	queryResponse.MakeEmpty();

	// Delete itens in the track data BList in the read response data;
	count = readResponse.tracks.CountItems();
	for (int32 i = 0; i < count; ++i) {
		delete (TrackData*)readResponse.tracks.RemoveItem(0L);
	}
						
	readResponse.tracks.MakeEmpty();
							
	return B_OK;
}


bool
CDDBDaemon::_CanLookup(const dev_t device, uint32* cddbId,
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
CDDBDaemon::_SelectResult(BList* response) const
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
CDDBDaemon::_WriteCDData(dev_t device, QueryResponseData* diskData,
	ReadResponseData* readResponse)
{
	// Rename volume.
	BVolume volume(device);
	
	status_t result;
	status_t error = B_OK;
	
	BString name = diskData->artist << " - " << diskData->title;
	name.ReplaceSet("/", " ");
	
	if ((result = volume.SetName(name.String())) != B_OK) {
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
		name = data->title;
		name.ReplaceSet("/", " ");
		
		if ((result = entry.Rename(name.String())) != B_OK) {
			printf("Failed renaming entry at index %d to \"%s\".\n", index,
				name.String());
			error = result;
				// User can benefit from continuing through all tracks.
				// Report error later.
		}
		
		// Add relevant attributes. We consider an error here as non-fatal.
		BNode node(&entry);
		node.WriteAttr("Audio:Title", B_STRING_TYPE, 0, (data->title).String(),
			(data->title).Length());
		node.WriteAttr("Audio:Album", B_STRING_TYPE, 0,
			(readResponse->title).String(),
			(readResponse->title).Length());
		node.WriteAttr("Audio:Genre", B_STRING_TYPE, 0,
			(readResponse->genre).String(),
			(readResponse->genre).Length());
		node.WriteAttr("Audio:Year", B_INT32_TYPE, 0, &(readResponse->year),
			sizeof(int32));

		if (data->artist == "") {
			node.WriteAttr("Audio:Artist", B_STRING_TYPE, 0,
				(readResponse->artist).String(),
				(readResponse->artist).Length());
		} else {
			node.WriteAttr("Audio:Artist", B_STRING_TYPE, 0,
				(data->artist).String(), (data->artist).Length());			
		}
			
		index++;
	}
	
	return error;
}	


int main(void) {
	printf("CDDB Daemon for Haiku v1.0.0 started.\n");
	CDDBDaemon* cddbDaemon = new CDDBDaemon();
	cddbDaemon->Run();
	delete cddbDaemon; 
}
