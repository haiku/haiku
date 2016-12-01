/*
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com. All rights reserved.
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
#include <StringView.h>
#include <TextControl.h>

#include "AppMessageCodes.h"
#include "Architecture.h"
#include "CppLanguage.h"
#include "GuiTeamUiSettings.h"
#include "MemoryView.h"
#include "MessageCodes.h"
#include "Team.h"
#include "UserInterface.h"
#include "Value.h"


enum {
	MSG_NAVIGATE_PREVIOUS_BLOCK 		= 'npbl',
	MSG_NAVIGATE_NEXT_BLOCK				= 'npnl',
	MSG_MEMORY_BLOCK_RETRIEVED			= 'mbre',
	MSG_EDIT_CURRENT_BLOCK				= 'mecb',
	MSG_COMMIT_MODIFIED_BLOCK			= 'mcmb',
	MSG_REVERT_MODIFIED_BLOCK			= 'mrmb'
};


InspectorWindow::InspectorWindow(::Team* team, UserInterfaceListener* listener,
	BHandler* target)
	:
	BWindow(BRect(100, 100, 700, 500), "Inspector", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fListener(listener),
	fAddressInput(NULL),
	fHexMode(NULL),
	fTextMode(NULL),
	fWritableBlockIndicator(NULL),
	fMemoryView(NULL),
	fCurrentBlock(NULL),
	fCurrentAddress(0LL),
	fTeam(team),
	fLanguage(NULL),
	fExpressionInfo(NULL),
	fTarget(target)
{
	AutoLocker< ::Team> teamLocker(fTeam);
	fTeam->AddListener(this);
}


InspectorWindow::~InspectorWindow()
{
	_SetCurrentBlock(NULL);

	if (fLanguage != NULL)
		fLanguage->ReleaseReference();

	if (fExpressionInfo != NULL) {
		fExpressionInfo->RemoveListener(this);
		fExpressionInfo->ReleaseReference();
	}

	AutoLocker< ::Team> teamLocker(fTeam);
	fTeam->RemoveListener(this);
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
	fLanguage = new CppLanguage();
	fExpressionInfo = new ExpressionInfo();
	fExpressionInfo->AddListener(this);

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
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(fAddressInput = new BTextControl("addrInput",
			"Target Address:", "",
			new BMessage(MSG_INSPECT_ADDRESS)))
			.Add(fPreviousBlockButton = new BButton("navPrevious", "<",
				new BMessage(MSG_NAVIGATE_PREVIOUS_BLOCK)))
			.Add(fNextBlockButton = new BButton("navNext", ">",
				new BMessage(MSG_NAVIGATE_NEXT_BLOCK)))
		.End()
		.AddGroup(B_HORIZONTAL)
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
		.AddGroup(B_HORIZONTAL)
			.Add(fWritableBlockIndicator = new BStringView("writableIndicator",
				_GetCurrentWritableIndicator()))
			.AddGlue()
			.Add(fEditBlockButton = new BButton("editBlock", "Edit",
				new BMessage(MSG_EDIT_CURRENT_BLOCK)))
			.Add(fCommitBlockButton = new BButton("commitBlock", "Commit",
				new BMessage(MSG_COMMIT_MODIFIED_BLOCK)))
			.Add(fRevertBlockButton = new BButton("revertBlock", "Revert",
				new BMessage(MSG_REVERT_MODIFIED_BLOCK)))
		.End()
	.End();

	fHexMode->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fEndianMode->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fTextMode->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	int32 targetEndian = fTeam->GetArchitecture()->IsBigEndian()
		? EndianModeBigEndian : EndianModeLittleEndian;

	scrollView->SetTarget(fMemoryView = MemoryView::Create(fTeam, this));

	fAddressInput->SetTarget(this);
	fPreviousBlockButton->SetTarget(this);
	fNextBlockButton->SetTarget(this);
	fPreviousBlockButton->SetEnabled(false);
	fNextBlockButton->SetEnabled(false);

	fEditBlockButton->SetTarget(this);
	fCommitBlockButton->SetTarget(this);
	fRevertBlockButton->SetTarget(this);

	fEditBlockButton->SetEnabled(false);
	fCommitBlockButton->Hide();
	fRevertBlockButton->Hide();

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

	AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY, new BMessage(
			MSG_NAVIGATE_PREVIOUS_BLOCK));
	AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY, new BMessage(
			MSG_NAVIGATE_NEXT_BLOCK));
}


void
InspectorWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_THREAD_STATE_CHANGED:
		{
			::Thread* thread;
			if (message->FindPointer("thread",
					reinterpret_cast<void**>(&thread)) != B_OK) {
				break;
			}

			BReference< ::Thread> threadReference(thread, true);
			if (thread->State() == THREAD_STATE_STOPPED) {
				if (fCurrentBlock != NULL) {
					_SetCurrentBlock(NULL);
					_SetToAddress(fCurrentAddress);
				}
			}
			break;
		}
		case MSG_INSPECT_ADDRESS:
		{
			target_addr_t address = 0;
			if (message->FindUInt64("address", &address) != B_OK) {
				if (fAddressInput->TextView()->TextLength() == 0)
					break;

				fExpressionInfo->SetTo(fAddressInput->Text());

				fListener->ExpressionEvaluationRequested(fLanguage,
					fExpressionInfo);
			} else
				_SetToAddress(address);
			break;
		}
		case MSG_EXPRESSION_EVALUATED:
		{
			BString errorMessage;
			BReference<ExpressionResult> reference;
			ExpressionResult* value = NULL;
			if (message->FindPointer("value",
					reinterpret_cast<void**>(&value)) == B_OK) {
				reference.SetTo(value, true);
				if (value->Kind() == EXPRESSION_RESULT_KIND_PRIMITIVE) {
					Value* primitive = value->PrimitiveValue();
					BVariant variantValue;
					primitive->ToVariant(variantValue);
					if (variantValue.Type() == B_STRING_TYPE) {
						errorMessage.SetTo(variantValue.ToString());
					} else {
						_SetToAddress(variantValue.ToUInt64());
						break;
					}
				}
			} else {
				status_t result = message->FindInt32("result");
				errorMessage.SetToFormat("Failed to evaluate expression: %s",
					strerror(result));
			}

			BAlert* alert = new(std::nothrow) BAlert("Inspect Address",
				errorMessage.String(), "Close");
			if (alert != NULL)
				alert->Go();
			break;
		}

		case MSG_NAVIGATE_PREVIOUS_BLOCK:
		case MSG_NAVIGATE_NEXT_BLOCK:
		{
			if (fCurrentBlock != NULL) {
				target_addr_t address = fCurrentBlock->BaseAddress();
				if (message->what == MSG_NAVIGATE_PREVIOUS_BLOCK)
					address -= fCurrentBlock->Size();
				else
					address += fCurrentBlock->Size();

				BMessage setMessage(MSG_INSPECT_ADDRESS);
				setMessage.AddUInt64("address", address);
				PostMessage(&setMessage);
			}
			break;
		}
		case MSG_MEMORY_BLOCK_RETRIEVED:
		{
			TeamMemoryBlock* block = NULL;
			status_t result;
			if (message->FindPointer("block",
					reinterpret_cast<void **>(&block)) != B_OK
				|| message->FindInt32("result", &result) != B_OK) {
				break;
			}

			if (result == B_OK) {
				_SetCurrentBlock(block);
				fPreviousBlockButton->SetEnabled(true);
				fNextBlockButton->SetEnabled(true);
			} else {
				BString errorMessage;
				errorMessage.SetToFormat("Unable to read address 0x%" B_PRIx64
					": %s", block->BaseAddress(), strerror(result));

				BAlert* alert = new(std::nothrow) BAlert("Inspect address",
					errorMessage.String(), "Close");
				if (alert == NULL)
					break;

				alert->Go(NULL);
				block->ReleaseReference();
			}
			break;
		}
		case MSG_EDIT_CURRENT_BLOCK:
		{
			_SetEditMode(true);
			break;
		}
		case MSG_MEMORY_DATA_CHANGED:
		{
			if (fCurrentBlock == NULL)
				break;

			target_addr_t address;
			if (message->FindUInt64("address", &address) == B_OK
				&& address >= fCurrentBlock->BaseAddress()
				&& address < fCurrentBlock->BaseAddress()
					+ fCurrentBlock->Size()) {
				fCurrentBlock->Invalidate();
				_SetEditMode(false);
				fListener->InspectRequested(address, this);
			}
			break;
		}
		case MSG_COMMIT_MODIFIED_BLOCK:
		{
			// TODO: this could conceivably be extended to detect the
			// individual modified regions and only write those back.
			// That would require potentially submitting multiple separate
			// write requests, and thus require tracking all the writes being
			// waited upon for completion.
			fListener->MemoryWriteRequested(fCurrentBlock->BaseAddress(),
				fMemoryView->GetEditedData(), fCurrentBlock->Size());
			break;
		}
		case MSG_REVERT_MODIFIED_BLOCK:
		{
			_SetEditMode(false);
			break;
		}
		default:
		{
			BWindow::MessageReceived(message);
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
InspectorWindow::ThreadStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STATE_CHANGED);
	BReference< ::Thread> threadReference(event.GetThread());
	message.AddPointer("thread", threadReference.Get());

	if (PostMessage(&message) == B_OK)
		threadReference.Detach();
}


void
InspectorWindow::MemoryChanged(const Team::MemoryChangedEvent& event)
{
	BMessage message(MSG_MEMORY_DATA_CHANGED);
	message.AddUInt64("address", event.GetTargetAddress());
	message.AddUInt64("size", event.GetSize());

	PostMessage(&message);
}


void
InspectorWindow::MemoryBlockRetrieved(TeamMemoryBlock* block)
{
	BMessage message(MSG_MEMORY_BLOCK_RETRIEVED);
	message.AddPointer("block", block);
	message.AddInt32("result", B_OK);
	PostMessage(&message);
}


void
InspectorWindow::MemoryBlockRetrievalFailed(TeamMemoryBlock* block,
	status_t result)
{
	BMessage message(MSG_MEMORY_BLOCK_RETRIEVED);
	message.AddPointer("block", block);
	message.AddInt32("result", result);
	PostMessage(&message);
}


void
InspectorWindow::TargetAddressChanged(target_addr_t address)
{
	AutoLocker<BLooper> lock(this);
	if (lock.IsLocked()) {
		fCurrentAddress = address;
		BString computedAddress;
		computedAddress.SetToFormat("0x%" B_PRIx64, address);
		fAddressInput->SetText(computedAddress.String());
	}
}


void
InspectorWindow::HexModeChanged(int32 newMode)
{
	AutoLocker<BLooper> lock(this);
	if (lock.IsLocked()) {
		BMenu* menu = fHexMode->Menu();
		if (newMode < 0 || newMode > menu->CountItems())
			return;
		BMenuItem* item = menu->ItemAt(newMode);
		item->SetMarked(true);
	}
}


void
InspectorWindow::EndianModeChanged(int32 newMode)
{
	AutoLocker<BLooper> lock(this);
	if (lock.IsLocked()) {
		BMenu* menu = fEndianMode->Menu();
		if (newMode < 0 || newMode > menu->CountItems())
			return;
		BMenuItem* item = menu->ItemAt(newMode);
		item->SetMarked(true);
	}
}


void
InspectorWindow::TextModeChanged(int32 newMode)
{
	AutoLocker<BLooper> lock(this);
	if (lock.IsLocked()) {
		BMenu* menu = fTextMode->Menu();
		if (newMode < 0 || newMode > menu->CountItems())
			return;
		BMenuItem* item = menu->ItemAt(newMode);
		item->SetMarked(true);
	}
}


void
InspectorWindow::ExpressionEvaluated(ExpressionInfo* info, status_t result,
	ExpressionResult* value)
{
	BMessage message(MSG_EXPRESSION_EVALUATED);
	message.AddInt32("result", result);
	BReference<ExpressionResult> reference;
	if (value != NULL) {
		reference.SetTo(value);
		message.AddPointer("value", value);
	}

	if (PostMessage(&message) == B_OK)
		reference.Detach();
}


status_t
InspectorWindow::LoadSettings(const GuiTeamUiSettings& settings)
{
	AutoLocker<BLooper> lock(this);
	if (!lock.IsLocked())
		return B_ERROR;

	BMessage inspectorSettings;
	if (settings.Settings("inspectorWindow", inspectorSettings) != B_OK)
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


void
InspectorWindow::_SetToAddress(target_addr_t address)
{
	fCurrentAddress = address;
	if (fCurrentBlock == NULL
		|| !fCurrentBlock->Contains(address)) {
		fListener->InspectRequested(address, this);
	} else
		fMemoryView->SetTargetAddress(fCurrentBlock, address);
}


void
InspectorWindow::_SetCurrentBlock(TeamMemoryBlock* block)
{
	AutoLocker< ::Team> teamLocker(fTeam);
	if (fCurrentBlock != NULL) {
		fCurrentBlock->RemoveListener(this);
		fCurrentBlock->ReleaseReference();
	}

	fCurrentBlock = block;
	fMemoryView->SetTargetAddress(fCurrentBlock, fCurrentAddress);
	_UpdateWritableOptions();
}


bool
InspectorWindow::_GetWritableState() const
{
	return fCurrentBlock != NULL ? fCurrentBlock->IsWritable() : false;
}


void
InspectorWindow::_SetEditMode(bool enabled)
{
	if (enabled == fMemoryView->GetEditMode())
		return;

	status_t error = fMemoryView->SetEditMode(enabled);
	if (error != B_OK)
		return;

	if (enabled) {
		fEditBlockButton->Hide();
		fCommitBlockButton->Show();
		fRevertBlockButton->Show();
	} else {
		fEditBlockButton->Show();
		fCommitBlockButton->Hide();
		fRevertBlockButton->Hide();
	}

	fHexMode->SetEnabled(!enabled);
	fEndianMode->SetEnabled(!enabled);

	// while the block is being edited, disable block navigation controls.
	fAddressInput->SetEnabled(!enabled);
	fPreviousBlockButton->SetEnabled(!enabled);
	fNextBlockButton->SetEnabled(!enabled);

	InvalidateLayout();
}


void
InspectorWindow::_UpdateWritableOptions()
{
	fEditBlockButton->SetEnabled(_GetWritableState());
	_UpdateWritableIndicator();
}


void
InspectorWindow::_UpdateWritableIndicator()
{
	fWritableBlockIndicator->SetText(_GetCurrentWritableIndicator());
}


const char*
InspectorWindow::_GetCurrentWritableIndicator() const
{
	static char buffer[32];
	snprintf(buffer, sizeof(buffer), "Writable: %s", fCurrentBlock == NULL
			? "N/A" : fCurrentBlock->IsWritable() ? "Yes" : "No");

	return buffer;
}
