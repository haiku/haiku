/*
 * Copyright 2012-2014, Rene Gollent, rene@gollent.com.
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

#include "AutoLocker.h"

#include "Architecture.h"
#include "CppLanguage.h"
#include "MessageCodes.h"
#include "Number.h"
#include "UserInterface.h"
#include "Value.h"
#include "Watchpoint.h"


WatchPromptWindow::WatchPromptWindow(::Team* team, target_addr_t address,
	uint32 type, int32 length, UserInterfaceListener* listener)
	:
	BWindow(BRect(), "Edit Watchpoint", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fInitialAddress(address),
	fInitialType(type),
	fInitialLength(length),
	fTeam(team),
	fRequestedAddress(0),
	fRequestedLength(0),
	fAddressInput(NULL),
	fLengthInput(NULL),
	fTypeField(NULL),
	fListener(listener),
	fLanguage(NULL)
{
	AutoLocker< ::Team> teamLocker(fTeam);
	fTeam->AddListener(this);
	fTeam->GetArchitecture()->AcquireReference();
}


WatchPromptWindow::~WatchPromptWindow()
{
	fTeam->GetArchitecture()->ReleaseReference();

	AutoLocker< ::Team> teamLocker(fTeam);
	fTeam->RemoveListener(this);

	if (fLanguage != NULL)
		fLanguage->ReleaseReference();
}


WatchPromptWindow*
WatchPromptWindow::Create(::Team* team, target_addr_t address, uint32 type,
	int32 length, UserInterfaceListener* listener)
{
	WatchPromptWindow* self = new WatchPromptWindow(team, address, type,
		length, listener);

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
	fLanguage = new CppLanguage();

	BString text;
	text.SetToFormat("0x%" B_PRIx64, fInitialAddress);
	fAddressInput = new BTextControl("Address:", text, NULL);

	text.SetToFormat("%" B_PRId32, fInitialLength);
	fLengthInput = new BTextControl("Length:", text, NULL);

	int32 maxDebugRegisters = 0;
	int32 maxBytesPerRegister = 0;
	uint8 debugCapabilityFlags = 0;
	fTeam->GetArchitecture()->GetWatchpointDebugCapabilities(maxDebugRegisters,
		maxBytesPerRegister, debugCapabilityFlags);

	BMenu* typeMenu = new BMenu("Watch type");

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
		.SetInsets(B_USE_DEFAULT_SPACING)
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
WatchPromptWindow::ExpressionEvaluated(
	const Team::ExpressionEvaluationEvent& event)
{
	BMessage message(MSG_EXPRESSION_EVALUATED);
	AutoLocker<BLooper> lock(this);
	if (!lock.IsLocked())
		return;

	BString expression = event.GetExpression();
	if (expression != fAddressInput->Text()
		&& expression != fLengthInput->Text()) {
		return;
	}

	lock.Unlock();
	message.AddInt32("result", event.GetResult());
	Value* value = event.GetValue();
	BReference<Value> reference;
	if (value != NULL) {
		reference.SetTo(value);
		message.AddPointer("value", value);
	}

	if (PostMessage(&message) == B_OK)
		reference.Detach();
}


void
WatchPromptWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_EXPRESSION_EVALUATED:
		{
			BString errorMessage;
			BReference<Value> reference;
			Value* value = NULL;
			if (message->FindPointer("value",
					reinterpret_cast<void**>(&value)) == B_OK) {
				reference.SetTo(value, true);
				BVariant variant;
				value->ToVariant(variant);
				if (variant.Type() == B_UINT64_TYPE) {
					fRequestedAddress = variant.ToUInt64();
					break;
				} else if (variant.Type() == B_INT32_TYPE)
					fRequestedLength = variant.ToInt32();
				else
					value->ToString(errorMessage);
			} else {
				status_t result = message->FindInt32("result");
				errorMessage.SetToFormat("Failed to evaluate expression: %s",
					strerror(result));
			}

			if (fRequestedLength <= 0)
				errorMessage = "Watchpoint length must be at least 1 byte.";

			if (!errorMessage.IsEmpty()) {
				BAlert* alert = new(std::nothrow) BAlert("Edit Watchpoint",
					errorMessage.String(), "Close");
				if (alert != NULL)
					alert->Go();
				break;
			}

			fListener->ClearWatchpointRequested(fInitialAddress);
			fListener->SetWatchpointRequested(fRequestedAddress,
				fTypeField->Menu()->IndexOf(fTypeField->Menu()->FindMarked()),
				fRequestedLength, true);

			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case MSG_SET_WATCHPOINT:
		{
			fRequestedAddress = 0;
			fRequestedLength = 0;

			fListener->ExpressionEvaluationRequested(fLanguage,
				fAddressInput->Text(), B_UINT64_TYPE);

			fListener->ExpressionEvaluationRequested(fLanguage,
				fLengthInput->Text(), B_INT32_TYPE);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}

}
