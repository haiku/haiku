/*
 * Copyright 2008, Bruno Albuquerque, bga@bug-br.org.br. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "cddb_daemon.h"

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <NodeMonitor.h>
#include <Volume.h>

#include <fs_info.h>


static const char* kCddaFsName = "cdda";


CDDBDaemon::CDDBDaemon()
	: BApplication("application/x-vnd.Haiku-CDDBDaemon"),
	  fVolumeRoster(new BVolumeRoster)
{
	fVolumeRoster->StartWatching();
	
	BVolume volume;
	while (fVolumeRoster->GetNextVolume(&volume) == B_OK) {
		uint32 cddbId;
		if (CanLookup(volume.Device(), &cddbId)) {
			printf("CD can be looked up. CDDB id = %lu.\n", cddbId);
			// TODO(bga): Implement and enable CDDB database lookup.
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
						uint32 cddbId;
						if (CanLookup(device, &cddbId)) {
							printf("CD can be looked up. CDDB id = %lu.\n",
								cddbId);
							// TODO(bga): Implement and enable CDDB database lookup.
						}
					}
				}		
			}
			break;
		default:
			BApplication::MessageReceived(message);
	}
}


bool
CDDBDaemon::CanLookup(const dev_t device, uint32* cddbId) const
{
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

	return true;
}


int main(void) {
	CDDBDaemon* cddbDaemon = new CDDBDaemon();
	cddbDaemon->Run();
	delete cddbDaemon; 
}
