/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Jérôme Duval, jerome.duval@free.fr
 */

#ifndef __SCREENSAVERITEM_H__
#define __SCREENSAVERITEM_H__

#include <ListItem.h>
#include <String.h>
#include <Entry.h>


class ScreenSaverItem : public BStringItem {
public:
		ScreenSaverItem(const char* event_name,const char* path) 
			: BStringItem(event_name), fPath(path) {
		
		}
		
		const char*		Path() const { return fPath.String();}
private:
			BString		fPath;
};
#endif
