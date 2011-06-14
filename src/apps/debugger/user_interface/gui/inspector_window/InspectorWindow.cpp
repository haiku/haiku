/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "InspectorWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <ExpressionParser.h>

#include "MemoryView.h"
#include "MessageCodes.h"
#include "UserInterface.h"


InspectorWindow::InspectorWindow(UserInterfaceListener* listener)
	:
	BWindow(BRect(100, 100, 500, 250), "Inspector", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fListener(listener),
	fMemoryView(NULL),
	fCurrentBlock(NULL)
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
	BScrollView* scrollView;

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fAddressInput = new BTextControl("addrInput",
			"Target Address:", "",
			new BMessage(MSG_INSPECT_ADDRESS)))
		.Add(scrollView = new BScrollView("memory scroll",
			NULL, 0, false, true), 3.0f)
	.End();

	scrollView->SetTarget(fMemoryView = MemoryView::Create());

	fAddressInput->SetTarget(this);
}



void
InspectorWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_INSPECT_ADDRESS:
		{
			ExpressionParser parser;
			parser.SetSupportHexInput(true);
			target_addr_t address = 0;
			const char* addressExpression = fAddressInput->Text();
			BString errorMessage;
			try {
				address = parser.EvaluateToInt64(addressExpression);
			} catch(ParseException parseError) {
				errorMessage.SetToFormat("Failed to parse address: %s",
					parseError.message.String());
			} catch(...) {
				errorMessage.SetToFormat(
					"Unknown error while parsing address");
			}

			if (errorMessage.Length() > 0) {
				BAlert* alert = new(std::nothrow) BAlert("Inspect Address",
					errorMessage.String(), "Close");
				if (alert != NULL)
					alert->Go();
			} else {
				if (fCurrentBlock != NULL
					&& !fCurrentBlock->Contains(address)) {
					fCurrentBlock->ReleaseReference();
					fCurrentBlock = NULL;
				}

				if (fCurrentBlock == NULL)
					fListener->InspectRequested(address, this);
				else
					fMemoryView->SetTargetAddress(fCurrentBlock, address);

				fCurrentAddress = address;
				BString computedAddress;
				computedAddress.SetToFormat("0x%" B_PRIx64, address);
				fAddressInput->SetText(computedAddress.String());
			}
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
	fCurrentBlock = block;
	fMemoryView->SetTargetAddress(block, fCurrentAddress);
}
