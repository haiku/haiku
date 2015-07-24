/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellTextControlEditor.h"

#include "Value.h"
#include "ValueFormatter.h"


enum {
	MSG_INPUT_VALIDATION_NEEDED 	= 'ivne',
	MSG_TEXT_VALUE_CHANGED 			= 'tevc'
};


TableCellTextControlEditor::TableCellTextControlEditor(::Value* initialValue,
	ValueFormatter* formatter)
	:
	TableCellFormattedValueEditor(initialValue, formatter),
	BTextControl("", "", NULL)
{
}


TableCellTextControlEditor::~TableCellTextControlEditor()
{
}


status_t
TableCellTextControlEditor::Init()
{
	BMessage* message = new(std::nothrow) BMessage(
		MSG_INPUT_VALIDATION_NEEDED);
	if (message == NULL)
		return B_NO_MEMORY;

	SetMessage(message);

	message = new(std::nothrow) BMessage(MSG_TEXT_VALUE_CHANGED);
	if (message == NULL)
		return B_NO_MEMORY;

	SetModificationMessage(message);

	return B_OK;
}


BView*
TableCellTextControlEditor::GetView()
{
	return this;
}


void
TableCellTextControlEditor::AttachedToWindow()
{
	BTextControl::AttachedToWindow();

	SetTarget(this);

	BString output;

	if (GetValueFormatter()->FormatValue(InitialValue(), output) == B_OK)
		SetText(output);

	NotifyEditBeginning();
}


void
TableCellTextControlEditor::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TEXT_VALUE_CHANGED:
		{
			// TODO: highlight the input view in some way to show
			// invalid inputs

			// fall through
		}
		case MSG_INPUT_VALIDATION_NEEDED:
		{
			if (ValidateInput()) {
				::Value* value = NULL;
				status_t error = GetValueForInput(value);
				if (error != B_OK)
					break;

				BReference< ::Value> valueReference(value, true);
				NotifyEditCompleted(value);
			}
			break;
		}
		default:
			BTextControl::MessageReceived(message);
			break;
	}
}
