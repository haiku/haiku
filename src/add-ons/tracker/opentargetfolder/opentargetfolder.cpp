/* 
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


//! Open Target Folder - opens the folder of the link target in Tracker


#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <Messenger.h>
#include <Path.h>
#include <SymLink.h>

#include <tracker_private.h>


extern "C" void
process_refs(entry_ref directoryRef, BMessage* message, void*)
{
	BDirectory directory(&directoryRef);
	uint32 errors = 0;
	entry_ref ref;

	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BSymLink link(&ref);
		if (link.InitCheck() != B_OK || !link.IsSymLink()) {
			errors++;
			continue;
		}

		BEntry targetEntry;
		BPath path;
		if (link.MakeLinkedPath(&directory, &path) < B_OK
			|| targetEntry.SetTo(path.Path()) != B_OK
			|| targetEntry.GetParent(&targetEntry) != B_OK) {
			BAlert* alert = new BAlert("Open Target Folder",
				"Cannot open target folder. Maybe this link is broken?",
				"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(NULL);
			continue;
		}

		// create Tracker message...
		entry_ref target;
		targetEntry.GetRef(&target);

		BMessage trackerMessage(B_REFS_RECEIVED);
		trackerMessage.AddRef("refs", &target);

		// ...and send it
		BMessenger messenger(kTrackerSignature);
		messenger.SendMessage(&trackerMessage);

		// TODO: select entry via scripting?
	}

	if (errors > 0) {
		BAlert* alert = new BAlert("Open Target Folder",
			"This add-on can only be used on symbolic links.\n"
			"It opens the folder of the link target in Tracker.",
			"OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	}
}


int
main(int /*argc*/, char** /*argv*/)
{
	fprintf(stderr, "This can only be used as a Tracker add-on.\n");
	return -1; 
}
