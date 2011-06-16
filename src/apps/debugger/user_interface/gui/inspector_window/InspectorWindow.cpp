/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "InspectorWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Button.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <ExpressionParser.h>

#include "MemoryView.h"
#include "MessageCodes.h"
#include "Team.h"
#include "UserInterface.h"


enum {
	MSG_NAVIGATE_PREVIOUS_BLOCK = 'npbl',
	MSG_NAVIGATE_NEXT_BLOCK		= 'npnl'
};


InspectorWindow::InspectorWindow(::Team* team, UserInterfaceListener* listener)
	:
	BWindow(BRect(100, 100, 700, 500), "Inspector", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fListener(listener),
	fAddressInput(NULL),
	fHexMode(NULL),
	fTextMode(NULL),
	fMemoryView(NULL),
	fCurrentBlock(NULL),
	fCurrentAddress(0LL),
	fTeam(team)
{
}


InspectorWindow::~InspectorWindow()
{
}


/* static */ InspectorWindow*
InspectorWindow::Create(::Team* team, UserInterfaceListener* listener)
{
	InspectorWindow* self = new InspectorWindow(team, listener);

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

	BMenu* hexMenu = new BMenu("Hex Mode");
	BMessage* message = new BMessage(MSG_SET_HEX_MODE);
	message->AddInt32("mode", HexModeNone);
	BMenuItem* item = new BMenuItem("<None>", message, '0');
	hexMenu->AddItem(item);
	message = new BMessage(*message);
	message->ReplaceInt32("mode", HexMode8BitInt);
	item = new BMenuItem("8-bit integer", message, '1');
	hexMenu->AddItem(item);
	message = new BMessage(*message);
	message->ReplaceInt32("mode", HexMode16BitInt);
	item = new BMenuItem("16-bit integer", message, '2');
	hexMenu->AddItem(item);
	message = new BMessage(*message);
	message->ReplaceInt32("mode", HexMode32BitInt);
	item = new BMenuItem("32-bit integer", message, '3');
	hexMenu->AddItem(item);
	message = new BMessage(*message);
	message->ReplaceInt32("mode", HexMode64BitInt);
	item = new BMenuItem("64-bit integer", message, '4');
	hexMenu->AddItem(item);


	BMenu* textMenu = new BMenu("Text Mode");
	message = new BMessage(MSG_SET_TEXT_MODE);
	message->AddInt32("mode", TextModeNone);
	item = new BMenuItem("<None>", message, 'N');
	textMenu->AddItem(item);
	message = new BMessage(*message);
	message->ReplaceInt32("mode", TextModeASCII);
	item = new BMenuItem("ASCII", message, 'A');
	textMenu->AddItem(item);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(4.0f, 4.0f, 4.0f, 4.0f)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(fAddressInput = new BTextControl("addrInput",
			"Target Address:", "",
			new BMessage(MSG_INSPECT_ADDRESS)))
			.Add(fPreviousBlockButton = new BButton("navPrevious", "<",
				new BMessage(MSG_NAVIGATE_PREVIOUS_BLOCK)))
			.Add(fNextBlockButton = new BButton("navNext", ">",
				new BMessage(MSG_NAVIGATE_NEXT_BLOCK)))
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(fHexMode = new BMenuField("outputStyle", "Hex Mode:",
				hexMenu))
			.AddGlue()
			.Add(fTextMode = new BMenuField("viewMode",  "Text Mode:",
				textMenu))
		.End()
		.Add(scrollView = new BScrollView("memory scroll",
			NULL, 0, false, true), 3.0f)
	.End();

	fHexMode->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTextMode->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	scrollView->SetTarget(fMemoryView = MemoryView::Create());

	fAddressInput->SetTarget(this);
	fPreviousBlockButton->SetTarget(this);
	fNextBlockButton->SetTarget(this);
	fPreviousBlockButton->SetEnabled(false);
	fNextBlockButton->SetEnabled(false);

	hexMenu->SetLabelFromMarked(true);
	hexMenu->SetTargetForItems(fMemoryView);
	textMenu->SetLabelFromMarked(true);
	textMenu->SetTargetForItems(fMemoryView);

	// default to 8-bit format w/ text display
	hexMenu->ItemAt(1)->SetMarked(true);
	textMenu->ItemAt(1)->SetMarked(true);
}



void
InspectorWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_INSPECT_ADDRESS:
		{
			target_addr_t address = 0;
			bool addressValid = false;
			if (msg->FindUInt64("address", &address) != B_OK)
			{
				ExpressionParser parser;
				parser.SetSupportHexInput(true);
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
				} else
					addressValid = true;
			} else {
				addressValid = true;
			}

			if (addressValid) {
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
		case MSG_NAVIGATE_PREVIOUS_BLOCK:
		case MSG_NAVIGATE_NEXT_BLOCK:
		{
			if (fCurrentBlock != NULL)
			{
				target_addr_t address = fCurrentBlock->BaseAddress();
				if (msg->what == MSG_NAVIGATE_PREVIOUS_BLOCK)
					address -= fCurrentBlock->Size();
				else
					address += fCurrentBlock->Size();

				BMessage setMessage(MSG_INSPECT_ADDRESS);
				setMessage.AddUInt64("address", address);
				PostMessage(&setMessage);
			}
			break;
		}
		{
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
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
	BAutolock lock(this);
	if (lock.IsLocked()) {
		fCurrentBlock = block;
		fMemoryView->SetTargetAddress(block, fCurrentAddress);
		fPreviousBlockButton->SetEnabled(true);
		fNextBlockButton->SetEnabled(true);
	}
}
