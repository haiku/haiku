/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ExpressionPromptWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <String.h>
#include <TextControl.h>

#include "MessageCodes.h"
#include "UiUtils.h"


enum {
	MSG_CHANGE_EVALUATION_TYPE = 'chet',
};


ExpressionPromptWindow::ExpressionPromptWindow(BHandler* addTarget,
	BHandler* closeTarget)
	:
	BWindow(BRect(), "Add Expression", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fExpressionInput(NULL),
	fCancelButton(NULL),
	fAddButton(NULL),
	fAddTarget(addTarget),
	fCloseTarget(closeTarget),
	fCurrentType(B_INT64_TYPE)
{
}


ExpressionPromptWindow::~ExpressionPromptWindow()
{
}


ExpressionPromptWindow*
ExpressionPromptWindow::Create(BHandler* addTarget, BHandler* closeTarget)
{
	ExpressionPromptWindow* self = new ExpressionPromptWindow(addTarget,
		closeTarget);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}


void
ExpressionPromptWindow::_Init()
{
	fExpressionInput = new BTextControl("Expression:", NULL,
		new BMessage(MSG_EVALUATE_EXPRESSION));
	BLayoutItem* labelItem = fExpressionInput->CreateLabelLayoutItem();
	BLayoutItem* inputItem = fExpressionInput->CreateTextViewLayoutItem();
	inputItem->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	inputItem->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	labelItem->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BMenuField* typeField = new BMenuField("Type:", _BuildTypesMenu());
	typeField->Menu()->SetTargetForItems(this);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(labelItem)
			.Add(inputItem)
			.Add(typeField->CreateLabelLayoutItem())
			.Add(typeField->CreateMenuBarLayoutItem())
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add((fCancelButton = new BButton("Cancel",
					new BMessage(B_QUIT_REQUESTED))))
			.Add((fAddButton = new BButton("Add",
					new BMessage(MSG_ADD_NEW_EXPRESSION))))
		.End();

	fExpressionInput->SetTarget(this);
	fCancelButton->SetTarget(this);
	fAddButton->SetTarget(this);
	fAddButton->MakeDefault(true);
	fExpressionInput->TextView()->MakeFocus(true);
}


BMenu*
ExpressionPromptWindow::_BuildTypesMenu()
{
	BMenu* menu = new BMenu("Types");
	menu->SetLabelFromMarked(true);

	_AddMenuItemForType(menu, B_INT8_TYPE);
	_AddMenuItemForType(menu, B_UINT8_TYPE);
	_AddMenuItemForType(menu, B_INT16_TYPE);
	_AddMenuItemForType(menu, B_UINT16_TYPE);
	_AddMenuItemForType(menu, B_INT32_TYPE);
	_AddMenuItemForType(menu, B_UINT32_TYPE);
	BMenuItem* item = _AddMenuItemForType(menu, B_INT64_TYPE);
	if (item != NULL)
		item->SetMarked(true);

	_AddMenuItemForType(menu, B_UINT64_TYPE);
	_AddMenuItemForType(menu, B_FLOAT_TYPE);
	_AddMenuItemForType(menu, B_DOUBLE_TYPE);

	return menu;
}


BMenuItem*
ExpressionPromptWindow::_AddMenuItemForType(BMenu* menu, type_code type)
{
	BMessage *message = new BMessage(MSG_CHANGE_EVALUATION_TYPE);
	message->AddInt32("type", type);

	BMenuItem* item = new BMenuItem(UiUtils::TypeCodeToString(type), message);
	menu->AddItem(item);

	return item;
}


void
ExpressionPromptWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


bool
ExpressionPromptWindow::QuitRequested()
{
	BMessenger messenger(fCloseTarget);
	messenger.SendMessage(MSG_EXPRESSION_PROMPT_WINDOW_CLOSED);

	return BWindow::QuitRequested();
}


void
ExpressionPromptWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CHANGE_EVALUATION_TYPE:
		{
			fCurrentType = message->FindInt32("type");
			break;
		}

		case MSG_ADD_NEW_EXPRESSION:
		{
			BMessage addMessage(MSG_EXPRESSION_PROMPT_WINDOW_CLOSED);
			addMessage.AddString("expression", fExpressionInput->Text());
			addMessage.AddInt32("type", fCurrentType);
			addMessage.AddMessenger("target", BMessenger(fAddTarget));

			BMessenger(fCloseTarget).SendMessage(&addMessage);
			Quit();
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}

}
