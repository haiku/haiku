/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Ryan Leavengood, leavengood@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Entry.h>
#include <Message.h>
#include <Node.h>
#include <String.h>


extern "C" void
process_refs(entry_ref dir, BMessage* message, void* /*reserved*/)
{
	entry_ref ref;
	for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BNode node(&ref);
		BString type;

		if (node.InitCheck() == B_OK
			&& node.ReadAttrString("BEOS:TYPE", &type) == B_OK
			&& type == "text/x-email") {
			BString previousStatus;
			BString status("Read");

			// Only update the attribute if there is an actual change
			if (node.ReadAttrString("MAIL:status", &previousStatus) != B_OK
				|| previousStatus != status)
				node.WriteAttrString("MAIL:status", &status);
		}
	}
}

