/*
 * Copyright 2006, Ryan Leavengood, leavengood@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Entry.h>
#include <Message.h>
#include <Node.h>
#include <String.h>
#include <TrackerAddon.h>


extern "C" void
process_refs(entry_ref dir, BMessage* msg, void* /*reserved*/)
{
        int32 refs;
        entry_ref ref;
        BString status("!!!STATUS_HERE!!!");
        BString type;

        for (refs = 0; msg->FindRef("refs", refs, &ref) == B_NO_ERROR; refs++) {
                BNode node(&ref);
                if ((node.InitCheck() == B_NO_ERROR)
                        && (node.ReadAttrString("BEOS:TYPE", &type) == B_NO_ERROR)
                        && (type == "text/x-email"))
                                node.WriteAttrString("MAIL:status", &status);
        }
}                                                                                     

