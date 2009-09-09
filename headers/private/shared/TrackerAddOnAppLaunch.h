/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _TRACKER_ADDON_APP_LAUNCH_H
#define _TRACKER_ADDON_APP_LAUNCH_H


#include <Entry.h>
#include <image.h>
#include <Roster.h>
#include <TrackerAddOn.h>


void
process_refs(entry_ref directory, BMessage* refs, void* reserved)
{
	// When this header file is included by an application it becomes
	// ready to be used as a Tracker add-on. This function will launch
	// the application with the entry_refs given by Tracker to the addon.

	// The "directory" entry_ref holds the entry_ref of the folder from where
	// the addon was launched. If launched from a query it's probably invalid.
	// The "refs" BMessage is a standard B_REFS_RECEIVED message whose
	// "refs" array contains the entry_refs of the selected files.
	// The last argument, "reserved", is currently unused.

	refs->AddRef("dir_ref", &directory);
		// The folder's entry_ref might be useful to some applications.
		// We include it by a different name than the other entry_refs.
		// It could be useful to know if an application got its refs as a 
		// Tracker addon. A B_REFS_RECEIVED message with a "dir_ref"
		// provides such a hint.

	// get the path of the Tracker add-on
	image_info image;
	int32 cookie = 0;
	status_t status = get_next_image_info(0, &cookie, &image);

	while (status == B_OK) {
		if (((char*)process_refs >= (char*)image.text
			&& (char*)process_refs <= (char*)image.text + image.text_size)
			|| ((char*)process_refs >= (char*)image.data
			&& (char*)process_refs <= (char*)image.data + image.data_size))
			break;
		
		status = get_next_image_info(0, &cookie, &image);
	}

	entry_ref addonRef;
	if (get_ref_for_path(image.name, &addonRef) != B_OK)
		return;

	status = be_roster->Launch(&addonRef, refs);
	if (status == B_OK || status == B_ALREADY_RUNNING)
		return;

	// If launching by entry_ref fails it's probably futile to
	// launch by signature this way, but we can try anyway.

	app_info appInfo;
	if (be_roster->GetAppInfo(&addonRef, &appInfo) != B_OK)
		return;

	be_roster->Launch(appInfo.signature, refs);
}


#endif	// _TRACKER_ADDON_APP_LAUNCH_H

