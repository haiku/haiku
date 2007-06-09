/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "LaunchButton.h"

#include <malloc.h> // string.h is not enough on Haiku?!?
#include <stdio.h>
#include <string.h>

#include <AppDefs.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <File.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Region.h>
#include <Roster.h>
#include <Window.h>

//#include "BubbleHelper.h"
#include "PadView.h"

static const float kDragStartDist = 10.0;
static const float kDragBitmapAlphaScale = 0.6;
//static const char* kEmptyHelpString = "You can drag an icon here.";

bigtime_t
LaunchButton::fClickSpeed = 0;

// constructor
LaunchButton::LaunchButton(const char* name, uint32 id, const char* label,
						   BMessage* message, BHandler* target)
	: IconButton(name, id, label, message, target),
	  fRef(NULL),
	  fAppSig(NULL),
	  fDescription(""),
	  fAnticipatingDrop(false),
	  fLastClickTime(0)
{
	if (fClickSpeed == 0 || get_click_speed(&fClickSpeed) < B_OK)
		fClickSpeed = 500000;

	BSize size(32.0 + 8.0, 32.0 + 8.0);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);
}

// destructor
LaunchButton::~LaunchButton()
{
	delete fRef;
	if (fAppSig)
		free(fAppSig);
}

// AttachedToWindow
void
LaunchButton::AttachedToWindow()
{
	IconButton::AttachedToWindow();
	_UpdateToolTip();
}

// DetachedFromWindow
void
LaunchButton::DetachedFromWindow()
{
//	BubbleHelper::Default()->SetHelp(this, NULL);
}

// Draw
void
LaunchButton::Draw(BRect updateRect)
{
	if (fAnticipatingDrop) {
		rgb_color color = fRef ? ui_color(B_KEYBOARD_NAVIGATION_COLOR)
							   : (rgb_color){ 0, 130, 60, 255 };
		SetHighColor(color);
		// limit clipping region to exclude the blue rect we just drew
		BRect r(Bounds());
		StrokeRect(r);
		r.InsetBy(1.0, 1.0);
		BRegion region(r);
		ConstrainClippingRegion(&region);
	}
	if (IsValid()) {
		IconButton::Draw(updateRect);
	} else {
		rgb_color background = LowColor();
		rgb_color lightShadow = tint_color(background, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
		rgb_color shadow = tint_color(background, B_DARKEN_1_TINT);
		rgb_color light = tint_color(background, B_LIGHTEN_1_TINT);
		BRect r(Bounds());
		_DrawFrame(r, shadow, light, lightShadow, lightShadow);
		r.InsetBy(2.0, 2.0);
		SetHighColor(lightShadow);
		FillRect(r);
	}
}

// MessageReceived
void
LaunchButton::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED: {
			entry_ref ref; 
			if (message->FindRef("refs", &ref) >= B_OK) {
				if (fRef) {
					if (ref != *fRef) {
						BEntry entry(fRef, true);
						if (entry.IsDirectory()) {
							message->PrintToStream();
							// copy stuff into the directory
						} else {
							message->what = B_REFS_RECEIVED;
							team_id team;
							if (fAppSig)
								team = be_roster->TeamFor(fAppSig);
							else
								team = be_roster->TeamFor(fRef);
							if (team < B_OK) {
								if (fAppSig)
									be_roster->Launch(fAppSig, message, &team);
								else
									be_roster->Launch(fRef, message, &team);
							} else {
								app_info appInfo;
								if (team >= B_OK && be_roster->GetRunningAppInfo(team, &appInfo) >= B_OK) {
									BMessenger messenger(appInfo.signature, team);
									if (messenger.IsValid())
										messenger.SendMessage(message);
								}
							}
						}
					}
				} else {
					SetTo(&ref);
				}
			}
			break;
		}
		case B_PASTE:
		case B_MODIFIERS_CHANGED:
		default:
			IconButton::MessageReceived(message);
			break;
	}
}

// MouseDown
void
LaunchButton::MouseDown(BPoint where)
{
	bigtime_t now = system_time();
	bool callInherited = true;
	if (now - fLastClickTime < fClickSpeed)
		callInherited = false;
	fLastClickTime = now;
	if (BMessage* message = Window()->CurrentMessage()) {
		uint32 buttons;
		message->FindInt32("buttons", (int32*)&buttons);
		if (buttons & B_SECONDARY_MOUSE_BUTTON) {
			if (PadView* parent = dynamic_cast<PadView*>(Parent())) {
				parent->DisplayMenu(ConvertToParent(where), this);
				_ClearFlags(STATE_INSIDE);
				callInherited = false;
			}
		} else {
			fDragStart = where;
		}
	}
	if (callInherited)
		IconButton::MouseDown(where);
}

