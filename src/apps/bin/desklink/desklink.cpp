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

#include <View.h>
#include <Dragger.h>
#include <Message.h>
#include <Alert.h>
#include <Bitmap.h>
#include <stdio.h>
#include <Application.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Roster.h>
#include <Deskbar.h>
#include <strings.h>
#include <List.h>
#include <String.h>
#include "VolumeSlider.h"
#include "DeskButton.h"
#include "iconfile.h"

#define MEDIA_SETTINGS 'mese'
#define SOUND_SETTINGS 'sose'
#define OPEN_MEDIA_PLAYER 'omep'

const char *app_signature = "application/x-vnd.be.desklink";
// the application signature used by the replicant to find the supporting
// code

class _EXPORT VCButton;
	// the dragger part has to be exported

class VCButton : public BView {
public:
	VCButton(BRect frame, const char *name,
		uint32 resizeMask = B_FOLLOW_ALL, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED);
		
	VCButton(BMessage *);
		// BMessage * based constructor needed to support archiving
	virtual ~VCButton();

	// archiving overrides
	static VCButton *Instantiate(BMessage *data);
	virtual	status_t Archive(BMessage *data, bool deep = true) const;


	// misc BView overrides
	virtual void MouseDown(BPoint);
	virtual void MouseUp(BPoint);
	
	virtual void Draw(BRect );

	virtual void MessageReceived(BMessage *);
private:
	BBitmap *		segments;
	VolumeSlider	*volumeSlider;
};

//
//	This is the exported function that will be used by Deskbar
//	to create and add the replicant
//
extern "C" _EXPORT BView* instantiate_deskbar_item();

BView *
instantiate_deskbar_item()
{
	return new VCButton(BRect(0, 0, 16, 16), "VCButton");
}


VCButton::VCButton(BRect frame, const char *name,
	uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags)
{
	// Background Bitmap
	segments = new BBitmap(BRect(0, 0, kSpeakerWidth - 1, kSpeakerHeight - 1), B_COLOR_8_BIT);
	segments->SetBits(kSpeakerBits, kSpeakerWidth*kSpeakerHeight, 0, B_COLOR_8_BIT);
	// Background Color
	SetViewColor(184,184,184);

	//add dragger
	BRect rect(Bounds());
	rect.left = rect.right-7.0; 
	rect.top = rect.bottom-7.0;
	BDragger *dragger = new BDragger(rect, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
	dragger->SetViewColor(B_TRANSPARENT_32_BIT);
}


VCButton::VCButton(BMessage *message)
	:	BView(message),
		volumeSlider(NULL)
{
	// Background Bitmap
	segments = new BBitmap(BRect(0, 0, 16 - 1, 16 - 1), B_COLOR_8_BIT);
	segments->SetBits(kSpeakerBits, 16*16, 0, B_COLOR_8_BIT);
}


VCButton::~VCButton()
{
	delete segments;
}


// archiving overrides
VCButton *
VCButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "VCButton"))
		return NULL;
	return new VCButton(data);
}


status_t 
VCButton::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddString("add_on", app_signature);
	return B_NO_ERROR;
}


void
VCButton::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			(new BAlert("About Volume Control", "Volume Control (Replicant)\n"
				"  Brought to you by Jérôme DUVAL.\n\n"
				"OpenBeOS, 2003","OK"))->Go();
			break;
		case OPEN_MEDIA_PLAYER:
			// launch the media player app
			be_roster->Launch("application/x-vnd.Be.MediaPlayer");
			break;
		case MEDIA_SETTINGS:
			// launch the media prefs app
			be_roster->Launch("application/x-vnd.Be.MediaPrefs");
			break;
		case SOUND_SETTINGS:
			// launch the sounds prefs app
			be_roster->Launch("application/x-vnd.Be.SoundsPrefs");
			break;
		default:
			BView::MessageReceived(message);
			break;		
	}
}


void 
VCButton::Draw(BRect rect)
{
	BView::Draw(rect);
	
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(segments);
}


void
VCButton::MouseDown(BPoint point)
{
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);
	
	where = ConvertToScreen(point);
	
	if (mouseButtons & B_SECONDARY_MOUSE_BUTTON) {
		BPopUpMenu *menu = new BPopUpMenu("", false, false);
		menu->SetFont(be_plain_font);
		menu->AddItem(new BMenuItem("Media Settings...", new BMessage(MEDIA_SETTINGS)));
		menu->AddItem(new BMenuItem("Sound Settings...", new BMessage(SOUND_SETTINGS)));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Open MediaPlayer", new BMessage(OPEN_MEDIA_PLAYER)));
		
		menu->SetTargetForItems(this);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4), 
			where + BPoint(4, 4)));
	} else if (mouseButtons & B_PRIMARY_MOUSE_BUTTON) {
		// Show VolumeSlider
		volumeSlider = new VolumeSlider(BRect(where.x,where.y,where.x+207,where.y+19));
		volumeSlider->Show();
	}
}


void
VCButton::MouseUp(BPoint point)
{
	/* don't Quit() ! thanks for FFM users */
}




int
main(int, char **argv)
{	
	BApplication app(app_signature);
	bool atLeastOnePath = false;
	BList titleList;
	BList actionList;
		
	for(int32 i=1; argv[i]!=NULL; i++) {
		
		if (strncmp(argv[i], "cmd=", 4) == 0) {
			BString *title = new BString(argv[i] + 4);
			int32 index = title->FindFirst(':');
			if(index<=0) {
				printf("desklink: usage: cmd=title:action\n");
			} else {
				title->Truncate(index);
				BString *action = new BString(argv[i] + 4);
				action->Remove(0, index+1);
				titleList.AddItem(title);
				actionList.AddItem(action);
			}
			continue;
		}
		
		atLeastOnePath = true;
	
		BEntry entry(argv[i], true);
		if(!entry.Exists()) {
			printf("desklink: cannot find '%s'\n", argv[i]);
			return 1;
		}
		
		entry_ref ref;
		entry.GetRef(&ref);
		
		status_t err;
		err = BDeskbar().AddItem(new DeskButton(BRect(0, 0, 16, 16), &ref, "DeskButton",
			titleList, actionList));
	
		if(err!=B_OK) {
			printf("desklink: Deskbar refuses link to '%s': %s\n", argv[i], strerror(err));	
		}
		
		titleList.MakeEmpty();
		actionList.MakeEmpty();
	}
	
	if (!atLeastOnePath) {
		// print a simple usage string
		printf(	"usage: desklink { [ cmd=title:action ... ] path } ...\n");
		return 1;
	}
	
	return 0;
}
