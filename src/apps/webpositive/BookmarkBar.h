/*
 * Copyright 2014, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOKMARK_BAR_H
#define BOOKMARK_BAR_H


#include <map>

#include <MenuBar.h>
#include <Node.h>
#include <NodeMonitor.h>


class BEntry;

namespace BPrivate {
	class IconMenuItem;
}


class BookmarkBar: public BMenuBar {
public:
									BookmarkBar(const char* title,
										BHandler* target,
										const entry_ref* navDir);
									~BookmarkBar();

	void							AttachedToWindow();
	void							MessageReceived(BMessage* message);

private:
	void							_AddItem(ino_t inode, BEntry* entry);

private:
	node_ref						fNodeRef;
	std::map<ino_t, BPrivate::IconMenuItem*>	fItemsMap;
};


#endif // BOOKMARK_BAR_H
