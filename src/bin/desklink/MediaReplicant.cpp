/*
 * Copyright 2003-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Marcus Overhagen
 *		Jonas Sundström
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Stephan Aßmus <superstippi@gmx.de>
 */


//! Volume control, and media shortcuts in Deskbar


#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>
#include <ToolTip.h>
#include <ToolTipManager.h>

#include "desklink.h"
#include "iconfile.h"
#include "MixerControl.h"
#include "VolumeWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaReplicant"


static const uint32 kMsgOpenMediaSettings = 'mese';
static const uint32 kMsgOpenSoundSettings = 'sose';
static const uint32 kMsgOpenMediaPlayer = 'omep';
static const uint32 kMsgToggleBeep = 'tdbp';
static const uint32 kMsgVolumeWhich = 'svwh';

static const char* kReplicantName = "MediaReplicant";
	// R5 name needed, Media prefs manel removes by name

static const char* kSettingsFile = "x-vnd.Haiku-desklink";


class VolumeToolTip : public BToolTip {
public:
	VolumeToolTip(int32 which = VOLUME_USE_MIXER)
		:
		fWhich(which)
	{
		fView = new BStringView("", "");
	}

	virtual ~VolumeToolTip()
	{
		delete fView;
	}

	virtual BView* View() const
	{
		return fView;
	}

	virtual void AttachedToWindow()
	{
		Update();
	}

	void SetWhich(int32 which)
	{
		fWhich = which;
	}

	void Update()
	{
		if (!Lock())
			return;

		MixerControl control;
		control.Connect(fWhich);

		BString text;
		text.SetToFormat(B_TRANSLATE("%g dB"), control.Volume());
		fView->SetText(text.String());

		Unlock();
	}

private:
	BStringView*	fView;
	int32			fWhich;
};


class MediaReplicant : public BView {
public:
							MediaReplicant(BRect frame, const char* name,
								uint32 resizeMask = B_FOLLOW_ALL,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
							MediaReplicant(BMessage* archive);

	virtual					~MediaReplicant();

	// archiving overrides
	static	MediaReplicant*	Instantiate(BMessage* data);
	virtual	status_t		Archive(BMessage* data, bool deep = true) const;

	// BView overrides
	virtual void			AttachedToWindow();
	virtual void			MouseDown(BPoint point);
	virtual void			Draw(BRect updateRect);
	virtual void			MessageReceived(BMessage* message);

private:
			status_t		_LaunchByPath(const char* path);
			status_t		_LaunchBySignature(const char* signature);
			void			_Launch(const char* prettyName,
								const char* signature, directory_which base,
								const char* fileName);
			void			_LoadSettings();
			void			_SaveSettings();
			void			_Init();

			BBitmap*		fIcon;
			VolumeWindow*	fVolumeSlider;
			bool 			fDontBeep;
				// don't beep on volume change
			int32 			fVolumeWhich;
				// which volume parameter to act on (Mixer/Phys.Output)
};


MediaReplicant::MediaReplicant(BRect frame, const char* name,
		uint32 resizeMask, uint32 flags)
	:
	BView(frame, name, resizeMask, flags),
	fVolumeSlider(NULL)
{
	_Init();
}


MediaReplicant::MediaReplicant(BMessage* message)
	:
	BView(message),
	fVolumeSlider(NULL)
{
	_Init();
}


MediaReplicant::~MediaReplicant()
{
	delete fIcon;
	_SaveSettings();
}


MediaReplicant*
MediaReplicant::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, kReplicantName))
		return NULL;

	return new(std::nothrow) MediaReplicant(data);
}


status_t
MediaReplicant::Archive(BMessage* data, bool deep) const
{
	status_t status = BView::Archive(data, deep);
	if (status < B_OK)
		return status;

	return data->AddString("add_on", kAppSignature);
}


void
MediaReplicant::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent)
		SetViewColor(parent->ViewColor());

	BView::AttachedToWindow();
}


void
MediaReplicant::Draw(BRect rect)
{
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIcon);
}


void
MediaReplicant::MouseDown(BPoint point)
{
	int32 buttons = B_PRIMARY_MOUSE_BUTTON;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL)
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);

	BPoint where = ConvertToScreen(point);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		BPopUpMenu* menu = new BPopUpMenu("", false, false);
		menu->SetFont(be_plain_font);

		menu->AddItem(new BMenuItem(
			B_TRANSLATE("Media preferences" B_UTF8_ELLIPSIS),
			new BMessage(kMsgOpenMediaSettings)));
		menu->AddItem(new BMenuItem(
			B_TRANSLATE("Sound preferences" B_UTF8_ELLIPSIS),
			new BMessage(kMsgOpenSoundSettings)));

		menu->AddSeparatorItem();

		menu->AddItem(new BMenuItem(B_TRANSLATE("Open MediaPlayer"),
			new BMessage(kMsgOpenMediaPlayer)));

		menu->AddSeparatorItem();

		BMenu* subMenu = new BMenu(B_TRANSLATE("Options"));
		menu->AddItem(subMenu);

		BMenuItem* item = new BMenuItem(B_TRANSLATE("Control physical output"),
			new BMessage(kMsgVolumeWhich));
		item->SetMarked(fVolumeWhich == VOLUME_USE_PHYS_OUTPUT);
		subMenu->AddItem(item);

		item = new BMenuItem(B_TRANSLATE("Beep"),
			new BMessage(kMsgToggleBeep));
		item->SetMarked(!fDontBeep);
		subMenu->AddItem(item);

		menu->SetTargetForItems(this);
		subMenu->SetTargetForItems(this);

		menu->Go(where, true, true, BRect(where - BPoint(4, 4),
			where + BPoint(4, 4)));
	} else {
		// Show VolumeWindow
		fVolumeSlider = new VolumeWindow(BRect(where.x, where.y,
			where.x + 207, where.y + 19), fDontBeep, fVolumeWhich);
		fVolumeSlider->Show();
	}
}


