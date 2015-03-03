/* 
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		John Scipione, jscipione@gmail.com
 */


//! Open Target Folder - opens the folder of the link target in Tracker


#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Entry.h>
#include <Messenger.h>
#include <Path.h>
#include <SymLink.h>

#include <tracker_private.h>


extern "C" void
process_refs(entry_ref directoryRef, BMessage* message, void*)
{
	entry_ref ref;

	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BSymLink link(&ref);
		if (link.InitCheck() != B_OK || !link.IsSymLink()) {
			BAlert* alert = new BAlert("Open Target Folder",
				"This add-on can only be used on symbolic links.\n"
				"It opens the folder of the link target in Tracker.",
				"OK");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(NULL);
			break;
		}

		BEntry targetEntry(&directoryRef, true);
		if (targetEntry.InitCheck() != B_OK) {
			BAlert* alert = new BAlert("Open Target Folder",
				"Cannot open target entry. Maybe this link is broken?",
				"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(NULL);
			break;
		}

		BEntry parentEntry;
		if (targetEntry.GetParent(&parentEntry) != B_OK) {
			BAlert* alert = new BAlert("Open Target Folder",
				"Cannot open target entry folder. Maybe this link is broken?",
				"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(NULL);
			break;
		}

		entry_ref parent;
		if (parentEntry.GetRef(&parent) != B_OK) {
			BAlert* alert = new BAlert("Open Target Folder",
				"Unable to locate entry_ref for the target entry folder.",
				"OK");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(NULL);
			break;
		}

		// create Tracker message...
		BMessage trackerMessage(B_REFS_RECEIVED);
		trackerMessage.AddRef("refs", &parent);

		// ...and send it
		BMessenger messenger(kTrackerSignature);
		messenger.SendMessage(&trackerMessage);

		// TODO: select entry via scripting?
	}
}


int
main(int /*argc*/, char** /*argv*/)
{
	fprintf(stderr, "This can only be used as a Tracker add-on.\n");
	return -1; 
}
