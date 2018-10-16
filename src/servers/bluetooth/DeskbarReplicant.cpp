/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Weirauch, dev@m-phasis.de
 */


#include "DeskbarReplicant.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <IconUtils.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>

#include <bluetoothserver_p.h>

extern "C" _EXPORT BView *instantiate_deskbar_item(void);
status_t our_image(image_info& image);

const uint32 kMsgOpenBluetoothPreferences = 'obtp';
const uint32 kMsgQuitBluetoothServer = 'qbts';

const char* kDeskbarItemName = "BluetoothServerReplicant";
const char* kClassName = "DeskbarReplicant";


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BluetoothReplicant"


//	#pragma mark -


DeskbarReplicant::DeskbarReplicant(BRect frame, int32 resizingMode)
	: BView(frame, kDeskbarItemName, resizingMode,
		B_WILL_DRAW | B_FRAME_EVENTS)
{
	_Init();
}


DeskbarReplicant::DeskbarReplicant(BMessage* archive)
	: BView(archive)
{
	_Init();
}


DeskbarReplicant::~DeskbarReplicant()
{
}


void
DeskbarReplicant::_Init()
{
	fIcon = NULL;

	image_info info;
	if (our_image(info) != B_OK)
		return;

	BFile file(info.name, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	BResources resources(&file);
	if (resources.InitCheck() < B_OK)
		return;

	size_t size;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE,
		"tray_icon", &size);
	if (data != NULL) {
		BBitmap* icon = new BBitmap(Bounds(), B_RGBA32);
		if (icon->InitCheck() == B_OK
			&& BIconUtils::GetVectorIcon((const uint8 *)data,
				size, icon) == B_OK) {
			fIcon = icon;
		} else
			delete icon;
	}
}


DeskbarReplicant *
DeskbarReplicant::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, kClassName))
		return NULL;

	return new DeskbarReplicant(archive);
}


status_t
DeskbarReplicant::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", BLUETOOTH_SIGNATURE);
	if (status == B_OK)
		status = archive->AddString("class", kClassName);

	return status;
}


void
DeskbarReplicant::AttachedToWindow()
{
	BView::AttachedToWindow();
	AdoptParentColors();

	SetLowColor(ViewColor());
}


void
DeskbarReplicant::Draw(BRect updateRect)
{
	if (!fIcon) {
		/* At least display something... */
		rgb_color lowColor = LowColor();
		SetLowColor(0, 113, 187, 255);
		FillRoundRect(Bounds().InsetBySelf(3.f, 0.f), 5.f, 7.f, B_SOLID_LOW);
		SetLowColor(lowColor);
	} else {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIcon);
		SetDrawingMode(B_OP_COPY);
	}
}


void
DeskbarReplicant::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgOpenBluetoothPreferences:
			be_roster->Launch(BLUETOOTH_APP_SIGNATURE);
			break;

		case kMsgQuitBluetoothServer:
			_QuitBluetoothServer();
			break;

		default:
			BView::MessageReceived(msg);
	}
}


void
DeskbarReplicant::MouseDown(BPoint where)
{
	BPoint point;
	uint32 buttons;
	GetMouse(&point, &buttons);
	if (!(buttons & B_SECONDARY_MOUSE_BUTTON)) {
		return;
	}

	BPopUpMenu* menu = new BPopUpMenu(B_EMPTY_STRING, false, false);

	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(kMsgOpenBluetoothPreferences)));

	// TODO show list of known/paired devices

	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(kMsgQuitBluetoothServer)));

	menu->SetTargetForItems(this);
	ConvertToScreen(&point);
	menu->Go(point, true, true, true);
}


void
DeskbarReplicant::_QuitBluetoothServer()
{
	if (!be_roster->IsRunning(BLUETOOTH_SIGNATURE)) {
		// The server isn't running, so remove ourself
		BDeskbar deskbar;
		deskbar.RemoveItem(kDeskbarItemName);

		return;
	}
	status_t status = BMessenger(BLUETOOTH_SIGNATURE).SendMessage(
		B_QUIT_REQUESTED);
	if (status < B_OK) {
		_ShowErrorAlert(B_TRANSLATE("Stopping the Bluetooth server failed."),
			status);
	}
}


void
DeskbarReplicant::_ShowErrorAlert(BString msg, status_t status)
{
	BString error = B_TRANSLATE("Error: %status%");
	error.ReplaceFirst("%status%", strerror(status));
	msg << "\n\n" << error;
	BAlert* alert = new BAlert(B_TRANSLATE("Bluetooth error"), msg.String(),
		B_TRANSLATE("OK"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go(NULL);
}

//	#pragma mark -


extern "C" _EXPORT BView *
instantiate_deskbar_item(void)
{
	return new DeskbarReplicant(BRect(0, 0, 15, 15),
		B_FOLLOW_LEFT | B_FOLLOW_TOP);
}

//	#pragma mark -


status_t
our_image(image_info& image)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if ((char *)our_image >= (char *)image.text
			&& (char *)our_image <= (char *)image.text + image.text_size)
			return B_OK;
	}

	return B_ERROR;
}
