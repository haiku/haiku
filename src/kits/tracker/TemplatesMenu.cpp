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
#include <Directory.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Message.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <String.h>

#include <kernel/fs_attr.h>

#include "Attributes.h"
#include "Commands.h"

#include "IconMenuItem.h"
#include "MimeTypes.h"
#include "TemplatesMenu.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TemplatesMenu"


namespace BPrivate {

const char* kTemplatesDirectory = "Tracker/Tracker New Templates";

} // namespace BPrivate


//	#pragma mark - TemplatesMenu


TemplatesMenu::TemplatesMenu(const BMessenger &target, const char* label)
	:
	BMenu(label),
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
	if (result != B_OK)
		return result;

	for (int i = 0; i < CountItems(); i++) {
		BMenu* submenu = ItemAt(i)->Submenu();
		if (submenu != NULL) {
			result = SetTargetForSubmenuItems(submenu, target);
			if (result != B_OK)
				return result;
		}
	}

	if (fOpenItem)
		fOpenItem->SetTarget(be_app_messenger);

	return result;
}


status_t
TemplatesMenu::SetTargetForItems(BMessenger messenger)
{
	status_t result = BMenu::SetTargetForItems(messenger);
	if (result != B_OK)
		return result;

	for (int i = 0; i < CountItems(); i++) {
		BMenu* submenu = ItemAt(i)->Submenu();
		if (submenu != NULL) {
			result = SetTargetForSubmenuItems(submenu, messenger);
			if (result != B_OK)
				return result;
		}
	}

	if (fOpenItem)
		fOpenItem->SetTarget(be_app_messenger);

	return result;
}


bool
TemplatesMenu::BuildMenu(bool addItems)
{
	// clear everything...
	fOpenItem = NULL;
	int32 count = CountItems();
	while (count--)
		delete RemoveItem((int32)0);

	// add the folder
	IconMenuItem* menuItem = new IconMenuItem(B_TRANSLATE("New folder"),
		new BMessage(kNewFolder), B_DIR_MIMETYPE, B_MINI_ICON);
	AddItem(menuItem);
	menuItem->SetShortcut('N', 0);
	AddSeparatorItem();

	// the templates folder
	BPath path;
	find_directory (B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append(kTemplatesDirectory);
	mkdir(path.Path(), 0777);

	count = 0;

	count += IterateTemplateDirectory(addItems, &path, this);

	AddSeparatorItem();

	// this is the message sent to open the templates folder
	BDirectory templatesDir(path.Path());
	BMessage* message = new BMessage(B_REFS_RECEIVED);
	BEntry entry;
	entry_ref dirRef;
	if (templatesDir.GetEntry(&entry) == B_OK)
		entry.GetRef(&dirRef);

	message->AddRef("refs", &dirRef);

	// add item to show templates folder
	fOpenItem = new BMenuItem(B_TRANSLATE("Edit templates" B_UTF8_ELLIPSIS), message);
	AddItem(fOpenItem);
	if (dirRef == entry_ref())
		fOpenItem->SetEnabled(false);

	return count > 0;
}


void
TemplatesMenu::UpdateMenuState()
{
	BuildMenu(false);
}


int
TemplatesMenu::IterateTemplateDirectory(bool addItems, BPath* path, BMenu* menu)
{
	uint32 count = 0;
	if (!path || !menu)
		return count;

	BEntry entry;
	BList subMenus;
	BList subDirs;
	BList files;
	BDirectory templatesDir(path->Path());
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
				count++;

				// If not adding items, we are just seeing if there
				// are any to list.  So if we find one, immediately
				// bail and return the result.
				if (!addItems)
					return count;

				entry_ref ref;
				entry.GetRef(&ref);

				// Check if the template is a directory
				BDirectory dir(&entry);
				if (dir.InitCheck() == B_OK) {
					// check if the directory is a submenu, aka has kAttrTemplateSubMenu
					// (_trk/_template_submenu) attribute
					attr_info attrInfo;
					if (node.GetAttrInfo(kAttrTemplateSubMenu, &attrInfo) == B_OK) {
						ssize_t size;
						bool value;
						size = node.ReadAttr(kAttrTemplateSubMenu, B_BOOL_TYPE, 0, &value,
							sizeof(bool));
						if (size == sizeof(bool) && value == true) {
							// if submenu add it to subMenus list and iterate contents
							BPath subdirPath;
							if (entry.GetPath(&subdirPath) == B_OK) {
								BMenu* subMenu = new BMenu(fileName);
								count += IterateTemplateDirectory(addItems, &subdirPath, subMenu);
								subMenus.AddItem((void*)subMenu);
								continue;
							}
							continue;
						}
					} else {
						// Otherwise add it to subDirs list
						BMessage* message = new BMessage(kNewEntryFromTemplate);
						message->AddRef("refs_template", &ref);
						message->AddString("name", fileName);
						subDirs.AddItem(new IconMenuItem(fileName, message, &nodeInfo,
							B_MINI_ICON));
						continue;
					}
				}

				// Add template files to files list
				BMessage* message = new BMessage(kNewEntryFromTemplate);
				message->AddRef("refs_template", &ref);
				message->AddString("name", fileName);
				files.AddItem(new IconMenuItem(fileName, message, &nodeInfo, B_MINI_ICON));
			}
		}
	}

	// Add submenus to menu
	for (int32 i = 0; i < subMenus.CountItems(); i++)
		menu->AddItem((BMenu*)subMenus.ItemAt(i));

	if (subMenus.CountItems() > 0)
		menu->AddSeparatorItem();

	// Add subdirs to menu
	for (int32 i = 0; i < subDirs.CountItems(); i++)
		menu->AddItem((BMenuItem*)subDirs.ItemAt(i));

	if (subDirs.CountItems() > 0)
		menu->AddSeparatorItem();

	// Add files to menu
	for (int32 i = 0; i < files.CountItems(); i++)
		menu->AddItem((BMenuItem*)files.ItemAt(i));

	return count > 0;
}


status_t
TemplatesMenu::SetTargetForSubmenuItems(BMenu* menu, BMessenger messenger)
{
	if (!menu)
		return B_ERROR;

	status_t result;

	result = menu->SetTargetForItems(messenger);
	if (result != B_OK)
		return result;

	for (int i = 0; i < menu->CountItems(); i++) {
		BMenu* submenu = menu->ItemAt(i)->Submenu();
		if (submenu != NULL) {
			result = SetTargetForSubmenuItems(submenu, messenger);
			if (result != B_OK)
				return result;
		}
	}
	return result;
}


status_t
TemplatesMenu::SetTargetForSubmenuItems(BMenu* menu, BHandler* target)
{
	if (!menu || !target)
		return B_ERROR;

	status_t result;

	result = menu->SetTargetForItems(target);
	if (result != B_OK)
		return result;

	for (int i = 0; i < menu->CountItems(); i++) {
		BMenu* submenu = menu->ItemAt(i)->Submenu();
		if (submenu != NULL) {
			result = SetTargetForSubmenuItems(submenu, target);
			if (result != B_OK)
				return result;
		}
	}
	return result;
}
