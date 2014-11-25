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
#include <Directory.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <Path.h>
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
	BBox(NULL, B_NAVIGABLE_JUMP, B_NO_BORDER),
	fListView(NULL)
{
	SetName("Interfaces");
}


InterfacesAddOn::~InterfacesAddOn()
{
}


BView*
InterfacesAddOn::CreateView()
{
	// Construct the ListView
	fListView = new InterfacesListView("interfaces");
	fListView->SetSelectionMessage(new BMessage(kMsgInterfaceSelected));

	BScrollView* scrollView = new BScrollView("scrollView", fListView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	// Build the layout
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(scrollView)
	);

	return this;
}


void
InterfacesAddOn::AttachedToWindow()
{
	fListView->SetTarget(this);
}


// FIXME with this scheme the apply and revert button will only take effect for
// the currently shown interface. This can be confusing, it would be better to
// keep the state of all interfaces and allow reverting and saving all of them.
status_t
InterfacesAddOn::Save()
{
	// TODO : Profile?

	InterfaceView* settingsView = _SettingsView();
	if (settingsView != NULL)
		settingsView->Apply();

	BString settingsData;

	status_t result = fListView->SaveItems(settingsData);
	if (result != B_OK)
		return result;

	if (settingsData.IsEmpty()) {
		// Nothing to save, all interfaces are auto-configured
		return B_OK;
	}

	BPath path;
	result = find_directory(B_SYSTEM_SETTINGS_DIRECTORY, &path, true);
	if (result != B_OK)
		return result;

	path.Append("network");
	create_directory(path.Path(), 0755);

	path.Append("interfaces");

	BFile settingsFile(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	result = settingsFile.Write(settingsData.String(),settingsData.Length());
	if (result != settingsData.Length())
		return result;

	return B_OK;
}


status_t
InterfacesAddOn::Revert()
{
	InterfaceView* settingsView = _SettingsView();
	if (settingsView != NULL)
		settingsView->Revert();
	return B_OK;
}


void
InterfacesAddOn::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgInterfaceSelected:
			_ShowPanel();
			break;

		case B_OBSERVER_NOTICE_CHANGE:
			fListView->Invalidate();
			break;

		default:
			BBox::MessageReceived(msg);
	}
}


void
InterfacesAddOn::Show()
{
	BView::Show();
	_ShowPanel();
}


InterfaceView*
InterfacesAddOn::_SettingsView()
{
	BView* view = Window()->FindView("panel")->ChildAt(0);
	return dynamic_cast<InterfaceView*>(view);
}


void
InterfacesAddOn::_ShowPanel()
{
	int nr = fListView->CurrentSelection();
	InterfaceListItem *item = NULL;
	if (nr != -1)
		item = dynamic_cast<InterfaceListItem*>(fListView->ItemAt(nr));

	if (item == NULL)
		return;

	BView* panel = Window()->FindView("panel");
	BView* settingsView = panel->ChildAt(0);

	// Remove currently displayed settings view
	if (settingsView != NULL) {
		settingsView->RemoveSelf();
		delete settingsView;
	}

	settingsView = new InterfaceView(item->GetSettings());
	Window()->FindView("panel")->AddChild(settingsView);
}
