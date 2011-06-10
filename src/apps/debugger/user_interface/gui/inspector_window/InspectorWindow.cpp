/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "InspectorWindow.h"

#include <Application.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <stdio.h>

#include "MemoryView.h"
#include "MessageCodes.h"
#include "UserInterface.h"


InspectorWindow::InspectorWindow(UserInterfaceListener* listener)
	:
	BWindow(BRect(100, 100, 500, 250), "Inspector", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fListener(listener)
{
}


InspectorWindow::~InspectorWindow()
{
}


/* static */ InspectorWindow*
InspectorWindow::Create(UserInterfaceListener* listener)
{
	InspectorWindow* self = new InspectorWindow(listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
InspectorWindow::_Init()
{
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fAddressInput = new BTextControl("addrInput",
			"Target Address:", "",
			new BMessage(MSG_INSPECT_ADDRESS)))
		.Add(fMemoryView = new MemoryView(NULL, NULL))
	.End();

	fAddressInput->SetTarget(this);
}



void
InspectorWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_INSPECT_ADDRESS:
		{
			const char* addressExpression = fAddressInput->Text();
			fListener->InspectRequested(addressExpression, this);
			break;
		}
	}
}


bool
InspectorWindow::QuitRequested()
{
	be_app_messenger.SendMessage(MSG_INSPECTOR_WINDOW_CLOSED);
	return true;
}


void
InspectorWindow::MemoryBlockRetrieved(TeamMemoryBlock* block)
{
	// TODO: implement
}
