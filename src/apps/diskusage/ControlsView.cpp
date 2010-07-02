/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */


#include "ControlsView.h"

#include <Bitmap.h>
#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <SupportDefs.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "Common.h"


class VolumeMenuItem: public BMenuItem {
public:
						VolumeMenuItem(BVolume* volume, BMessage* message);
	virtual				~VolumeMenuItem();

	virtual	void		GetContentSize(float* width, float* height);
	virtual	void		DrawContent();

			BVolume*	Volume() const
							{ return fVolume; }
			status_t	Invoke(BMessage* mesage = NULL)
							{ return BMenuItem::Invoke(); }

private:
			BBitmap*	fIcon;
			BVolume*	fVolume;
};


VolumeMenuItem::VolumeMenuItem(BVolume* volume, BMessage* message)
	:
	BMenuItem(kEmptyStr, message),
	fIcon(new BBitmap(BRect(0, 0, 15, 15), B_RGBA32)),
	fVolume(volume)
{
	char name[B_PATH_NAME_LENGTH];
	fVolume->GetName(name);
	SetLabel(name);

	if (fVolume->GetIcon(fIcon, B_MINI_ICON) < B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
}


VolumeMenuItem::~VolumeMenuItem()
{
	delete fIcon;
	delete fVolume;
}


void
VolumeMenuItem::GetContentSize(float* width, float* height)
{
	*width = be_plain_font->StringWidth(Label());

	struct font_height fh;
	be_plain_font->GetHeight(&fh);
	float fontHeight = fh.ascent + fh.descent + fh.leading;
	if (fIcon) {
		*height = max_c(fontHeight, fIcon->Bounds().Height());
		*width += fIcon->Bounds().Width() + kSmallHMargin;
	} else
		*height = fontHeight;
}


void
VolumeMenuItem::DrawContent()
{
	if (fIcon) {
		Menu()->SetDrawingMode(B_OP_OVER);
		Menu()->MovePenBy(0.0, -1.0);
		Menu()->DrawBitmap(fIcon);
		Menu()->SetDrawingMode(B_OP_COPY);
		Menu()->MovePenBy(fIcon->Bounds().Width() + kSmallHMargin, 0.0);
	}
	BMenuItem::DrawContent();
}


// #pragma mark -


class ControlsView::VolumePopup: public BMenuField {
public:
								VolumePopup(BRect r);
	virtual						~VolumePopup();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			BVolume*			FindDeviceFor(dev_t device,
									bool invoke = false);

private:
			void				_AddVolume(dev_t device);
			void				_RemoveVolume(dev_t device);

			BVolumeRoster*		fVolumeRoster;
};


ControlsView::VolumePopup::VolumePopup(BRect r)
	:
	BMenuField(r, NULL, kVolMenuLabel, new BPopUpMenu(kVolMenuDefault), false,
		B_FOLLOW_LEFT)
{
	SetViewColor(kWindowColor);
	SetLowColor(kWindowColor);

	SetDivider(kSmallHMargin + StringWidth(kVolMenuLabel));

	// Populate the menu with the persistent volumes.
	fVolumeRoster = new BVolumeRoster();

	BVolume tempVolume;
	while (fVolumeRoster->GetNextVolume(&tempVolume) == B_OK) {
		if (tempVolume.IsPersistent()) {
			BVolume* volume = new BVolume(tempVolume);
			BMessage* message = new BMessage(kMenuSelectVol);
			message->AddPointer(kNameVolPtr, volume);
			VolumeMenuItem *item = new VolumeMenuItem(volume, message);
			Menu()->AddItem(item);
		}
	}
}


ControlsView::VolumePopup::~VolumePopup()
{
	fVolumeRoster->StopWatching();
	delete fVolumeRoster;
}


void
ControlsView::VolumePopup::AttachedToWindow()
{
	// Begin watching mount and unmount events.
	fVolumeRoster->StartWatching(BMessenger(this));
}


void
ControlsView::VolumePopup::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			switch (message->FindInt32("opcode")) {
				case B_DEVICE_MOUNTED:
					_AddVolume(message->FindInt32("new device"));
					break;

				case B_DEVICE_UNMOUNTED:
					_RemoveVolume(message->FindInt32("device"));
					break;
			}
			break;

		default:
			BMenuField::MessageReceived(message);
			break;
	}
}


