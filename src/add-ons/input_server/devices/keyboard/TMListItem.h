// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        TMListItem.h
//  Author:      Jérôme Duval
//  Description: Keyboard input server addon
//  Created :    October 24, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef TMLISTITEM_H
#define TMLISTITEM_H

#include <Bitmap.h>
#include <ListItem.h>
#include <Path.h>

class TMListItem : public BListItem 
{
public:
	TMListItem(team_info &info);
	~TMListItem();

	virtual void DrawItem(BView *owner, BRect frame, bool complete = false);
	virtual void Update(BView *owner, const BFont *finfo);
	const team_info	*GetInfo();
	
	const BBitmap *LargeIcon() { return &fLargeIcon; };
	const BPath *Path() { return &fPath; };
	bool		IsSystemServer();
	bool		fFound;
private:
	team_info	fInfo;
	BBitmap		fIcon, fLargeIcon;
	BPath		fPath;
};


#endif //TMLISTITEM_H