// MouseUp
void
LaunchButton::MouseUp(BPoint where)
{
	if (fAnticipatingDrop) {
		fAnticipatingDrop = false;
		Invalidate();
	}
	IconButton::MouseUp(where);
}

// MouseMoved
void
LaunchButton::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if ((dragMessage && (transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW))
		&& ((dragMessage->what == B_SIMPLE_DATA
			|| dragMessage->what == B_REFS_RECEIVED) || fRef)) {
		if (!fAnticipatingDrop) {
			fAnticipatingDrop = true;
			Invalidate();
		}
	}
	if (!dragMessage || (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW)) {
		if (fAnticipatingDrop) {
			fAnticipatingDrop = false;
			Invalidate();
		}
	}
	// see if we should create a drag message
	if (_HasFlags(STATE_TRACKING) && fRef) {
		BPoint diff = where - fDragStart;
		float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
		if (dist >= kDragStartDist) {
			// stop tracking
			_ClearFlags(STATE_PRESSED | STATE_TRACKING | STATE_INSIDE);
			// create drag bitmap and message
			if (BBitmap* bitmap = Bitmap()) {
				if (bitmap->ColorSpace() == B_RGB32) {
					// make semitransparent
					uint8* bits = (uint8*)bitmap->Bits();
					uint32 width = bitmap->Bounds().IntegerWidth() + 1;
					uint32 height = bitmap->Bounds().IntegerHeight() + 1;
					uint32 bpr = bitmap->BytesPerRow();
					for (uint32 y = 0; y < height; y++) {
						uint8* bitsHandle = bits;
						for (uint32 x = 0; x < width; x++) {
							bitsHandle[3] = uint8(bitsHandle[3] * kDragBitmapAlphaScale);
							bitsHandle += 4;
						}
						bits += bpr;
					}
				}
				BMessage message(B_SIMPLE_DATA);
				message.AddPointer("button", this);
				message.AddRef("refs", fRef);
				DragMessage(&message, bitmap, B_OP_ALPHA, fDragStart);
			}
		}
	}
	IconButton::MouseMoved(where, transit, dragMessage);
}

// SetTo
void
LaunchButton::SetTo(const entry_ref* ref)
{
	if (fAppSig) {
		free(fAppSig);
		fAppSig = NULL;
	}
	delete fRef;
	if (ref) {
		fRef = new entry_ref(*ref);
		// follow links
		BEntry entry(fRef, true);
		entry.GetRef(fRef);

		_UpdateIcon(fRef);
		// see if this is an application
		BFile file(ref, B_READ_ONLY);
		BAppFileInfo info;
		if (info.SetTo(&file) >= B_OK) {
			char mimeSig[B_MIME_TYPE_LENGTH];
			if (info.GetSignature(mimeSig) >= B_OK) {
				SetTo(mimeSig, false);
			} else {
				printf("no MIME sig\n");
			}
		} else {
			printf("no app\n");
		}
	} else {
		fRef = NULL;
		ClearIcon();
	}
	_UpdateToolTip();
}

// Ref
entry_ref*
LaunchButton::Ref() const
{
	return fRef;
}

// SetTo
void
LaunchButton::SetTo(const char* appSig, bool updateIcon)
{
	if (appSig) {
		if (fAppSig)
			free(fAppSig);
		fAppSig = strdup(appSig);
		if (updateIcon) {
			entry_ref ref;
			if (be_roster->FindApp(fAppSig, &ref) >= B_OK)
				SetTo(&ref);
		}
	}
	_UpdateToolTip();
}

// SetDesciption
void
LaunchButton::SetDescription(const char* text)
{
	fDescription.SetTo(text);
	_UpdateToolTip();
}

// _UpdateToolTip
void
LaunchButton::_UpdateToolTip()
{
	if (fRef) {
		BString helper(fRef->name);
		if (fDescription.CountChars() > 0) {
			helper << "\n\n" << fDescription.String();
		} else {
			BFile file(fRef, B_READ_ONLY);
			BAppFileInfo appFileInfo;
			version_info info;
			if (appFileInfo.SetTo(&file) >= B_OK
				&& appFileInfo.GetVersionInfo(&info,
											  B_APP_VERSION_KIND) >= B_OK
				&& strlen(info.short_info) > 0) {
				helper << "\n\n" << info.short_info;
			}
		}
//		BubbleHelper::Default()->SetHelp(this, helper.String());
	} else {
//		BubbleHelper::Default()->SetHelp(this, kEmptyHelpString);
	}
}

// _UpdateIcon
void
LaunchButton::_UpdateIcon(const entry_ref* ref)
{
	BBitmap* icon = new BBitmap(BRect(0.0, 0.0, 31.0, 31.0), B_RGBA32);
	if (BNodeInfo::GetTrackerIcon(ref, icon, B_LARGE_ICON) >= B_OK)
		SetIcon(icon);

	delete icon;
}