BVolume*
ControlsView::VolumePopup::FindDeviceFor(dev_t device, bool invoke)
{
	BVolume* volume = NULL;

	// Iterate through items looking for a BVolume representing this device.
	for (int i = 0; VolumeMenuItem* item = (VolumeMenuItem*)Menu()->ItemAt(i); i++) {
		if (item->Volume()->Device() == device) {
			volume = item->Volume();
			if (invoke)
				item->Invoke();
			break;
		}
	}

	return volume;
}


void
ControlsView::VolumePopup::_AddVolume(dev_t device)
{
	// Make sure the volume is not already in the menu.
	for (int i = 0; i < Menu()->CountItems(); i++) {
		VolumeMenuItem* item = (VolumeMenuItem*)Menu()->ItemAt(i);
		if (item->Volume()->Device() == device)
			return;
	}

	// Add the newly mounted volume to the menu.
	BVolume* volume = new BVolume(device);
	BMessage* message = new BMessage(kMenuSelectVol);
	message->AddPointer(kNameVolPtr, volume);
	VolumeMenuItem* item = new VolumeMenuItem(volume, message);
	Menu()->AddItem(item);
}


void
ControlsView::VolumePopup::_RemoveVolume(dev_t device)
{
	for (int i = 0; i < Menu()->CountItems(); i++) {
		VolumeMenuItem* item = (VolumeMenuItem*)Menu()->ItemAt(i);
		if (item->Volume()->Device() == device) {
			// If the volume being removed is currently selected, prompt for a
			// different volume.
			if (item->IsMarked()) {
				// update the displayed volume label now that there is no marked
				// item:
				Menu()->Superitem()->SetLabel(kVolMenuDefault);

				BMessage messae(kMenuSelectVol);
				messae.AddPointer(kNameVolPtr, NULL);
				Window()->PostMessage(&messae);
			}

			Menu()->RemoveItem(item);
			return;
		}
	}
}


// #pragma mark -


ControlsView::ControlsView(BRect r)
	: BView(r, NULL, B_FOLLOW_LEFT_RIGHT, 0)
{
	SetViewColor(kWindowColor);

	r.top += kSmallVMargin;
	r.right -= kSmallHMargin;
	float buttonWidth = kButtonMargin + StringWidth(kHelpBtnLabel);
	r.left = r.right - buttonWidth;
	BButton* helpButton = new BButton(r, NULL, kHelpBtnLabel, new BMessage(kBtnHelp),
		B_FOLLOW_RIGHT);
	if (!kFoundHelpFile)
		helpButton->SetEnabled(false);

	r.right = r.left - kSmallHMargin;
	buttonWidth = kButtonMargin + StringWidth(kStrRescan);
	r.left = r.right - max_c(kMinButtonWidth, buttonWidth);
	fRescanButton = new BButton(r, NULL, kStrRescan, new BMessage(kBtnRescan),
		B_FOLLOW_RIGHT);

	fVolumePopup = new VolumePopup(
		BRect(kSmallHMargin, kSmallVMargin, r.left - kSmallHMargin, kSmallVMargin));

	float width, height;
	fRescanButton->GetPreferredSize(&width, &height);
	ResizeTo(Bounds().Width(), height + 6.0);

	// Horizontal divider
	r = Bounds();
	r.top = r.bottom - 1.0;
	r.left -= 5.0; r.right += 5.0;
	BBox* divider = new BBox(r, NULL, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);

	AddChild(fVolumePopup);
	AddChild(divider);
	AddChild(fRescanButton);
	AddChild(helpButton);
}


ControlsView::~ControlsView()
{
}


BVolume*
ControlsView::FindDeviceFor(dev_t device, bool invoke)
{
	return fVolumePopup->FindDeviceFor(device, invoke);
}
