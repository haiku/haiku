/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <Directory.h>
#include <E-mail.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <MailDaemon.h>
#include <mail_util.h>
#include <MenuItem.h>
#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>


static BPoint
mouse_position()
{
	// Returns the mouse position in screen coordinates
	BPoint position;
	get_mouse(&position, NULL);
	return position;
}


static void
add_status_item(BMenu* menu, const char* name)
{
	if (menu->FindItem(name) != NULL)
		return;

	// Sort items alphabetically
	int32 index;
	for (index = 0; index < menu->CountItems(); index++) {
		if (strcmp(menu->ItemAt(index)->Label(), name) > 0)
			break;
	}

	menu->AddItem(new BMenuItem(name, NULL), index);
}


static void
retrieve_status_items(BMenu* menu)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Mail/status");

	BDirectory directory(path.Path());

	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		if (!strcmp(ref.name, ".") || !strcmp(ref.name, ".."))
			continue;

		add_status_item(menu, ref.name);
	}

	add_status_item(menu, "New");
	add_status_item(menu, "Read");
	add_status_item(menu, "Replied");
}


// #pragma mark -


extern "C" void
process_refs(entry_ref dir, BMessage* message, void* /*reserved*/)
{
	BPopUpMenu* menu = new BPopUpMenu("status");
	retrieve_status_items(menu);

	BMenuItem* item = menu->Go(mouse_position() - BPoint(5, 5), false, true);
	if (item == NULL)
		return;

	BString status = item->Label();
	//TODO:This won't work anymore when the menu gets translated! Use index!

	entry_ref ref;
	for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BNode node(&ref);
		BString type;

		if (node.InitCheck() == B_OK
			&& node.ReadAttrString("BEOS:TYPE", &type) == B_OK
			&& (type == B_MAIL_TYPE || type == B_PARTIAL_MAIL_TYPE)) {
			BString previousStatus;
			read_flags previousRead;
			
			// Update the MAIL:read flag
			if (status == "New") {
				if (read_read_attr(node, previousRead) != B_OK ||
					previousRead != B_UNREAD)
					write_read_attr(node, B_UNREAD);
			}
			else if (status == "Read") {
				// if we're marking it via the add-on, we haven't really read it
				// so use B_SEEN instead of B_READ
				// Check both B_SEEN and B_READ
				// (so we don't overwrite B_READ with B_SEEN)
				if (read_read_attr(node, previousRead) != B_OK ||
					(previousRead != B_SEEN && previousRead != B_READ)) {
					int32 account;
					if (node.ReadAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE,
						0LL, &account, sizeof(account)) == sizeof(account))
						BMailDaemon::MarkAsRead(account, ref, B_SEEN);
					else
						write_read_attr(node, B_SEEN);
				}
			}
			// ignore "Replied"; no matching MAIL:read status

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

