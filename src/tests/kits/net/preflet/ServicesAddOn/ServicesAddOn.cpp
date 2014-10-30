/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "ServicesAddOn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ListView.h>
#include <ScrollView.h>
#include <String.h>
#include <Window.h>

#include "DNSSettingsView.h"


static const int32 kSelectionChanged = 'ssrv';


NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index != 0)
		return NULL;

	return new ServicesAddOn(image);
}


// #pragma mark -


ServicesAddOn::ServicesAddOn(image_id image)
	:
	NetworkSetupAddOn(image),
	BGroupView("Services")
{
}


BView*
ServicesAddOn::CreateView()
{
	fServicesListView = new BListView("system_services",
		B_SINGLE_SELECTION_LIST);
	fServicesListView->SetSelectionMessage(new BMessage(kSelectionChanged));

	BScrollView* sv = new BScrollView( "ScrollView",
		fServicesListView, B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	fServicesListView->AddItem(new BStringItem("DNS"));

#if 0
	// Enable this when the inetd view is ready
	if (_ParseInetd() != B_OK)
		_ParseXinetd();
#endif

	AddChild(sv);

	return this;
}


void
ServicesAddOn::AttachedToWindow()
{
	fServicesListView->SetTarget(this);
}


void
ServicesAddOn::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kSelectionChanged:
		{
			BStringItem* item = static_cast<BStringItem*>(
				fServicesListView->ItemAt(message->FindInt32("index")));

			if (item == NULL)
				return;

			BView* panel = Window()->FindView("panel");
			BView* settingsView = panel->ChildAt(0);

			// Remove currently displayed settings view
			if (settingsView != NULL) {
				settingsView->RemoveSelf();
				delete settingsView;
			}

			if (strcmp(item->Text(), "DNS") == 0) {
				settingsView = new DNSSettingsView();
				panel->AddChild(settingsView);
			} else {
				// TODO show a standard "inetd service" view
			}
			break;
		}
		default:
			BGroupView::MessageReceived(message);
	}
}


status_t
ServicesAddOn::Save()
{
	BView* panel = Window()->FindView("panel");
	DNSSettingsView* settingsView = dynamic_cast<DNSSettingsView*>(
		panel->ChildAt(0));

	// View not active - nothing to save
	if (settingsView == NULL)
		return B_OK;

	return settingsView->Apply();
}


status_t
ServicesAddOn::Revert()
{
	BView* panel = Window()->FindView("panel");
	DNSSettingsView* settingsView = dynamic_cast<DNSSettingsView*>(
		panel->ChildAt(0));

	// View not active - nothing to revert
	if (settingsView == NULL)
		return B_OK;

	return settingsView->Revert();
}


status_t
ServicesAddOn::_ParseInetd()
{
	FILE *f = fopen("/etc/inetd.conf", "r");
	if (f) {
		char line[1024], *linePtr;
		char *token;

		while (fgets(line, sizeof(line), f)) {
			linePtr = line;
			if (! *linePtr)
				continue;

			if (line[0] == '#') {
				if (line[1] == ' ' || line[1] == '\t' ||
					line[1] == '\0' || line[1] == '\n' ||
					line[1] == '\r')
					// skip comments
					continue;
				linePtr++;	// jump the disable/comment service mark
			}

			BString label;
			token = strtok(linePtr, " \t");	// service name
			label << token;
			token = strtok(NULL, " \t");	// type
			label << " (" << token << ")";
			token = strtok(NULL, " \t");	// protocol
			label << " " << token;

			fServicesListView->AddItem(new BStringItem(label.String()));
		}
		fclose(f);
		return B_OK;
	}

	return B_ERROR;
}


status_t
ServicesAddOn::_ParseXinetd()
{
	FILE *f = fopen("/boot/system/settings/network/services", "r");
	if (f) {
		char line[1024], *linePtr;
		char *token;
		char *loc;

		while (fgets(line, sizeof(line), f)) {
			linePtr = line;

			if (! *linePtr)
				continue;

			if (line[0] == '#' || line[0] == '\n') {
				// Skip commented out lines
				continue;
			}

			loc = strstr(linePtr, "service ");

			if (loc) {
				BString label;
				token = strtok(loc, " ");
				token = strtok(NULL, " ");
				label << token;
				fServicesListView->AddItem(new BStringItem(label.String()));
			}

		}
		fclose(f);
		return B_OK;
	}

	return B_ERROR;
}
