// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2005, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        ScreenSaverItem.h
//  Author:      Jérôme Duval
//  Description: ScreenSaver Preferences
//  Created :    July 28, 2005
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef __SCREENSAVERITEM_H__
#define __SCREENSAVERITEM_H__

#include <StringItem.h>
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
