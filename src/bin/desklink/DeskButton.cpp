/*
 * Copyright 2003-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Jérôme Duval
 */


#include "DeskButton.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Dragger.h>
#include <MenuItem.h>
#include <Message.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>

#include <stdlib.h>

#define OPEN_REF	'opre'
#define DO_ACTION	'doac'

extern const char *kAppSignature;
	// from desklink.cpp


DeskButton::DeskButton(BRect frame, entry_ref* ref, const char* name,
		BList& titles, BList& actions, uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask, flags),
	fRef(*ref),
	fActionList(actions),
	fTitleList(titles)
{
	// Background Color
	SetViewColor(184, 184, 184);

	//add dragger
	BRect rect(Bounds());
	rect.left = rect.right - 7.0; 
	rect.top = rect.bottom - 7.0;
	BDragger *dragger = new BDragger(rect, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
	dragger->SetViewColor(B_TRANSPARENT_32_BIT);

	fSegments = new BBitmap(BRect(0,0,15,15), B_CMAP8);
	BNodeInfo::GetTrackerIcon(&fRef, fSegments, B_MINI_ICON);
}


DeskButton::DeskButton(BMessage *message)
	:	BView(message)	
{
	message->FindRef("ref", &fRef);
	
	BString title, action;
	int32 index = 0;
	while(message->FindString("title", index, &title)==B_OK 
		&& message->FindString("action", index, &action)==B_OK) {
		fTitleList.AddItem(new BString(title));
		fActionList.AddItem(new BString(action));
		index++;
	}
	
	fSegments = new BBitmap(BRect(0,0,15,15), B_CMAP8);
	BNodeInfo::GetTrackerIcon(&fRef, fSegments, B_MINI_ICON);
}


DeskButton::~DeskButton()
{
	delete fSegments;
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
	
	data->AddRef("ref", &fRef);
	
	for (int32 i = 0; i < fTitleList.CountItems()
			&& i < fActionList.CountItems(); i++) {
		data->AddString("title", *(BString*)fTitleList.ItemAt(i));
		data->AddString("action", *(BString*)fActionList.ItemAt(i));
	}

	data->AddString("add_on", kAppSignature);
	return B_NO_ERROR;
}


void
DeskButton::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			(new BAlert("About Desklink", "Desklink (Replicant)\n"
				"  Brought to you by Jérôme DUVAL.\n\n"
				"Copyright " B_UTF8_COPYRIGHT "2003-2006, Haiku","OK"))->Go();
			break;
		case OPEN_REF:
			be_roster->Launch(&fRef);
			break;
		case DO_ACTION:
		{
			BString action;
			if (message->FindString("action", &action) == B_OK) {
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
	DrawBitmap(fSegments);
}


void
DeskButton::MouseDown(BPoint point)
{
	uint32 mouseButtons = 0;
	if (Window()->CurrentMessage() != NULL)
		mouseButtons = Window()->CurrentMessage()->FindInt32("buttons");

	BPoint where = ConvertToScreen(point);

	if (mouseButtons & B_SECONDARY_MOUSE_BUTTON) {
		BString label = "Open ";
		label += fRef.name;
		BPopUpMenu *menu = new BPopUpMenu("", false, false);
		menu->SetFont(be_plain_font);
		menu->AddItem(new BMenuItem(label.String(), new BMessage(OPEN_REF)));
		if (fTitleList.CountItems() > 0 && fActionList.CountItems() > 0) {
			menu->AddSeparatorItem();
			for (int32 i = 0; i < fTitleList.CountItems()
					&& i < fActionList.CountItems(); i++) {
				BMessage *message = new BMessage(DO_ACTION);
				message->AddString("action", *(BString*)fActionList.ItemAt(i));
				menu->AddItem(new BMenuItem(((BString*)fTitleList.ItemAt(i))->String(), message));
			}
		}

		menu->SetTargetForItems(this);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4), 
			where + BPoint(4, 4)));
	} else if (mouseButtons & B_PRIMARY_MOUSE_BUTTON) {
		be_roster->Launch(&fRef);
	}
}
