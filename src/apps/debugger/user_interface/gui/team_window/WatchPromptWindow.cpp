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

#include "MessageCodes.h"
#include "UserInterface.h"
#include "Watchpoint.h"


WatchPromptWindow::WatchPromptWindow(target_addr_t address, uint32 type,
	int32 length, UserInterfaceListener* listener)
	:
	BWindow(BRect(), "Edit Watchpoint", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fInitialAddress(address),
	fInitialType(type),
	fInitialLength(length),
	fAddressInput(NULL),
	fLengthInput(NULL),
	fTypeField(NULL),
	fListener(listener)
{
}


WatchPromptWindow::~WatchPromptWindow()
{
}


WatchPromptWindow*
WatchPromptWindow::Create(target_addr_t address, uint32 type, int32 length,
	UserInterfaceListener* listener)
{
	WatchPromptWindow* self = new WatchPromptWindow(address, type, length,
		listener);

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

	BMenu* typeMenu = new BMenu("Watch Type");
	typeMenu->AddItem(new BMenuItem("Read", NULL));
	typeMenu->AddItem(new BMenuItem("Write", NULL));
	typeMenu->AddItem(new BMenuItem("Read/Write", NULL));
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
			target_addr_t address;
			int32 length;
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
