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
	while (fVolumeRoster->GetNextVolume(&volume) == B_OK) {
		if (_Lookup(volume.Device()) != B_OK) {
			continue;
		}
	}
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
		return B_BAD_TYPE;
	}
		
	printf("CD can be looked up. CDDB id = %08lx.\n", cddbId);

	CDDBServer cddb_server("freedb.freedb.org:80");

	status_t result;

	BList queryResponse;
	if ((result = cddb_server.Query(cddbId, toc, &queryResponse)) != B_OK) {
		free(toc);
		return result;
	}
			
	free(toc);

	QueryResponseData* diskData = _SelectResult(&queryResponse);
	if (diskData == NULL) {
		return B_BAD_INDEX;
	}

	ReadResponseData readResponse;
	if ((result = cddb_server.Read(diskData, &readResponse)) != B_OK) {
		return result;
	}
	
	if (_WriteCDData(device, diskData, &readResponse) == B_OK) {
		printf("CD data saved.\n");
	} else {
		printf("CD data not saved.\n" );
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
	if (response->CountItems() != 0)
		return (QueryResponseData*)response->ItemAt(0L);
	
	return NULL;
}


status_t
CDDBDaemon::_WriteCDData(dev_t device, QueryResponseData* diskData,
	ReadResponseData* readResponse)
{
	// Rename volume.
	BVolume volume(device);
	
	status_t result;
	if ((result = volume.SetName((diskData->title).String())) != B_OK) {
		printf("Can't set volume name.\n");
		return result;
	}
	
	return B_ERROR;
}	


int main(void) {
	CDDBDaemon* cddbDaemon = new CDDBDaemon();
	cddbDaemon->Run();
	delete cddbDaemon; 
}