void
MediaReplicant::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOpenMediaPlayer:
			_Launch("MediaPlayer", "application/x-vnd.Haiku-MediaPlayer",
				B_SYSTEM_APPS_DIRECTORY, "MediaPlayer");
			break;

		case kMsgOpenMediaSettings:
			_Launch("Media Preferences", "application/x-vnd.Haiku-Media",
				B_SYSTEM_PREFERENCES_DIRECTORY, "Media");
			break;

		case kMsgOpenSoundSettings:
			_Launch("Sounds Preferences", "application/x-vnd.Haiku-Sounds",
				B_SYSTEM_PREFERENCES_DIRECTORY, "Sounds");
			break;

		case kMsgToggleBeep:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) != B_OK)
				return;

			item->SetMarked(!item->IsMarked());
			fDontBeep = !item->IsMarked();
			break;
		}

		case kMsgVolumeWhich:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) != B_OK)
				return;

			item->SetMarked(!item->IsMarked());
			fVolumeWhich = item->IsMarked()
				? VOLUME_USE_PHYS_OUTPUT : VOLUME_USE_MIXER;

			if (VolumeToolTip* tip = dynamic_cast<VolumeToolTip*>(ToolTip()))
				tip->SetWhich(fVolumeWhich);
			break;
		}

		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaY;
			if (message->FindFloat("be:wheel_delta_y", &deltaY) == B_OK
				&& deltaY != 0.0) {
				MixerControl mixerControl;
				mixerControl.Connect(fVolumeWhich);
				mixerControl.ChangeVolumeBy(deltaY < 0 ? 6 : -6);

				VolumeToolTip* tip = dynamic_cast<VolumeToolTip*>(ToolTip());
				if (tip != NULL) {
					tip->Update();
					ShowToolTip(tip);
				}
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


status_t
MediaReplicant::_LaunchByPath(const char* path)
{
	entry_ref ref;
	status_t status = get_ref_for_path(path, &ref);
	if (status != B_OK)
		return status;

	status = be_roster->Launch(&ref);
	if (status != B_ALREADY_RUNNING)
		return status;

	// The application runs already, bring it to front

	app_info appInfo;
	status = be_roster->GetAppInfo(&ref, &appInfo);
	if (status != B_OK)
		return status;

	return be_roster->ActivateApp(appInfo.team);
}


status_t
MediaReplicant::_LaunchBySignature(const char* signature)
{
	status_t status = be_roster->Launch(signature);
	if (status != B_ALREADY_RUNNING)
		return status;

	// The application runs already, bring it to front

	app_info appInfo;
	status = be_roster->GetAppInfo(signature, &appInfo);
	if (status != B_OK)
		return status;

	return be_roster->ActivateApp(appInfo.team);
}


void
MediaReplicant::_Launch(const char* prettyName, const char* signature,
	directory_which base, const char* fileName)
{
	BPath path;
	status_t status = find_directory(base, &path);
	if (status == B_OK)
		path.Append(fileName);

	// launch the application
	if (_LaunchBySignature(signature) != B_OK
		&& _LaunchByPath(path.Path()) != B_OK) {
		BString message = B_TRANSLATE("Couldn't launch ");
		message << prettyName;

		(new BAlert(B_TRANSLATE("desklink"), message.String(),
			B_TRANSLATE("OK")))->Go();
	}
}


void
MediaReplicant::_LoadSettings()
{
	fDontBeep = false;
	fVolumeWhich = VOLUME_USE_MIXER;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, false) < B_OK)
		return;

	path.Append(kSettingsFile);

	BFile settings(path.Path(), B_READ_ONLY);
	if (settings.InitCheck() < B_OK)
		return;

	BMessage msg;
	if (msg.Unflatten(&settings) < B_OK)
		return;

	msg.FindInt32("volwhich", &fVolumeWhich);
	msg.FindBool("dontbeep", &fDontBeep);
}


void
MediaReplicant::_SaveSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, false) < B_OK)
		return;

	path.Append(kSettingsFile);

	BFile settings(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (settings.InitCheck() < B_OK)
		return;

	BMessage msg('CNFG');
	msg.AddInt32("volwhich", fVolumeWhich);
	msg.AddBool("dontbeep", fDontBeep);

	ssize_t size = 0;
	msg.Flatten(&settings, &size);
}


void
MediaReplicant::_Init()
{
	fIcon = new BBitmap(BRect(0, 0, kSpeakerWidth - 1, kSpeakerHeight - 1),
		B_RGBA32);
	BIconUtils::GetVectorIcon(kSpeakerIcon, sizeof(kSpeakerIcon), fIcon);

	_LoadSettings();

	SetToolTip(new VolumeToolTip(fVolumeWhich));
}


//	#pragma mark -


extern "C" BView*
instantiate_deskbar_item(void)
{
	return new MediaReplicant(BRect(0, 0, 16, 16), kReplicantName);
}

