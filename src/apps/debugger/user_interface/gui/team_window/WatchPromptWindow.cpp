/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "WatchPromptWindow.h"

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <String.h>
#include <TextControl.h>

#include <ExpressionParser.h>

#include "Architecture.h"
#include "MessageCodes.h"
#include "UserInterface.h"
#include "Watchpoint.h"


WatchPromptWindow::WatchPromptWindow(Architecture* architecture,
	target_addr_t address, uint32 type, int32 length,
	UserInterfaceListener* listener)
	:
	BWindow(BRect(), "Edit Watchpoint", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fInitialAddress(address),
	fInitialType(type),
	fInitialLength(length),
	fArchitecture(architecture),
	fAddressInput(NULL),
	fLengthInput(NULL),
	fTypeField(NULL),
	fListener(listener)
{
	fArchitecture->AcquireReference();
}


WatchPromptWindow::~WatchPromptWindow()
{
	fArchitecture->ReleaseReference();
}


WatchPromptWindow*
WatchPromptWindow::Create(Architecture* architecture, target_addr_t address,
	uint32 type, int32 length, UserInterfaceListener* listener)
{
	WatchPromptWindow* self = new WatchPromptWindow(architecture, address,
		type, length, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}

void
WatchPromptWindow::_Init()
{
	BString text;
	text.SetToFormat("0x%" B_PRIx64, fInitialAddress);
	fAddressInput = new BTextControl("Address:", text, NULL);

	text.SetToFormat("%" B_PRId32, fInitialLength);
	fLengthInput = new BTextControl("Length:", text, NULL);

	int32 maxDebugRegisters = 0;
	int32 maxBytesPerRegister = 0;
	uint8 debugCapabilityFlags = 0;
	fArchitecture->GetWatchpointDebugCapabilities(maxDebugRegisters,
		maxBytesPerRegister, debugCapabilityFlags);

	BMenu* typeMenu = new BMenu("Watch Type");

	BMenuItem* watchTypeItem = new BMenuItem("Read", NULL);
	watchTypeItem->SetEnabled(
		(debugCapabilityFlags & WATCHPOINT_CAPABILITY_FLAG_READ) != 0);
	typeMenu->AddItem(watchTypeItem);

	watchTypeItem = new BMenuItem("Write", NULL);
	watchTypeItem->SetEnabled(
		(debugCapabilityFlags & WATCHPOINT_CAPABILITY_FLAG_WRITE) != 0);
	typeMenu->AddItem(watchTypeItem);

	watchTypeItem = new BMenuItem("Read/Write", NULL);
	watchTypeItem->SetEnabled(
		(debugCapabilityFlags & WATCHPOINT_CAPABILITY_FLAG_READ_WRITE) != 0);
	typeMenu->AddItem(watchTypeItem);

	fTypeField = new BMenuField("Type:", typeMenu);
	BLayoutItem* labelItem = fTypeField->CreateLabelLayoutItem();
	labelItem->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(4.0f, 4.0f, 4.0f, 4.0f)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(fAddressInput)
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(fLengthInput)
			.Add(labelItem)
			.Add(fTypeField->CreateMenuBarLayoutItem())
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add((fWatchButton = new BButton("Set",
					new BMessage(MSG_SET_WATCHPOINT))))
			.Add((fCancelButton = new BButton("Cancel",
					new BMessage(B_QUIT_REQUESTED))))
		.End();

	fWatchButton->SetTarget(this);
	fCancelButton->SetTarget(this);

	fTypeField->Menu()->SetLabelFromMarked(true);
	fTypeField->Menu()->ItemAt(fInitialType)->SetMarked(true);
}

void
WatchPromptWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}

void
WatchPromptWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_WATCHPOINT:
		{
			target_addr_t address = 0;
			int32 length = 0;
			ExpressionParser parser;
			parser.SetSupportHexInput(true);
			BString errorMessage;
			try {
				address = parser.EvaluateToInt64(fAddressInput->Text());
				length = (int32)parser.EvaluateToInt64(fLengthInput->Text());
			} catch(ParseException parseError) {
				errorMessage.SetToFormat("Failed to parse data: %s",
					parseError.message.String());
			} catch(...) {
				errorMessage.SetToFormat(
					"Unknown error while parsing address");
			}

			if (!errorMessage.IsEmpty()) {
				BAlert* alert = new(std::nothrow) BAlert("Edit Watchpoint",
					errorMessage.String(), "Close");
				if (alert != NULL)
					alert->Go();
				break;
			}

			fListener->ClearWatchpointRequested(fInitialAddress);
			fListener->SetWatchpointRequested(address, fTypeField->Menu()
					->IndexOf(fTypeField->Menu()->FindMarked()), length, true);

			PostMessage(B_QUIT_REQUESTED);

			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}

}
