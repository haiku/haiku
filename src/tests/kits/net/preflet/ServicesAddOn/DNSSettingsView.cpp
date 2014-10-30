/*
 * Copyright 2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "DNSSettingsView.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>


static const int32 kAddServer = 'adds';
static const int32 kDeleteServer = 'dels';
static const int32 kEditServer = 'edit';


DNSSettingsView::DNSSettingsView()
{
	fServerListView = new BListView("nameservers");
	fTextControl = new BTextControl("", "", NULL);
	fTextControl->SetModificationMessage(new BMessage(kEditServer));
	
	fAddButton = new BButton("Add", new BMessage(kAddServer));
	fUpButton = new BButton("Move up", NULL);
	fDownButton = new BButton("Move down", NULL);
	fRemoveButton = new BButton("Remove", new BMessage(kDeleteServer));

	BLayoutBuilder::Group<>(this)
		.AddGrid(B_HORIZONTAL)
			.Add(fTextControl, 0, 0)
			.Add(fAddButton, 1, 0)
			.Add(new BScrollView("scroll", fServerListView, 0, false, true),
				0, 1, 1, 4)
			.Add(fUpButton, 1, 1)
			.Add(fDownButton, 1, 2)
			.Add(fRemoveButton, 1, 3)
		.End()
	.End();

	_ParseResolvConf();
}


void
DNSSettingsView::AttachedToWindow()
{
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	fUpButton->SetTarget(this);
	fDownButton->SetTarget(this);

	fTextControl->SetTarget(this);
}


void
DNSSettingsView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case kAddServer:
		{
			const char* address = fTextControl->Text();
			fServerListView->AddItem(new BStringItem(address));
			break;
		}
		case kDeleteServer:
		{
			fServerListView->RemoveItem(fServerListView->CurrentSelection());
		}
		case kEditServer:
		{
			struct in_addr dummy;
			bool success = inet_aton(fTextControl->Text(), &dummy);
			fTextControl->MarkAsInvalid(!success);
			fAddButton->SetEnabled(success);
		}
		default:
			BGroupView::MessageReceived(message);
	}
}


status_t
DNSSettingsView::_ParseResolvConf()
{
	res_init();
	res_state state = __res_state();

	if (state != NULL) {
		for (int i = 0; i < state->nscount; i++) {
			fServerListView->AddItem(
				new BStringItem(inet_ntoa(state->nsaddr_list[i].sin_addr)));
		}
		return B_OK;
	}

	return B_ERROR;
}
