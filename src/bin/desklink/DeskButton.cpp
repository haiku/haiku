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

#include <Dragger.h>
#include <Bitmap.h>
#include <Message.h>
#include <String.h>
#include <Alert.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Roster.h>
#include <stdlib.h>
#include "DeskButton.h"

#define OPEN_REF	'opre'
#define DO_ACTION	'doac'

extern char *app_signature;

DeskButton::DeskButton(BRect frame, entry_ref *entry_ref, const char *name, BList &titles, BList &actions,
		uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags),
		ref(*entry_ref),
		actionList(actions),
		titleList(titles)
{
	// Background Color
	SetViewColor(184,184,184);

	//add dragger
	BRect rect(Bounds());
	rect.left = rect.right-7.0; 
	rect.top = rect.bottom-7.0;
	BDragger *dragger = new BDragger(rect, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
	dragger->SetViewColor(B_TRANSPARENT_32_BIT);

	segments = new BBitmap(BRect(0,0,15,15), B_CMAP8);
	BNodeInfo::GetTrackerIcon(&ref, segments, B_MINI_ICON);
}


DeskButton::DeskButton(BMessage *message)
	:	BView(message)	
{
	message->FindRef("ref", &ref);
	
	BString title, action;
	int32 index = 0;
	while(message->FindString("title", index, &title)==B_OK 
		&& message->FindString("action", index, &action)==B_OK) {
		titleList.AddItem(new BString(title));
		actionList.AddItem(new BString(action));
		index++;
	}
	
	segments = new BBitmap(BRect(0,0,15,15), B_CMAP8);
	BNodeInfo::GetTrackerIcon(&ref, segments, B_MINI_ICON);
}


DeskButton::~DeskButton()
{
	delete segments;
}


// archiving overrides
DeskButton *
DeskButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "DeskButton"))
		return NULL;
	return new DeskButton(data);
}


status_t 
DeskButton::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	
	data->AddRef("ref", &ref);
	
	for(int32 i=0; i<titleList.CountItems() &&
		i<actionList.CountItems(); i++) {
		data->AddString("title", *(BString*)titleList.ItemAt(i));
		data->AddString("action", *(BString*)actionList.ItemAt(i));
	}

	data->AddString("add_on", app_signature);
	return B_NO_ERROR;
}
	
	
void
DeskButton::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			(new BAlert("About Desklink", "Desklink (Replicant)\n"
				"  Brought to you by Jérôme DUVAL.\n\n"
				"OpenBeOS, 2003","OK"))->Go();
			break;
		case OPEN_REF:
			be_roster->Launch(&ref);
			break;
		case DO_ACTION:
		{
			BString action;
			if(message->FindString("action", &action)==B_OK) {
				action += " &";
				system(action.String());
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;		
	}
}


void 
DeskButton::Draw(BRect rect)
{
	BView::Draw(rect);
	
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(segments);
}


void
DeskButton::MouseDown(BPoint point)
{
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);
	
	where = ConvertToScreen(point);
	
	if (mouseButtons & B_SECONDARY_MOUSE_BUTTON) {
		BString label = "Open ";
		label += ref.name;
		BPopUpMenu *menu = new BPopUpMenu("", false, false);
		menu->SetFont(be_plain_font);
		menu->AddItem(new BMenuItem(label.String(), new BMessage(OPEN_REF)));
		if(titleList.CountItems()>0 && actionList.CountItems()>0) {
			menu->AddSeparatorItem();
			for(int32 i=0; i<titleList.CountItems() && i<actionList.CountItems(); i++) {
				BMessage *message = new BMessage(DO_ACTION);
				message->AddString("action", *(BString*)actionList.ItemAt(i));
				menu->AddItem(new BMenuItem(((BString*)titleList.ItemAt(i))->String(), message));
			}
		}

		menu->SetTargetForItems(this);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4), 
			where + BPoint(4, 4)));
	} else if (mouseButtons & B_PRIMARY_MOUSE_BUTTON) {
		be_roster->Launch(&ref);
	}
}
