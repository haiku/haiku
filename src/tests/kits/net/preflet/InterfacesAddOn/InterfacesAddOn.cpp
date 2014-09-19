/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus
 *		Axel Dörfler
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Philippe Houdoin
 * 		Fredrik Modéen
 *		Philippe Saint-Pierre
 *		Hugo Santos
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfacesAddOn.h"
#include "InterfaceView.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <ScrollView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InterfacesAddOn"


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
	BBox(NULL, B_NAVIGABLE_JUMP, B_NO_BORDER)
{
	fSettingsView = NULL;
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
InterfacesAddOn::CreateView()
{
	// Construct the ListView
	fListView = new InterfacesListView("interfaces");
	fListView->SetSelectionMessage(new BMessage(kMsgInterfaceSelected));

	BScrollView* scrollView = new BScrollView("scrollView", fListView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	// Construct the BButtons
	fOnOff = new BButton("onoff", B_TRANSLATE("Disable"),
		new BMessage(kMsgInterfaceToggle));
	fOnOff->SetEnabled(false);

	fRenegotiate = new BButton("heal", B_TRANSLATE("Renegotiate"),
		new BMessage(kMsgInterfaceRenegotiate));
	fRenegotiate->SetEnabled(false);

	// Build the layout
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(scrollView)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fOnOff)
			.AddGlue()
			.Add(fRenegotiate)
		.End()
	);

	return this;
}


void
InterfacesAddOn::AttachedToWindow()
{
	fListView->SetTarget(this);
	fOnOff->SetTarget(this);
	fRenegotiate->SetTarget(this);
}


// FIXME with this scheme the apply and revert button will only take effect for
// the currently shown interface. This can be confusing, it would be better to
// keep the state of all interfaces and allow reverting and saving all of them.
status_t
InterfacesAddOn::Save()
{
	// TODO : Profile?
	
	fSettingsView->Apply();
	return fListView->SaveItems();
}


status_t
InterfacesAddOn::Revert()
{
	fSettingsView->Revert();
	return B_OK;
}


void
InterfacesAddOn::MessageReceived(BMessage* msg)
{
	int nr = fListView->CurrentSelection();
	InterfaceListItem *item = NULL;
	if (nr != -1)
		item = dynamic_cast<InterfaceListItem*>(fListView->ItemAt(nr));

	switch (msg->what) {
		case kMsgInterfaceSelected:
		{
			fOnOff->SetEnabled(item != NULL);
			fRenegotiate->SetEnabled(item != NULL);
			if (item == NULL)
				break;
			fRenegotiate->SetEnabled(!item->IsDisabled());
			fOnOff->SetLabel(item->IsDisabled() ? "Enable" : "Disable");

			// TODO it would be better to reuse the view instead of recreating
			// one.
			if (fSettingsView != NULL) {
				fSettingsView->RemoveSelf();
				delete fSettingsView;
			}
			fSettingsView = new InterfaceView(item->GetSettings());
			AddChild(fSettingsView);
			break;
		}

		case kMsgInterfaceToggle:
		{
			if (item == NULL)
				break;

			item->SetDisabled(!item->IsDisabled());
			fOnOff->SetLabel(item->IsDisabled() ? "Enable" : "Disable");
			fRenegotiate->SetEnabled(!item->IsDisabled());
			fListView->Invalidate();
			break;
		}

		case kMsgInterfaceRenegotiate:
		{
			if (item == NULL)
				break;

			NetworkSettings* ns = item->GetSettings();
			ns->RenegotiateAddresses();
			break;
		}

		default:
			BBox::MessageReceived(msg);
	}
}
