// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        HEventList.cpp
//  Author:      Jérôme Duval, Oliver Ruiz Dorantes, Atsushi Takamatsu
//  Description: Sounds Preferences
//  Created :    November 24, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "HEventList.h"
#include "HEventItem.h"

#include "HWindow.h"
#include <Bitmap.h>
#include <ClassInfo.h>
#include <MediaFiles.h>
#include <Path.h>
#include <Alert.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <stdio.h>
#include <Mime.h>
#include <Roster.h>
#include <NodeInfo.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HEventList::HEventList(BRect rect, const char* name)
	: BListView(rect, name, B_SINGLE_SELECTION_LIST,
			B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
		fType(NULL)
{
	
}

/***********************************************************
 * Destructor
 ***********************************************************/
HEventList::~HEventList()
{
	RemoveAll();
	delete fType;
}

/***********************************************************
 * Set type
 ***********************************************************/
void
HEventList::SetType(const char* type)
{
	RemoveAll();
	BMediaFiles mfiles;
	mfiles.RewindRefs(type);
	delete fType;
	fType = strdup(type);
	
	BString name;
	entry_ref ref;
	while(mfiles.GetNextRef(&name,&ref) == B_OK) {
		BPath path(&ref);
		AddItem(new HEventItem(name.String(), path.Path()));
	}
}

/***********************************************************
 * Remove all items
 ***********************************************************/
void
HEventList::RemoveAll()
{
	BListItem *item;
	while((item = RemoveItem((int32)0))!=NULL)
		delete item;
	MakeEmpty();
}


/***********************************************************
 * Selection Changed
 ***********************************************************/
void
HEventList::SelectionChanged()
{
	BListView::SelectionChanged();
	
	int32 sel = CurrentSelection();
	if(sel >= 0)
	{
		HEventItem *item = cast_as(ItemAt(sel),HEventItem);
		if(!item)
			return;
			
		entry_ref ref;	
		BMediaFiles().GetRefFor(fType, item->Name(), &ref);
		
		BPath path(&ref);
		if((path.InitCheck()==B_OK) || (ref.name == NULL) || (strcmp(ref.name, "")==0)) {
			item->SetPath(path.Path());
			InvalidateItem(sel);
		} else {
			printf("name %s\n", ref.name);
			BMediaFiles().RemoveRefFor(fType, item->Name(), ref);
			(new BAlert("alert", "No such file or directory", "Ok"))->Go();
			return;
		}
		BMessage msg(M_EVENT_CHANGED);
		msg.AddString("name",item->Name());
		
		msg.AddString("path",item->Path());
		Window()->PostMessage(&msg);
	}
}	

/***********************************************************
 * SetPath
 ***********************************************************/
void
HEventList::SetPath(const char* path)
{
	int32 sel = CurrentSelection();
	if(sel >= 0) {
		HEventItem *item = cast_as(ItemAt(sel),HEventItem);
		if(!item)
			return;
		
		entry_ref ref;
		BEntry entry(path);
		entry.GetRef(&ref);
		BMediaFiles().SetRefFor(fType, item->Name(), ref);
		
		item->SetPath(path);
		InvalidateItem(sel);			
	}
}
