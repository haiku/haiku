// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        HEventItem.h, Oliver Ruiz Dorantes, Atsushi Takamatsu
//  Author:      Jérôme Duval
//  Description: Sounds Preferences
//  Created :    November 24, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef __HEVENTITEM_H__
#define __HEVENTITEM_H__

#include <ListItem.h>
#include <String.h>
#include <Entry.h>


class HEventItem : public BListItem {
public:
						HEventItem(const char* event_name
									,const char* path);
		virtual			~HEventItem();
		virtual	void 	DrawItem(BView *owner, BRect itemRect, 
							bool drawEverything = false);
		
		const char*		Name()const { return fName.String();}
		const char*		Path()const { return fPath.String();}
			void		Remove(const char *type);
			void		SetPath(const char* path);
protected:

private:
			BString		fName;
			BString		fPath;
			float		fText_offset;
};
#endif
