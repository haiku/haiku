/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "VariableEditWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <String.h>
#include <StringView.h>

#include "AppMessageCodes.h"
#include "TableCellValueEditor.h"
#include "Value.h"
#include "ValueNode.h"


enum {
	MSG_VARIABLE_VALUE_CHANGED		= 'vavc'
};


VariableEditWindow::VariableEditWindow(Value* initialValue, ValueNode* node,
	TableCellValueEditor* editor, BHandler* target)
	:
	BWindow(BRect(), "Edit value",
		B_FLOATING_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fCancelButton(NULL),
	fSaveButton(NULL),
	fTarget(target),
	fNode(node),
	fInitialValue(initialValue),
	fNewValue(NULL),
	fEditor(editor)
{
	fNode->AcquireReference();
	fInitialValue->AcquireReference();
	fEditor->AcquireReference();
	fEditor->AddListener(this);
}


VariableEditWindow::~VariableEditWindow()
{
	fNode->ReleaseReference();
	fInitialValue->ReleaseReference();
	if (fNewValue != NULL)
		fNewValue->ReleaseReference();

	fEditor->RemoveListener(this);
	fEditor->ReleaseReference();
}


VariableEditWindow*
VariableEditWindow::Create(Value* initialValue, ValueNode* node,
	TableCellValueEditor* editor, BHandler* target)
{
	VariableEditWindow* self = new VariableEditWindow(initialValue, node,
		editor, target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}


void
VariableEditWindow::_Init()
{
	BString label;
	BString initialValue;
	fInitialValue->ToString(initialValue);
	label.SetToFormat("Initial value for '%s': %s\n",
		fNode->Name().String(), initialValue.String());

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(new BStringView("initialLabel", label))
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(new BStringView("newLabel", "New value:"))
			.Add(fEditor->GetView())
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add((fCancelButton = new BButton("Cancel",
					new BMessage(B_QUIT_REQUESTED))))
			.Add((fSaveButton = new BButton("Save",
					new BMessage(MSG_WRITE_VARIABLE_VALUE))))
		.End();

	fCancelButton->SetTarget(this);
	fSaveButton->SetTarget(this);
	fSaveButton->MakeDefault(true);
	fEditor->GetView()->MakeFocus(true);
}


void
VariableEditWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


bool
VariableEditWindow::QuitRequested()
{
	fEditor->GetView()->RemoveSelf();

	BMessenger messenger(fTarget);
	messenger.SendMessage(MSG_VARIABLE_EDIT_WINDOW_CLOSED);

	return BWindow::QuitRequested();
}


void
VariableEditWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_VARIABLE_VALUE_CHANGED:
		{
			Value* value;
			if (message->FindPointer("value",
					reinterpret_cast<void**>(&value)) == B_OK) {
				if (fNewValue != NULL)
					fNewValue->ReleaseReference();

				fNewValue = value;
			}
			break;
		}
		case MSG_WRITE_VARIABLE_VALUE:
		{
			BMessage message(MSG_WRITE_VARIABLE_VALUE);
			message.AddPointer("node", fNode);
			message.AddPointer("value", fNewValue);

			// acquire a reference on behalf of the target
			BReference<Value> valueReference(fNewValue);
			if (BMessenger(fTarget).SendMessage(&message) == B_OK) {
				valueReference.Detach();
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
VariableEditWindow::TableCellEditBeginning()
{
}


void
VariableEditWindow::TableCellEditCancelled()
{
	PostMessage(B_QUIT_REQUESTED);
}


void
VariableEditWindow::TableCellEditEnded(Value* newValue)
{
	BReference<Value> valueReference(newValue);
	BMessage message(MSG_VARIABLE_VALUE_CHANGED);
	message.AddPointer("value", newValue);
	if (PostMessage(&message) == B_OK)
		valueReference.Detach();
}
