/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>
#include <View.h>
#include <Window.h>


/*!	Returns the current mouse position in screen coordinates.
	Since there is no method to retrieve this in the Be API without a view,
	this looks a bit more complicated.
*/
static BPoint
mouse_position()
{
	BWindow* window = new BWindow(BRect(-1000, -1000, -900, -900), "mouse",
		B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AVOID_FRONT | B_AVOID_FOCUS);
	BView* view = new BView(window->Bounds(), "mouse", B_FOLLOW_ALL, 0);
	window->AddChild(view);
	window->Run();

	window->Lock();

	BPoint position;
	uint32 buttons;
	view->GetMouse(&position, &buttons);
	view->ConvertToScreen(&position);

	window->Quit();

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

	entry_ref ref;
	for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BNode node(&ref);
		BString type;

		if (node.InitCheck() == B_OK
			&& node.ReadAttrString("BEOS:TYPE", &type) == B_OK
			&& type == "text/x-email") {
			BString previousStatus;

			// Only update the attribute if there is an actual change
			if (node.ReadAttrString("MAIL:status", &previousStatus) != B_OK
				|| previousStatus != status)
				node.WriteAttrString("MAIL:status", &status);
		}
	}
}

