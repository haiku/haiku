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
//  Modified by: François Revol, 10/31/2003
//  Modified by: Marcus Overhagen, 15/08/2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <FindDirectory.h>
#include <File.h>
#include <List.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <View.h>
#include <stdio.h>
#include <strings.h>
#include "VolumeSlider.h"
#include "DeskButton.h"
#include "iconfile.h"

#define MEDIA_SETTINGS 'mese'
#define SOUND_SETTINGS 'sose'
#define OPEN_MEDIA_PLAYER 'omep'
#define TOGGLE_DONT_BEEP 'tdbp'
#define SET_VOLUME_WHICH 'svwh'

#define VOLUME_CTL_NAME "MediaReplicant" /* R5 name needed, Media prefs manel removes by name */

#define SETTINGS_FILE "x-vnd.OBOS.DeskbarVolumeControlSettings"

const char *app_signature = "application/x-vnd.be.desklink";
// the application signature used by the replicant to find the supporting
// code

class _EXPORT MediaReplicant;
	// the dragger part has to be exported

class MediaReplicant : public BView {
public:
	MediaReplicant(BRect frame, const char *name,
		uint32 resizeMask = B_FOLLOW_ALL, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
		
	MediaReplicant(BMessage *);
		// BMessage * based constructor needed to support archiving
	virtual ~MediaReplicant();

	// archiving overrides
	static MediaReplicant *Instantiate(BMessage *data);
	virtual	status_t Archive(BMessage *data, bool deep = true) const;


	// misc BView overrides
	virtual void MouseDown(BPoint);
	virtual void MouseUp(BPoint);
	
	virtual void Draw(BRect );

	virtual void MessageReceived(BMessage *);
private:
	status_t LaunchByPath(const char *path);
	status_t LaunchBySig(const char *sig);
	void LoadSettings();
	void SaveSettings();
	BBitmap *		segments;
	VolumeSlider	*volumeSlider;
	bool confDontBeep; /* don't beep on volume change */
	int32 confVolumeWhich; /* which volume parameter to act on (Mixer/Phys.Output) */
};

//
//	This is the exported function that will be used by Deskbar
//	to create and add the replicant
//
extern "C" _EXPORT BView* instantiate_deskbar_item();

BView *
instantiate_deskbar_item()
{
	return new MediaReplicant(BRect(0, 0, 16, 16), VOLUME_CTL_NAME);
}


MediaReplicant::MediaReplicant(BRect frame, const char *name,
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
	LoadSettings();
}


MediaReplicant::MediaReplicant(BMessage *message)
	:	BView(message),
		volumeSlider(NULL)
{
	// Background Bitmap
	segments = new BBitmap(BRect(0, 0, 16 - 1, 16 - 1), B_COLOR_8_BIT);
	segments->SetBits(kSpeakerBits, 16*16, 0, B_COLOR_8_BIT);
	LoadSettings();
}


MediaReplicant::~MediaReplicant()
{
	delete segments;
	SaveSettings();
}


// archiving overrides
MediaReplicant *
MediaReplicant::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, VOLUME_CTL_NAME))
		return NULL;
	return new MediaReplicant(data);
}


status_t 
MediaReplicant::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddString("add_on", app_signature);
	return B_NO_ERROR;
}


void
MediaReplicant::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case B_ABOUT_REQUESTED:
		(new BAlert("About Volume Control", "Volume Control (Replicant)\n"
			    "  Brought to you by Jérôme DUVAL.\n\n"
			    "OpenBeOS, 2003","OK"))->Go();
		break;
	case OPEN_MEDIA_PLAYER:
		// launch the media player app
		if (B_OK == LaunchBySig("application/x-vnd.Haiku.MediaPlayer"))
			break;
		if (B_OK == LaunchBySig("application/x-vnd.Be.MediaPlayer"))
			break;
		if (B_OK == LaunchByPath("/boot/beos/apps/MediaPlayer"))
			break;
		(new BAlert("desklink", "Couldn't launch MediaPlayer", "OK"))->Go();
		break;
	case MEDIA_SETTINGS:
		// launch the media prefs app
		if (B_OK == LaunchBySig("application/x-vnd.Haiku.MediaPrefs"))
			break;
		if (B_OK == LaunchBySig("application/x-vnd.Be.MediaPrefs"))
			break;
		if (B_OK == LaunchByPath("/boot/home/config/be/Preferences/Media"))
			break;
		(new BAlert("desklink", "Couldn't launch Media Preferences", "OK"))->Go();
		break;
	case SOUND_SETTINGS:
		// launch the sounds prefs app
		if (B_OK == LaunchBySig("application/x-vnd.Haiku.SoundsPrefs"))
			break;
		if (B_OK == LaunchBySig("application/x-vnd.Be.SoundsPrefs"))
			break;
		if (B_OK == LaunchByPath("/boot/home/config/be/Preferences/Sounds"))
			break;
		(new BAlert("desklink", "Couldn't launch Sounds Preferences", "OK"))->Go();
		break;
	case TOGGLE_DONT_BEEP:
		confDontBeep = !confDontBeep;
		break;
	case SET_VOLUME_WHICH:
		message->FindInt32("volwhich", &confVolumeWhich);
		break;
	default:
		BView::MessageReceived(message);
		break;		
	}
}


status_t
MediaReplicant::LaunchByPath(const char *path)
{
	BEntry ent;
	entry_ref ref;
	app_info appInfo;
	status_t err;

	err = ent.SetTo(path);
	if (err)
		return err;
	err = ent.GetRef(&ref);
	if (err)
		return err;
	err = be_roster->Launch(&ref);
	if (err != B_ALREADY_RUNNING)
		return err; // should be B_OK or fatal error
	err = be_roster->GetAppInfo(&ref, &appInfo);
	if (err)
		return err;
	return be_roster->ActivateApp(appInfo.team);
}


