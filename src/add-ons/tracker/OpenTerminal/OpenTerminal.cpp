/*
 * Copyright 2007 - 2008 Chris Roberts. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Chris Roberts, cpr420@gmail.com
 */

#include <StorageKit.h>
#include <SupportKit.h>

#include <add-ons/tracker/TrackerAddOn.h>

#define APPLICATION "/boot/system/apps/Terminal &"
// The trailing '&' is so that it forks to the background and doesn't block 
//execution

void 
launch_terminal(BEntry* entry)
{
	BPath path;
	if (entry->GetPath(&path) != B_OK)
		return;

	chdir(path.Path());
	system (APPLICATION);
}


void
process_refs(entry_ref base_ref, BMessage* message, void* reserved) 
{
	BEntry entry;
	int32 i;
	entry_ref tracker_ref;

	for (i = 0; message->FindRef("refs", i, &tracker_ref) == B_OK; i++) {		
		// Pass 'true' as the traverse argument so that if the ref is a 
		// symbolic link, we get the target of the link to ensure that it 
		// actually exists
		if (entry.SetTo(&tracker_ref, true) == B_OK) {
			if (entry.IsDirectory())
				launch_terminal(&entry);
		}
	}

	// No folders were selected so we'll use the base folder
	if (i == 0) {
		if (entry.SetTo(&base_ref) == B_OK)
			launch_terminal(&entry);
	}
}
