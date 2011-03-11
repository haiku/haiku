/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Stephan Assmuß
 *		Axel Dörfler
 *		Philippe Houdoin
 * 		Fredrik Modéen
 *		Hugo Santos
 *		Philippe Saint-Pierre
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "InterfacesAddOn.h"
#include "InterfaceWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ScrollView.h>


NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index == 0)
		return new InterfacesAddOn(image);
	return NULL;
}


// #pragma mark -


InterfacesAddOn::InterfacesAddOn(image_id image)
	:
	NetworkSetupAddOn(image),
	BBox(BRect(0, 0, 0, 0), NULL, B_FOLLOW_ALL, B_NAVIGABLE_JUMP, B_NO_BORDER)
{
}


InterfacesAddOn::~InterfacesAddOn()
{
}


const char*
InterfacesAddOn::Name()
{
	return "Interfaces";
}


BView*
InterfacesAddOn::CreateView(BRect *bounds)
{
	BRect intViewRect = *bounds;

	// Construct the ListView
	fListview = new InterfacesListView(intViewRect,
		"interfaces", B_FOLLOW_ALL_SIDES);
	fListview->SetSelectionMessage(new BMessage(kMsgInterfaceSelected));
	fListview->SetInvocationMessage(new BMessage(kMsgInterfaceConfigure));

	BScrollView* scrollView = new BScrollView(NULL, fListview,
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	// Construct the BButtons
	fConfigure = new BButton(intViewRect, "configure",
		"Configure" B_UTF8_ELLIPSIS, new BMessage(kMsgInterfaceConfigure));
	fConfigure->SetEnabled(false);

	fOnOff = new BButton(intViewRect, "onoff", "Disable",
		new BMessage(kMsgInterfaceToggle));
	fOnOff->SetEnabled(false);

	fRenegotiate = new BButton(intViewRect, "heal",
		"Renegotiate", new BMessage(kMsgInterfaceRenegotiate));
	fRenegotiate->SetEnabled(false);

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(scrollView)
		.AddGroup(B_HORIZONTAL, 5)
			.Add(fConfigure)
			.Add(fOnOff)
			.AddGlue()
			.Add(fRenegotiate)
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	*bounds = Bounds();
	return this;
}


void
InterfacesAddOn::AttachedToWindow()
{
	fListview->SetTarget(this);
	fConfigure->SetTarget(this);
	fOnOff->SetTarget(this);
	fRenegotiate->SetTarget(this);
}


status_t
InterfacesAddOn::Save()
{
	// TODO : Profile?
	return fListview->SaveItems();
}


void
InterfacesAddOn::MessageReceived(BMessage* msg)
{
	int nr = fListview->CurrentSelection();
	InterfaceListItem *item = NULL;
	if (nr != -1) {
		item = dynamic_cast<InterfaceListItem*>(fListview->ItemAt(nr));
	}

	switch (msg->what) {
		case kMsgInterfaceSelected:
		{
			fConfigure->SetEnabled(item != NULL);
			fOnOff->SetEnabled(item != NULL);
			fRenegotiate->SetEnabled(item != NULL);
			if (!item)
				break;
			fConfigure->SetEnabled(!item->IsDisabled());
			fRenegotiate->SetEnabled(!item->IsDisabled());
			fOnOff->SetLabel(item->IsDisabled() ? "Enable" : "Disable");
			break;
		}

		case kMsgInterfaceConfigure:
		{
			if (!item)
				break;

			InterfaceWindow* sw = new InterfaceWindow(item->GetSettings());
			sw->Show();
			break;
		}

		case kMsgInterfaceToggle:
		{
			if (!item)
				break;

			item->SetDisabled(!item->IsDisabled());
			fConfigure->SetEnabled(!item->IsDisabled());
			fOnOff->SetLabel(item->IsDisabled() ? "Enable" : "Disable");
			fRenegotiate->SetEnabled(!item->IsDisabled());
			fListview->Invalidate();
			break;
		}

		case kMsgInterfaceRenegotiate:
		{
			if (!item)
				break;

			NetworkSettings* ns = item->GetSettings();
			ns->RenegotiateAddresses();
			break;
		}

		default:
			BBox::MessageReceived(msg);
	}
}
