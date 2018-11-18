/*
 * Copyright 2003-2018, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Marcus Overhagen
 *		Jonas Sundström
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Puck Meerburg, puck@puckipedia.nl
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
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>
#include <ToolTip.h>
#include <ToolTipManager.h>

#include "desklink.h"
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


class VolumeToolTip : public BTextToolTip {
public:
	VolumeToolTip(int32 which = VOLUME_USE_MIXER)
		:
		BTextToolTip(""),
		fWhich(which)
	{
	}

	virtual ~VolumeToolTip()
	{
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

		BTextView* view = (BTextView*)View();

		if (fMuteMessage.Length() != 0)
			view->SetText(fMuteMessage.String());
		else {
			MixerControl control;
			control.Connect(fWhich);

			BString text;
			text.SetToFormat(B_TRANSLATE("%g dB"), control.Volume());
			view->SetText(text.String());
		}
		Unlock();
	}

	void SetMuteMessage(const char* message)
	{
		fMuteMessage = message == NULL ? "" : message;
	}

private:
	int32			fWhich;
	BString			fMuteMessage;
};


class MediaReplicant : public BView {
public:
							MediaReplicant(BRect frame, const char* name,
								uint32 resizeMask = B_FOLLOW_ALL,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE
									| B_PULSE_NEEDED);
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
			BBitmap*		_LoadIcon(BResources& resources, const char* name);

			void			_DisconnectMixer();
			status_t		_ConnectMixer();

			MixerControl*	fMixerControl;

			BBitmap*		fIcon;
			BBitmap*		fMutedIcon;
			VolumeWindow*	fVolumeSlider;
			bool 			fDontBeep;
				// don't beep on volume change
			int32 			fVolumeWhich;
				// which volume parameter to act on (Mixer/Phys.Output)
			bool				fMuted;
};


status_t
our_image(image_info& image)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if ((char*)our_image >= (char*)image.text
			&& (char*)our_image <= (char*)image.text + image.text_size)
			return B_OK;
	}

	return B_ERROR;
}


//	#pragma mark -


MediaReplicant::MediaReplicant(BRect frame, const char* name,
		uint32 resizeMask, uint32 flags)
	:
	BView(frame, name, resizeMask, flags),
	fVolumeSlider(NULL),
	fMuted(false)
{
	_Init();
}


MediaReplicant::MediaReplicant(BMessage* message)
	:
	BView(message),
	fVolumeSlider(NULL),
	fMuted(false)
{
	_Init();
}


MediaReplicant::~MediaReplicant()
{
	delete fIcon;
	_SaveSettings();
	_DisconnectMixer();
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
	AdoptParentColors();

	_ConnectMixer();

	BView::AttachedToWindow();
}


void
MediaReplicant::Draw(BRect rect)
{
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fMuted ? fMutedIcon : fIcon);
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

	} else if ((buttons & B_TERTIARY_MOUSE_BUTTON) != 0) {
		if (fMixerControl != NULL) {
			fMixerControl->SetMute(!fMuted);
			fMuted = fMixerControl->Mute();
			VolumeToolTip* tip = dynamic_cast<VolumeToolTip*>(ToolTip());
			if (tip != NULL) {
				tip->SetMuteMessage(fMuted ? B_TRANSLATE("Muted"): NULL);
				tip->Update();
				ShowToolTip(tip);
			}
			Invalidate();
		}

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

			if (_ConnectMixer() != B_OK
				&& fVolumeWhich == VOLUME_USE_PHYS_OUTPUT) {
				// unable to switch to physical output
				item->SetMarked(false);
				fVolumeWhich = VOLUME_USE_MIXER;
				_ConnectMixer();
			}

			if (VolumeToolTip* tip = dynamic_cast<VolumeToolTip*>(ToolTip())) {
				tip->SetWhich(fVolumeWhich);
				tip->Update();
			}
			break;
		}

		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaY;
			if (message->FindFloat("be:wheel_delta_y", &deltaY) == B_OK
				&& deltaY != 0.0 && fMixerControl != NULL) {
				fMixerControl->ChangeVolumeBy(deltaY < 0 ? 6 : -6);

				VolumeToolTip* tip = dynamic_cast<VolumeToolTip*>(ToolTip());
				if (tip != NULL) {
					tip->Update();
					ShowToolTip(tip);
				}
			}
			break;
		}

		case B_MEDIA_NEW_PARAMETER_VALUE:
		{
			if (fMixerControl != NULL && !fMixerControl->Connected())
				return;

			bool setMuted = fMixerControl->Mute();
			if (setMuted != fMuted) {
				fMuted = setMuted;
				VolumeToolTip* tip = dynamic_cast<VolumeToolTip*>(ToolTip());
				if (tip != NULL) {
					tip->SetMuteMessage(fMuted ? B_TRANSLATE("Muted") : NULL);
					tip->Update();
				}
				Invalidate();
			}
			break;
		}

		case B_MEDIA_SERVER_STARTED:
			_ConnectMixer();
			break;

		case B_MEDIA_NODE_CREATED:
		{
			// It's not enough to wait for B_MEDIA_SERVER_STARTED message, as
			// the mixer will still be getting loaded by the media server
			media_node mixerNode;
			media_node_id mixerNodeID;
			BMediaRoster* roster = BMediaRoster::CurrentRoster();
			if (roster != NULL
				&& message->FindInt32("media_node_id",&mixerNodeID) == B_OK
				&& roster->GetNodeFor(mixerNodeID, &mixerNode) == B_OK) {
				if (mixerNode.kind == B_SYSTEM_MIXER) {
					_ConnectMixer();
					roster->ReleaseNode(mixerNode);
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

		BAlert* alert = new BAlert(
			B_TRANSLATE_COMMENT("desklink", "Title of an alert box"), 
			message.String(), B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
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
	image_info info;
	if (our_image(info) != B_OK)
		return;

	BFile file(info.name, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BResources resources(&file);
	if (resources.InitCheck() != B_OK)
		return;

	fIcon = _LoadIcon(resources, "Speaker");
	fMutedIcon = _LoadIcon(resources, "SpeakerMuted");

	_LoadSettings();

	SetToolTip(new VolumeToolTip(fVolumeWhich));
}


BBitmap*
MediaReplicant::_LoadIcon(BResources& resources, const char* name)
{
	size_t size;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, name, &size);
	if (data == NULL)
		return NULL;

	// Scale tray icon
	BBitmap* icon = new BBitmap(Bounds(), B_RGBA32);
	if (icon->InitCheck() != B_OK
		|| BIconUtils::GetVectorIcon((const uint8*)data, size, icon) != B_OK) {
		delete icon;
		return NULL;
	}
	return icon;
}


void
MediaReplicant::_DisconnectMixer()
{
	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if (roster == NULL)
		return;

	roster->StopWatching(this, B_MEDIA_SERVER_STARTED | B_MEDIA_NODE_CREATED);

	if (fMixerControl->MuteNode() != media_node::null) {
		roster->StopWatching(this, fMixerControl->MuteNode(),
			B_MEDIA_NEW_PARAMETER_VALUE);
	}

	delete fMixerControl;
	fMixerControl = NULL;
}


status_t
MediaReplicant::_ConnectMixer()
{
	_DisconnectMixer();

	BMediaRoster* roster = BMediaRoster::Roster();
	if (roster == NULL)
		return B_ERROR;

	roster->StartWatching(this, B_MEDIA_SERVER_STARTED | B_MEDIA_NODE_CREATED);

	fMixerControl = new MixerControl(fVolumeWhich);

	const char* errorString = NULL;
	float volume = 0.0;
	fMixerControl->Connect(fVolumeWhich, &volume, &errorString);

	if (errorString != NULL) {
		SetToolTip(errorString);
		return B_ERROR;
	}

	if (fMixerControl->MuteNode() != media_node::null) {
		roster->StartWatching(this, fMixerControl->MuteNode(),
			B_MEDIA_NEW_PARAMETER_VALUE);
		fMuted = fMixerControl->Mute();
	}

	return B_OK;
}


//	#pragma mark -


extern "C" BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	return new MediaReplicant(BRect(0, 0, maxHeight - 1, maxHeight - 1),
		kReplicantName);
}

