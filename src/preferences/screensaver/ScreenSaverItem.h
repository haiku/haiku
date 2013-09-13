/*
 * Copyright 2005-2013 Haiku, Inc. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 */
#ifndef SCREEN_SAVER_ITEM_H
#define SCREEN_SAVER_ITEM_H


#include <ListItem.h>
#include <String.h>
#include <Entry.h>


class ScreenSaverItem : public BStringItem {
public:
	ScreenSaverItem(const char* eventName, const char* path)
		: BStringItem(eventName), fPath(path) {}

	const char* Path() const { return fPath.String(); }

private:
	BString fPath;
};


#endif	// SCREEN_SAVER_ITEM_H
