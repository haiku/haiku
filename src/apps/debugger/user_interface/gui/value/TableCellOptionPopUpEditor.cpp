/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellOptionPopUpEditor.h"

#include "Value.h"


enum {
	MSG_SELECTED_OPTION_CHANGED 	= 'msoc'
};


TableCellOptionPopUpEditor::TableCellOptionPopUpEditor(::Value* initialValue,
	ValueFormatter* formatter)
	:
	TableCellFormattedValueEditor(initialValue, formatter),
	BOptionPopUp("optionEditor", NULL, NULL)
{
}


TableCellOptionPopUpEditor::~TableCellOptionPopUpEditor()
{
}


status_t
TableCellOptionPopUpEditor::Init()
{
	BMessage* message = new(std::nothrow) BMessage(
		MSG_SELECTED_OPTION_CHANGED);
	if (message == NULL)
		return B_NO_MEMORY;

	SetMessage(message);

	return ConfigureOptions();
}


BView*
TableCellOptionPopUpEditor::GetView()
{
	return this;
}


void
TableCellOptionPopUpEditor::AttachedToWindow()
{
	BOptionPopUp::AttachedToWindow();

	SetTarget(this);

	NotifyEditBeginning();
}


void
TableCellOptionPopUpEditor::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SELECTED_OPTION_CHANGED:
		{
			::Value* value = NULL;
			if (GetSelectedValue(value) == B_OK) {
				BReference< ::Value> valueReference(value, true);
				NotifyEditCompleted(value);
			} else
				NotifyEditCancelled();

			break;
		}
		default:
			BOptionPopUp::MessageReceived(message);
			break;
	}
}
