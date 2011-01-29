/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
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
 */


#include "InterfacesAddOn.h"
#include "InterfacesListView.h"
#include "NetworkWindow.h"

#include <stdio.h>

#include <ScrollView.h>
#include <Alert.h>



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
	float w, h;
	BRect r = *bounds;

#define H_MARGIN	10
#define V_MARGIN	10
#define SMALL_MARGIN	3

	if (r.Width() < 100 || r.Height() < 100)
		r.Set(0, 0, 100, 100);

	ResizeTo(r.Width(), r.Height());

	BRect rlv = r;
	rlv.bottom -= 72;
	rlv.InsetBy(2, 2);
	rlv.right -= B_V_SCROLL_BAR_WIDTH;
	fListview = new InterfacesListView(rlv, "interfaces", B_FOLLOW_ALL_SIDES);
	fListview->SetSelectionMessage(new BMessage(INTERFACE_SELECTED_MSG));
	fListview->SetInvocationMessage(new BMessage(CONFIGURE_INTERFACE_MSG));
	AddChild(new BScrollView(NULL, fListview, B_FOLLOW_ALL_SIDES, B_WILL_DRAW
			| B_FRAME_EVENTS, false, true));

	r.top = r.bottom - 60;
	fConfigure = new BButton(r, "configure", "Configure" B_UTF8_ELLIPSIS,
		new BMessage(CONFIGURE_INTERFACE_MSG), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT);

	fConfigure->GetPreferredSize(&w, &h);
	fConfigure->ResizeToPreferred();
	fConfigure->SetEnabled(false);
	AddChild(fConfigure);

	r.left += w + SMALL_MARGIN;

	fOnOff = new BButton(r, "onoff", "Disable",
				new BMessage(ONOFF_INTERFACE_MSG),
				B_FOLLOW_BOTTOM | B_FOLLOW_LEFT);
	fOnOff->GetPreferredSize(&w, &h);
	fOnOff->ResizeToPreferred();
	fOnOff->Hide();
	AddChild(fOnOff);

	*bounds = Bounds();
	return this;
}


void
InterfacesAddOn::AttachedToWindow()
{
	fListview->SetTarget(this);
	fConfigure->SetTarget(this);
	fOnOff->SetTarget(this);

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
	case INTERFACE_SELECTED_MSG: {
		fConfigure->SetEnabled(item != NULL);
		fOnOff->SetEnabled(item != NULL);
		if (item == NULL) {
			fOnOff->Hide();
			break;
		}
		fOnOff->SetLabel(item->IsDisabled() ? "Enable" : "Disable");
		fOnOff->Show();
		break;
	}

	case CONFIGURE_INTERFACE_MSG: {
		if (!item)
			break;

		NetworkWindow* nw = new NetworkWindow(item->GetSettings());
		nw->Show();
		break;
	}

	case ONOFF_INTERFACE_MSG:
		if (!item)
			break;

		item->SetDisabled(!item->IsDisabled());
		fOnOff->SetLabel(item->IsDisabled() ? "Enable" : "Disable");
		fListview->Invalidate();
		break;

	default:
		BBox::MessageReceived(msg);
	}
}
