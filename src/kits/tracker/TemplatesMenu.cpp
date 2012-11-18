/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include <Application.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <Locale.h>
#include <Mime.h>
#include <Message.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <MenuItem.h>
#include <stdio.h>

#include "Commands.h"

#include "TemplatesMenu.h"
#include "IconMenuItem.h"
#include "MimeTypes.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TemplatesMenu"


namespace BPrivate {

const char* kTemplatesDirectory = "Tracker/Tracker New Templates";

} // namespace BPrivate

TemplatesMenu::TemplatesMenu(const BMessenger &target, const char* label)
	:	BMenu(label),
		fTarget(target),
		fOpenItem(NULL)
{
}


TemplatesMenu::~TemplatesMenu()
{
}


void
TemplatesMenu::AttachedToWindow()
{
	BuildMenu();
	BMenu::AttachedToWindow();
	SetTargetForItems(fTarget);
}


status_t
TemplatesMenu::SetTargetForItems(BHandler* target)
{
	status_t result = BMenu::SetTargetForItems(target);
	if (fOpenItem)
		fOpenItem->SetTarget(be_app_messenger);

	return result;
}


status_t
TemplatesMenu::SetTargetForItems(BMessenger messenger)
{
	status_t result = BMenu::SetTargetForItems(messenger);
	if (fOpenItem)
		fOpenItem->SetTarget(be_app_messenger);

	return result;
}


void
TemplatesMenu::UpdateMenuState()
{
	BuildMenu(false);
}


bool
TemplatesMenu::BuildMenu(bool addItems)
{
	// Clear everything...
	fOpenItem = NULL;
	int32 count = CountItems();
	while (count--)
		delete RemoveItem((int32)0);

	// Add the Folder
	IconMenuItem* menuItem = new IconMenuItem(B_TRANSLATE("New folder"),
		new BMessage(kNewFolder), B_DIR_MIMETYPE, B_MINI_ICON);
	AddItem(menuItem);
	menuItem->SetShortcut('N', 0);

	// The Templates folder
	BPath path;
	find_directory (B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append(kTemplatesDirectory);
	mkdir(path.Path(), 0777);

	count = 0;

	BEntry entry;
	BDirectory templatesDir(path.Path());
	while (templatesDir.GetNextEntry(&entry) == B_OK) {
		BNode node(&entry);
		BNodeInfo nodeInfo(&node);
		char fileName[B_FILE_NAME_LENGTH];
		entry.GetName(fileName);
		if (nodeInfo.InitCheck() == B_OK) {
			char mimeType[B_MIME_TYPE_LENGTH];
			nodeInfo.GetType(mimeType);

			BMimeType mime(mimeType);
			if (mime.IsValid()) {
				if (count == 0)
					AddSeparatorItem();

				count++;

				// If not adding items, we are just seeing if there
				// are any to list.  So if we find one, immediately
				// bail and return the result.
				if (!addItems)
					break;

				entry_ref ref;
				entry.GetRef(&ref);
	
				BMessage* message = new BMessage(kNewEntryFromTemplate);
				message->AddRef("refs_template", &ref);
				message->AddString("name", fileName);
				AddItem(new IconMenuItem(fileName, message, &nodeInfo,
					B_MINI_ICON));
			}
		}
	}

	AddSeparatorItem();

	// This is the message sent to open the templates folder.
	BMessage* message = new BMessage(B_REFS_RECEIVED);
	entry_ref dirRef;
	if (templatesDir.GetEntry(&entry) == B_OK)
		entry.GetRef(&dirRef);
	message->AddRef("refs", &dirRef);

	// Add item to show templates folder.
	fOpenItem =	new BMenuItem(B_TRANSLATE("Edit templates" B_UTF8_ELLIPSIS),
			message);
	AddItem(fOpenItem);
	if (dirRef == entry_ref())
		fOpenItem->SetEnabled(false);

	return count > 0;
}
