// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  Program:	 desklink
//  Author:      Jérôme DUVAL
//  Description: VolumeControl and link items in Deskbar
//  Created :    October 20, 2003
//	Modified by: Jérome Duval
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef DESKBUTTON_H
#define DESKBUTTON_H

#include <View.h>
#include <List.h>
#include <Entry.h>

class DeskButton : public BView {
public:
	DeskButton(BRect frame, entry_ref *ref, const char *name, BList &titleList, BList &actionList,
		uint32 resizeMask = B_FOLLOW_ALL, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
		
	DeskButton(BMessage *);
		// BMessage * based constructor needed to support archiving
	virtual ~DeskButton();
	
	// archiving overrides
	static DeskButton *Instantiate(BMessage *data);
	virtual	status_t Archive(BMessage *data, bool deep = true) const;

	// misc BView overrides
	virtual void MouseDown(BPoint);
	
	virtual void Draw(BRect );

	virtual void MessageReceived(BMessage *);
private:
	BBitmap *		segments;
	entry_ref		ref;
	BList 			actionList, titleList;
};


#endif
