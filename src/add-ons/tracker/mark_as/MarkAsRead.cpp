/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Ryan Leavengood, leavengood@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <E-mail.h>
#include <Entry.h>
#include <MailDaemon.h>
#include <mail_util.h>
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
			&& (type == B_MAIL_TYPE || type == B_PARTIAL_MAIL_TYPE)) {
			BString previousStatus;
			BString status("Read");
			read_flags previousRead;
			// if we're marking it via the add-on, we haven't really read it
			// so use B_SEEN instead of B_READ
			read_flags read = B_SEEN;
			
			// Update the MAIL:read status to match
			// Check both B_SEEN and B_READ
			// (so we don't overwrite B_READ with B_SEEN)
			if (read_read_attr(node, previousRead) != B_OK ||
				(previousRead != B_SEEN && previousRead != B_READ)) {
				int32 account;
				if (node.ReadAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE,
					0LL, &account, sizeof(account)) == sizeof(account))
					BMailDaemon::MarkAsRead(account, ref, read);
				else
					write_read_attr(node, read);
			}

			// We want to keep the previous behavior of updating the status
			// string, but write_read_attr will only change the status string
			// if it's one of "New", "Seen", or "Read" (and not, for example,
			// "Replied"), so we change the status string here
			// Only update the attribute if there is an actual change
			if (node.ReadAttrString(B_MAIL_ATTR_STATUS, &previousStatus) != B_OK
				|| previousStatus != status)
				node.WriteAttrString(B_MAIL_ATTR_STATUS, &status);
		}
	}
}

