/*
 * Copyright 2014, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */


#include "BookmarkBar.h"

#include <Directory.h>
#include <Entry.h>
#include <IconMenuItem.h>
#include <Messenger.h>
#include <Window.h>

#include "NavMenu.h"


BookmarkBar::BookmarkBar(const char* title, BHandler* target,
	const entry_ref* navDir)
	: BMenuBar(title)
{
	BEntry(navDir).GetNodeRef(&fNodeRef);
}


BookmarkBar::~BookmarkBar()
{
	stop_watching(BMessenger(this));
}


void
BookmarkBar::AttachedToWindow()
{
	BMenuBar::AttachedToWindow();
	watch_node(&fNodeRef, B_WATCH_DIRECTORY, BMessenger(this));

	// Enumerate initial directory content
	BDirectory dir(&fNodeRef);
	BEntry bookmark;
	while(dir.GetNextEntry(&bookmark) == B_OK) {
		node_ref ref;
		bookmark.GetNodeRef(&ref);
		_AddItem(ref.node, &bookmark);
	}
}


void
BookmarkBar::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode = message->FindInt32("opcode");
			ino_t inode = message->FindInt64("node");
			switch(opcode) {
				case B_ENTRY_CREATED:
				{
					entry_ref ref;
					const char *name;

					message->FindInt32("device", &ref.device);
					message->FindInt64("directory", &ref.directory);
					message->FindString("name", &name);
					ref.set_name(name);

					BEntry entry(&ref);
					_AddItem(inode, &entry);
					break;
				}
				case B_ENTRY_MOVED:
				{
					if (fItemsMap[inode] == NULL)
					{
						entry_ref ref;
						const char *name;

						message->FindInt32("device", &ref.device);
						message->FindInt64("to directory", &ref.directory);
						message->FindString("name", &name);
						ref.set_name(name);

						BEntry entry(&ref);
						_AddItem(inode, &entry);
						break;
					} else {
						// Existing item. Check if it's a rename or a move
						// to some other folder.
						ino_t from, to;
						message->FindInt64("to directory", &to);
						message->FindInt64("from directory", &from);
						if (from == to)
						{
							const char *name;
							message->FindString("name", &name);
							fItemsMap[inode]->SetLabel(name);
							break;
						}
					}

					// fall through: the item was moved from here to
					// elsewhere, remove it from the bar.
				}
				case B_ENTRY_REMOVED:
				{
					IconMenuItem* item = fItemsMap[inode];
					RemoveItem(item);
					fItemsMap.erase(inode);
					delete item;
				}
			}
			return;
		}
	}

	BMenuBar::MessageReceived(message);
}


void
BookmarkBar::_AddItem(ino_t inode, BEntry* entry)
{
	char name[B_FILE_NAME_LENGTH];
	entry->GetName(name);

	// make sure the item doesn't already exists
	if (fItemsMap[inode] != NULL)
		return;
		
	entry_ref ref;
	entry->GetRef(&ref);

	IconMenuItem* item = NULL;

	if(entry->IsDirectory()) {
		BNavMenu* menu = new BNavMenu(name, B_REFS_RECEIVED, Window());
		menu->SetNavDir(&ref);
		item = new IconMenuItem(menu, NULL,
			"application/x-vnd.Be-directory", B_MINI_ICON);

	} else {
		BNode node(entry);
		BNodeInfo info(&node);

		BMessage* message = new BMessage(B_REFS_RECEIVED);
		message->AddRef("refs", &ref);
		item = new IconMenuItem(name, message, &info,
			B_MINI_ICON);
	}

	BMenuBar::AddItem(item);
	fItemsMap[inode] = item;
}
