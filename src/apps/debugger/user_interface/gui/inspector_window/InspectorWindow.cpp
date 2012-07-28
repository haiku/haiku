/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "InspectorWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <AutoLocker.h>
#include <Button.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <ExpressionParser.h>

#include "Architecture.h"
#include "GuiTeamUiSettings.h"
#include "MemoryView.h"
#include "MessageCodes.h"
#include "Team.h"
#include "UserInterface.h"


enum {
	MSG_NAVIGATE_PREVIOUS_BLOCK = 'npbl',
	MSG_NAVIGATE_NEXT_BLOCK		= 'npnl'
};


InspectorWindow::InspectorWindow(::Team* team, UserInterfaceListener* listener,
	BHandler* target)
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
	fTeam(team),
	fTarget(target)
{
}


InspectorWindow::~InspectorWindow()
{
}


/* static */ InspectorWindow*
InspectorWindow::Create(::Team* team, UserInterfaceListener* listener,
	BHandler* target)
{
	InspectorWindow* self = new InspectorWindow(team, listener, target);

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

	BMenu* endianMenu = new BMenu("Endian Mode");
	message = new BMessage(MSG_SET_ENDIAN_MODE);
	message->AddInt32("mode", EndianModeLittleEndian);
	item = new BMenuItem("Little Endian", message, 'L');
	endianMenu->AddItem(item);
	message = new BMessage(*message);
	message->ReplaceInt32("mode", EndianModeBigEndian);
	item = new BMenuItem("Big Endian", message, 'B');
	endianMenu->AddItem(item);

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
			.Add(fHexMode = new BMenuField("hexMode", "Hex Mode:",
				hexMenu))
			.AddGlue()
			.Add(fEndianMode = new BMenuField("endianMode", "Endian Mode:",
				endianMenu))
			.AddGlue()
			.Add(fTextMode = new BMenuField("viewMode",  "Text Mode:",
				textMenu))
		.End()
		.Add(scrollView = new BScrollView("memory scroll",
			NULL, 0, false, true), 3.0f)
	.End();

	fHexMode->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fEndianMode->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTextMode->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	int32 targetEndian = fTeam->GetArchitecture()->IsBigEndian()
		? EndianModeBigEndian : EndianModeLittleEndian;

	scrollView->SetTarget(fMemoryView = MemoryView::Create(fTeam));

	fAddressInput->SetTarget(this);
	fPreviousBlockButton->SetTarget(this);
	fNextBlockButton->SetTarget(this);
	fPreviousBlockButton->SetEnabled(false);
	fNextBlockButton->SetEnabled(false);

	hexMenu->SetLabelFromMarked(true);
	hexMenu->SetTargetForItems(fMemoryView);
	endianMenu->SetLabelFromMarked(true);
	endianMenu->SetTargetForItems(fMemoryView);
	textMenu->SetLabelFromMarked(true);
	textMenu->SetTargetForItems(fMemoryView);

	// default to 8-bit format w/ text display
	hexMenu->ItemAt(1)->SetMarked(true);
	textMenu->ItemAt(1)->SetMarked(true);

	if (targetEndian == EndianModeBigEndian)
		endianMenu->ItemAt(1)->SetMarked(true);
	else
		endianMenu->ItemAt(0)->SetMarked(true);

	fAddressInput->TextView()->MakeFocus(true);
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
	BMessage settings(MSG_INSPECTOR_WINDOW_CLOSED);
	SaveSettings(settings);

	BMessenger(fTarget).SendMessage(&settings);
	return true;
}


void
InspectorWindow::MemoryBlockRetrieved(TeamMemoryBlock* block)
{
	AutoLocker<BLooper> lock(this);
	if (lock.IsLocked()) {
		fCurrentBlock = block;
		fMemoryView->SetTargetAddress(block, fCurrentAddress);
		fPreviousBlockButton->SetEnabled(true);
		fNextBlockButton->SetEnabled(true);
	}
}


status_t
InspectorWindow::LoadSettings(const GuiTeamUiSettings& settings)
{
	AutoLocker<BLooper> lock(this);
	if (!lock.IsLocked())
		return B_ERROR;

	BMessage inspectorSettings;
	if (settings.Settings("inspectorWindow", inspectorSettings) == B_OK)
		return B_OK;

	BRect frameRect;
	if (inspectorSettings.FindRect("frame", &frameRect) == B_OK) {
		ResizeTo(frameRect.Width(), frameRect.Height());
		MoveTo(frameRect.left, frameRect.top);
	}

	_LoadMenuFieldMode(fHexMode, "Hex", inspectorSettings);
	_LoadMenuFieldMode(fEndianMode, "Endian", inspectorSettings);
	_LoadMenuFieldMode(fTextMode, "Text", inspectorSettings);

	return B_OK;
}


status_t
InspectorWindow::SaveSettings(BMessage& settings)
{
	AutoLocker<BLooper> lock(this);
	if (!lock.IsLocked())
		return B_ERROR;

	settings.MakeEmpty();

	status_t error = settings.AddRect("frame", Frame());
	if (error != B_OK)
		return error;

	error = _SaveMenuFieldMode(fHexMode, "Hex", settings);
	if (error != B_OK)
		return error;

	error = _SaveMenuFieldMode(fEndianMode, "Endian", settings);
	if (error != B_OK)
		return error;

	error = _SaveMenuFieldMode(fTextMode, "Text", settings);
	if (error != B_OK)
		return error;

	return B_OK;
}


void
InspectorWindow::_LoadMenuFieldMode(BMenuField* field, const char* name,
	const BMessage& settings)
{
	BString fieldName;
	int32 mode;
	fieldName.SetToFormat("%sMode", name);
	if (settings.FindInt32(fieldName.String(), &mode) == B_OK) {
		BMenu* menu = field->Menu();
		for (int32 i = 0; i < menu->CountItems(); i++) {
			BInvoker* item = menu->ItemAt(i);
			if (item->Message()->FindInt32("mode") == mode) {
				item->Invoke();
				break;
			}
		}
	}
}


status_t
InspectorWindow::_SaveMenuFieldMode(BMenuField* field, const char* name,
	BMessage& settings)
{
	BMenuItem* item = field->Menu()->FindMarked();
	if (item && item->Message()) {
		int32 mode = item->Message()->FindInt32("mode");
		BString fieldName;
		fieldName.SetToFormat("%sMode", name);
		return settings.AddInt32(fieldName.String(), mode);
	}

	return B_OK;
}
