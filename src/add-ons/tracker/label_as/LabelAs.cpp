/*
 * Copyright 2026 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Catalog.h>
#include <Collator.h>
#include <Directory.h>
#include <E-mail.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Message.h>
#include <Node.h>
#include <ObjectList.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Label-as add-on"


BCollator collator;


static int
compare_menu_items(const BMenuItem* a, const BMenuItem* b)
{
	return collator.Compare(a->Label(), b->Label());
}


static BPoint
mouse_position()
{
	BPoint position;
	get_mouse(&position, NULL);
	return position;
}


void
open_labels_folder()
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Mail/labels");

	BEntry entry(path.Path());
	if (!entry.Exists())
		create_directory(path.Path(), 0777);

	BEntry folderEntry;
	if (folderEntry.SetTo(path.Path()) == B_OK
		&& folderEntry.Exists()) {
		BMessage openFolderCommand(B_REFS_RECEIVED);
		BMessenger tracker("application/x-vnd.Be-TRAK");

		entry_ref ref;
		folderEntry.GetRef(&ref);
		openFolderCommand.AddRef("refs", &ref);
		tracker.SendMessage(&openFolderCommand);
	}
}


static void
retrieve_label_items(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Remove label"), NULL));
	menu->AddSeparatorItem();

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Mail/labels");
	BDirectory directory(path.Path());
	BObjectList<BMenuItem> items;
	entry_ref ref;

	while (directory.GetNextRef(&ref) == B_OK)
		items.AddItem(new BMenuItem(ref.name, NULL));

	if (!items.IsEmpty()) {
		if (BLocale::Default()->GetCollator(&collator) == B_OK) {
			collator.SetNumericSorting(true);
			items.SortItems(compare_menu_items);
		}
		for (int32 i = 0; i < items.CountItems(); i++)
			menu->AddItem(items.ItemAt(i));
		menu->AddSeparatorItem();
	}
	menu->AddItem(new BMenuItem(B_TRANSLATE("Manage labels" B_UTF8_ELLIPSIS), NULL));
}


// #pragma mark -


extern "C" void
process_refs(entry_ref dir, BMessage* message, void* /*reserved*/)
{
	BPopUpMenu* menu = new BPopUpMenu("labels");
	retrieve_label_items(menu);

	BMenuItem* item = menu->Go(mouse_position() - BPoint(5, 5), false, true);
	if (item == NULL)
		return;

	BString label = item->Label();
	entry_ref ref;
	for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BNode node(&ref);
		BString type;
		BString previousLabel;
		if (node.InitCheck() == B_OK && node.ReadAttrString("BEOS:TYPE", &type) == B_OK
			&& (type == B_MAIL_TYPE || type == B_PARTIAL_MAIL_TYPE)) {
			if (label == B_TRANSLATE("Remove label"))
				node.RemoveAttr(B_MAIL_ATTR_LABEL);
			else if (label == B_TRANSLATE("Manage labels" B_UTF8_ELLIPSIS))
				open_labels_folder();
			else if (node.ReadAttrString(B_MAIL_ATTR_LABEL, &previousLabel) != B_OK
				|| previousLabel != label)
				node.WriteAttrString(B_MAIL_ATTR_LABEL, &label);
		}
	}
}