status_t
MediaReplicant::LaunchBySig(const char *sig)
{
	app_info appInfo;
	status_t err;

	err = be_roster->Launch(sig);
	if (err != B_ALREADY_RUNNING)
		return err; // should be B_OK or fatal error
	err = be_roster->GetAppInfo(sig, &appInfo);
	if (err)
		return err;
	return be_roster->ActivateApp(appInfo.team);
}


void 
MediaReplicant::Draw(BRect rect)
{
	BView::Draw(rect);
	
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(segments);
}


void
MediaReplicant::MouseDown(BPoint point)
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
		menu->AddSeparatorItem();
		BMenuItem *tmpItem = new BMenuItem("Don't beep", new BMessage(TOGGLE_DONT_BEEP));
		menu->AddItem(tmpItem);
		tmpItem->SetMarked(confDontBeep);
		BMenu *volMenu = new BMenu("Act On");
		volMenu->SetFont(be_plain_font);
		BMessage *msg;
		msg = new BMessage(SET_VOLUME_WHICH);
		msg->AddInt32("volwhich", VOLUME_USE_MIXER);
		tmpItem = new BMenuItem("System Mixer", msg);
		tmpItem->SetMarked(confVolumeWhich == VOLUME_USE_MIXER);
		volMenu->AddItem(tmpItem);
		msg = new BMessage(SET_VOLUME_WHICH);
		msg->AddInt32("volwhich", VOLUME_USE_PHYS_OUTPUT);
		tmpItem = new BMenuItem("Physical Output", msg);
		volMenu->AddItem(tmpItem);
		tmpItem->SetMarked(confVolumeWhich == VOLUME_USE_PHYS_OUTPUT);
		menu->AddItem(volMenu);
		
		menu->SetTargetForItems(this);
		volMenu->SetTargetForItems(this);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4), 
			where + BPoint(4, 4)));
	} else if (mouseButtons & B_PRIMARY_MOUSE_BUTTON) {
		// Show VolumeSlider
		volumeSlider = new VolumeSlider(BRect(where.x,where.y,where.x+207,where.y+19), confDontBeep, confVolumeWhich);
		volumeSlider->Show();
	}
}


void
MediaReplicant::MouseUp(BPoint point)
{
	/* don't Quit() ! thanks for FFM users */
}

void MediaReplicant::LoadSettings()
{
	confDontBeep = false;
	confVolumeWhich = VOLUME_USE_MIXER;
	BPath p;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p, false) < B_OK)
		return;
	p.SetTo(p.Path(), SETTINGS_FILE);
	BFile settings(p.Path(), B_READ_ONLY);
	if (settings.InitCheck() < B_OK)
		return;
	BMessage msg;
	if (msg.Unflatten(&settings) < B_OK)
		return;
	msg.FindInt32("volwhich", &confVolumeWhich);
	msg.FindBool("dontbeep", &confDontBeep);
}

void MediaReplicant::SaveSettings()
{
	BPath p;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p, false) < B_OK)
		return;
	p.SetTo(p.Path(), SETTINGS_FILE);
	BFile settings(p.Path(), B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
	if (settings.InitCheck() < B_OK)
		return;
	BMessage msg('CNFG');
	msg.AddInt32("volwhich", confVolumeWhich);
	msg.AddBool("dontbeep", confDontBeep);
	ssize_t len=0;
	if (msg.Flatten(&settings, &len) < B_OK)
		return;
}


int
main(int, char **argv)
{	
	BApplication app(app_signature);
	bool atLeastOnePath = false;
	BList titleList;
	BList actionList;
		
	for(int32 i=1; argv[i]!=NULL; i++) {

		if (strcmp(argv[i], "--list") == 0) {
			BDeskbar db;
			int32 i, found = 0, count;
			count = db.CountItems();
			printf("Deskbar items:\n");
			/* the API is doomed, so don't try to enum for too long */
			for (i = 0; (found < count) && (i >= 0) && (i < 5000); i++) {
				const char scratch[2] = ""; /* BDeskbar is buggy */
				const char *name=scratch;
				if (db.GetItemInfo(i, &name) >= B_OK) {
					found++;
					printf("Item %ld: '%s'\n", i, name);
					free((void *)name); /* INTENDED */
				}
			}
			return 0;
		}

		if (strcmp(argv[i], "--remove") == 0) {
			BDeskbar db;
			int32 found = 0;
			uint32 count = db.CountItems();
			/* BDeskbar is definitely doomed ! */
			while ((db.RemoveItem("DeskButton") == B_OK) && (db.CountItems() < count)) {
				count = db.CountItems();
				found++;
			}
			printf("removed %ld items.\n", found);
			return 0;
		}
		
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
		err = BDeskbar().AddItem(new DeskButton(BRect(0, 0, 15, 15), &ref, "DeskButton",
			titleList, actionList));
	
		if(err!=B_OK) {
			printf("desklink: Deskbar refuses link to '%s': %s\n", argv[i], strerror(err));	
		}
		
		titleList.MakeEmpty();
		actionList.MakeEmpty();
	}
	
	if (!atLeastOnePath) {
		// print a simple usage string
		printf(	"usage: desklink { [ --list|--remove|[cmd=title:action ... ] path ] } ...\n"
			"--list: list all Deskbar addons.\n"
			"--remove: delete all desklink addons.\n");
		return 1;
	}
	
	return 0;
}
